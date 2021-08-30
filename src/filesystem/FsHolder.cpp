/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2021 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "FsHolder.h"
#include "Device.h"
#include "database/SqliteTransaction.h"
#include "logging/Logger.h"
#include "medialibrary/filesystem/IDevice.h"
#include "factory/DeviceListerFactory.h"
#include "discoverer/DiscovererWorker.h"
#include "parser/Parser.h"

#ifdef HAVE_LIBVLC
# include "utils/VLCInstance.h"
# include "filesystem/libvlc/DeviceLister.h"
#endif

#include <algorithm>

namespace medialibrary
{

FsHolder::FsHolder( MediaLibrary* ml )
    : m_ml( ml )
    , m_networkDiscoveryEnabled( false )
    , m_started( false )
{
    auto devLister = factory::createDeviceLister();
    if ( devLister != nullptr )
        m_deviceListers["file://"] = std::move( devLister );
#ifdef HAVE_LIBVLC
    auto lanSds = VLCInstance::get().mediaDiscoverers( VLC::MediaDiscoverer::Category::Lan );
    auto deviceLister = std::make_shared<fs::libvlc::DeviceLister>( "smb://" );
    for ( const auto& sd : lanSds )
        deviceLister->addSD( sd.name() );
    m_deviceListers["smb://"] = std::move( deviceLister );
#endif
}

FsHolder::~FsHolder()
{
    assert( m_callbacks.empty() == true );
}

bool FsHolder::addFsFactory( std::shared_ptr<fs::IFileSystemFactory> fsFactory )
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };

    auto it = std::find_if( cbegin( m_fsFactories  ),
                            cend( m_fsFactories ),
                            [&fsFactory]( const std::shared_ptr<fs::IFileSystemFactory>& fsf ) {
        return fsFactory->scheme() == fsf->scheme();
    });
    if ( it != cend( m_fsFactories ) )
        return false;
    m_fsFactories.push_back( std::move( fsFactory ) );
    return true;
}

void FsHolder::registerDeviceLister( const std::string& scheme,
                                     DeviceListerPtr lister )
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };

    m_deviceListers[scheme] = std::move( lister );
}

DeviceListerPtr FsHolder::deviceLister( const std::string& scheme ) const
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };

    auto it = m_deviceListers.find( scheme );
    if ( it == cend( m_deviceListers ) )
        return nullptr;
    return it->second;
}

bool FsHolder::setNetworkEnabled( bool enabled )
{
    auto expected = !enabled;
    if ( m_networkDiscoveryEnabled.compare_exchange_strong( expected,
                                                            enabled ) == false )
    {
        /* If the value didn't change, that's not a failure */
        return true;
    }
    if ( m_started.load( std::memory_order_acquire ) == false )
        return true;

    auto affected = false;

    std::unique_ptr<sqlite::Transaction> t;
    if ( enabled == false )
        t = m_ml->getConn()->newTransaction();

    for ( auto fsFactory : m_fsFactories )
    {
        if ( fsFactory->isNetworkFileSystem() == false )
            continue;

        if ( enabled == true )
        {
            if ( fsFactory->start( this ) == true )
            {
                fsFactory->refreshDevices();
                affected = true;
            }
        }
        else
        {
            auto devices = Device::fetchByScheme( m_ml, fsFactory->scheme() );
            for ( const auto& d : devices )
                d->setPresent( false );
            fsFactory->stop();
            affected = true;
        }
    }

    if ( t != nullptr )
        t->commit();

    return affected;
}

bool FsHolder::isNetworkEnabled() const
{
    return m_networkDiscoveryEnabled.load( std::memory_order_acquire );
}

void FsHolder::refreshDevices( fs::IFileSystemFactory& fsFactory )
{
    auto devices = Device::fetchByScheme( m_ml, fsFactory.scheme() );
    for ( auto& d : devices )
    {
        refreshDevice( *d, &fsFactory );
    }
    LOG_DEBUG( "Done refreshing devices in database." );
}

void FsHolder::startFsFactoriesAndRefresh()
{
    auto expected = false;
    if ( m_started.compare_exchange_strong( expected, true ) == false )
        return;

    {
    std::lock_guard<compat::Mutex> lock( m_mutex );

    for ( const auto& fsFactory : m_fsFactories )
    {
        /*
         * We only want to start the fs factory if it is a local one, or if
         * it's a network one and network discovery is enabled
         */
        if ( m_networkDiscoveryEnabled == true ||
             fsFactory->isNetworkFileSystem() == false )
        {
            fsFactory->start( this );
            fsFactory->refreshDevices();
        }
    }
    }

    std::lock_guard<compat::Mutex> lock{ m_cbMutex };
    auto devices = Device::fetchAll( m_ml );
    for ( const auto& d : devices )
    {
        auto fsFactory = fsFactoryForMrlLocked( d->scheme() );
        refreshDevice( *d, fsFactory.get() );
    }
}

void FsHolder::stopNetworkFsFactories()
{
    auto expected = true;
    if ( m_started.compare_exchange_strong( expected, false ) == false )
        return;

    std::lock_guard<compat::Mutex> lock( m_mutex );

    for ( auto& fsFactory : m_fsFactories )
    {
        if ( fsFactory->isNetworkFileSystem() == false ||
             fsFactory->isStarted() == false )
            continue;
        fsFactory->stop();
    }
}

std::shared_ptr<fs::IFileSystemFactory>
FsHolder::fsFactoryForMrl( const std::string& mrl ) const
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };

    return fsFactoryForMrlLocked( mrl );
}

std::shared_ptr<fs::IFileSystemFactory>
FsHolder::fsFactoryForMrlLocked( const std::string& mrl ) const
{
    for ( const auto& f : m_fsFactories )
    {
        if ( f->isMrlSupported( mrl ) )
        {
            if ( f->isNetworkFileSystem() &&
                 m_networkDiscoveryEnabled.load( std::memory_order_acquire ) == false )
                return nullptr;
            return f;
        }
    }
    return nullptr;
}

void FsHolder::refreshDevice( Device& device, fs::IFileSystemFactory* fsFactory )
{
    auto deviceFs = fsFactory != nullptr ?
                        fsFactory->createDevice( device.uuid() ) : nullptr;
    auto fsDevicePresent = deviceFs != nullptr && deviceFs->isPresent();
    if ( device.isPresent() != fsDevicePresent )
    {
        LOG_INFO( "Device ", device.uuid(), " changed presence state: ",
                  device.isPresent(), " -> ", fsDevicePresent );
        device.setPresent( fsDevicePresent );
    }
    else
        LOG_INFO( "Device ", device.uuid(), " presence is unchanged" );

    if ( device.isRemovable() == true && device.isPresent() == true )
        device.updateLastSeen();
}

void FsHolder::startFsFactory( fs::IFileSystemFactory& fsFactory ) const
{
    auto fsCb = static_cast<const fs::IFileSystemFactoryCb*>( this );
    fsFactory.start( const_cast<fs::IFileSystemFactoryCb*>( fsCb ) );
    fsFactory.refreshDevices();
}

void FsHolder::registerCallback( IFsHolderCb* cb )
{
    std::lock_guard<compat::Mutex> lock{ m_cbMutex };

    auto it = std::find( cbegin( m_callbacks ), cend( m_callbacks ), cb );
    if ( it != cend( m_callbacks ) )
    {
        assert( !"Double registration of IFsHolderCb" );
        return;
    }
    m_callbacks.push_back( cb );
}

void FsHolder::unregisterCallback( IFsHolderCb* cb )
{
    std::lock_guard<compat::Mutex> lock{ m_cbMutex };

    auto it = std::find( cbegin( m_callbacks ), cend( m_callbacks ), cb );
    if ( it == cend( m_callbacks ) )
    {
        assert( !"Unregistering unregistered callback" );
        return;
    }
    m_callbacks.erase( it );
}

void FsHolder::onDeviceMounted( const fs::IDevice& deviceFs,
                                const std::string& newMountpoint )
{
    /*
     * This callback might be called synchronously by an external device lister
     * upon a call to fsFactory->refreshDevices()
     * This means we must not acquire m_mutex from here as it would most likely
     * already be held.
     */
    auto device = Device::fromUuid( m_ml, deviceFs.uuid(), deviceFs.scheme() );
    if ( device == nullptr )
        return;
    if ( device->isPresent() == deviceFs.isPresent() )
    {
        if ( deviceFs.isNetwork() == true )
            device->addMountpoint( newMountpoint, time( nullptr ) );
        return;
    }

    assert( device->isRemovable() == true );

    LOG_INFO( "Device ", deviceFs.uuid(), " changed presence state: ",
              device->isPresent() ? "1" : "0", " -> ",
              deviceFs.isPresent() ? "1" : "0" );
    auto previousPresence = device->isPresent();
    auto t = m_ml->getConn()->newTransaction();
    device->setPresent( deviceFs.isPresent() );
    if ( deviceFs.isNetwork() == true )
        device->addMountpoint( newMountpoint, time( nullptr ) );
    t->commit();
    if ( previousPresence == false )
    {
        // We need to reload the entrypoint in case a previous discovery was
        // interrupted before its end (causing the tasks that were spawned
        // to be deleted when the device go away, requiring a new discovery)
        // Also, there might be new content on the device since it was last
        // scanned.
        // We also want to resume any parsing tasks that were previously
        // started before the device went away
        std::lock_guard<compat::Mutex> lock{ m_cbMutex };
        assert( deviceFs.isPresent() == true );
        for ( const auto cb : m_callbacks )
            cb->onDeviceReappearing( device->id() );
    }
}

void FsHolder::onDeviceUnmounted( const fs::IDevice& deviceFs,
                                  const std::string& )
{
    auto device = Device::fromUuid( m_ml, deviceFs.uuid(), deviceFs.scheme() );
    if ( device == nullptr )
    {
        /*
         * If we haven't added this device to the database, it means we never
         * discovered anything on it, so we don't really care if it's mounted
         * or not
         */
        return;
    }

    assert( device->isRemovable() == true );
    if ( device->isPresent() == deviceFs.isPresent() )
        return;

    LOG_INFO( "Device ", deviceFs.uuid(), " changed presence state: ",
              device->isPresent() ? "1" : "0", " -> ",
              deviceFs.isPresent() ? "1" : "0" );
    device->setPresent( deviceFs.isPresent() );
    if ( deviceFs.isPresent() == false )
    {
        std::lock_guard<compat::Mutex> lock{ m_cbMutex };
        assert( deviceFs.isPresent() == false );
        for ( const auto cb : m_callbacks )
            cb->onDeviceReappearing( device->id() );
    }
}


}
