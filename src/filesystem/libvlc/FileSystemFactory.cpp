/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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
#include "config.h"
#endif

#ifndef HAVE_LIBVLC
# error This file requires libvlc
#endif

#include "FileSystemFactory.h"
#include "filesystem/libvlc/Directory.h"
#include "filesystem/libvlc/Device.h"
#include "utils/Filename.h"
#include "MediaLibrary.h"
#include "logging/Logger.h"

#include <algorithm>
#include <cstring>
#include <cassert>

namespace medialibrary
{
namespace fs
{
namespace libvlc
{

FileSystemFactory::FileSystemFactory( MediaLibraryPtr ml,
                                      const std::string& scheme )
    : m_scheme( scheme )
    , m_deviceLister( ml->deviceLister( scheme ) )
    , m_cb( nullptr )
{
    m_isNetwork = strncasecmp( m_scheme.c_str(), "file://",
                               m_scheme.length() ) != 0;
}

std::shared_ptr<fs::IDirectory> FileSystemFactory::createDirectory( const std::string& mrl )
{
    return std::make_shared<Directory>( mrl, *this );
}

std::shared_ptr<fs::IFile> FileSystemFactory::createFile( const std::string& mrl )
{
    assert( isStarted() == true );
    auto fsDir = createDirectory( utils::file::directory( mrl ) );
    assert( fsDir != nullptr );
    return fsDir->file( mrl );
}

std::shared_ptr<fs::IDevice>
FileSystemFactory::createDevice( const std::string& uuid )
{
    // Let deviceByUuidLocked handle the isStarted assertion
    std::unique_lock<compat::Mutex> lock( m_devicesLock );
    return deviceByUuidLocked( uuid );
}

std::shared_ptr<fs::IDevice>
FileSystemFactory::createDeviceFromMrl( const std::string& mrl )
{
    // Let deviceByMrlLocked handle the isStarted assertion
    std::unique_lock<compat::Mutex> lock( m_devicesLock );
    return deviceByMrlLocked( mrl );
}

void FileSystemFactory::refreshDevices()
{
    assert( isStarted() == true );
    m_deviceLister->refresh();
}

bool FileSystemFactory::isMrlSupported( const std::string& mrl ) const
{
    return strncasecmp( m_scheme.c_str(), mrl.c_str(),
                        m_scheme.length() ) == 0;
}

bool FileSystemFactory::isNetworkFileSystem() const
{
    return m_isNetwork;
}

const std::string& FileSystemFactory::scheme() const
{
    return m_scheme;
}

bool FileSystemFactory::start( fs::IFileSystemFactoryCb* cb )
{
    LOG_DEBUG( "Starting FS Factory with scheme ", m_scheme );
    if ( isStarted() == true )
        return true;
    m_cb = cb;
    return m_deviceLister->start( this );
}

void FileSystemFactory::stop()
{
    assert( isStarted() == true );
    m_deviceLister->stop();
    m_cb = nullptr;
}

void FileSystemFactory::onDeviceMounted( const std::string& uuid,
                                         const std::string& mountpoint,
                                         bool removable )
{
    assert( isStarted() == true );
    auto addMountpoint = true;
    std::shared_ptr<fs::IDevice> device;
    {
        std::unique_lock<compat::Mutex> lock( m_devicesLock );
        device = deviceByUuidLocked( uuid );
        if ( device == nullptr )
        {
            device = std::make_shared<Device>( uuid, mountpoint,
                                               m_scheme, removable, m_isNetwork );
            m_devices.push_back( device );
            addMountpoint = false;
        }
    }
    /* Update the mountpoint outside of the device list lock context */
    if ( addMountpoint == true )
        device->addMountpoint( mountpoint );

    m_cb->onDeviceMounted( *device, mountpoint );

    if ( addMountpoint == false )
        m_devicesCond.notify_all();
}

void FileSystemFactory::onDeviceUnmounted( const std::string& uuid,
                                           const std::string& mountpoint )
{
    assert( isStarted() == true );
    std::shared_ptr<fs::IDevice> device;
    {
        std::unique_lock<compat::Mutex> lock( m_devicesLock );
        device = deviceByUuidLocked( uuid );
    }
    if ( device == nullptr )
    {
        assert( !"Unknown device was unmounted" );
        return;
    }
    device->removeMountpoint( mountpoint );
    m_cb->onDeviceUnmounted( *device, mountpoint );
}

std::shared_ptr<fs::IDevice> FileSystemFactory::deviceByUuidLocked( const std::string& uuid )
{
    assert( isStarted() == true );
    auto it = std::find_if( begin( m_devices ), end( m_devices ),
                            [&uuid]( const std::shared_ptr<fs::IDevice>& d ) {
        return strcasecmp( d->uuid().c_str(), uuid.c_str() ) == 0;
    });
    if ( it == end( m_devices ) )
        return nullptr;
    return *it;
}

std::shared_ptr<fs::IDevice> FileSystemFactory::deviceByMrlLocked( const std::string& mrl )
{
    assert( isStarted() == true );
    std::shared_ptr<fs::IDevice> res;
    std::string mountpoint;
    for ( const auto& d : m_devices )
    {
        auto match = d->matchesMountpoint( mrl );
        if ( std::get<0>( match ) == false )
            continue;
        auto newMountpoint = std::get<1>( match );
        if ( res == nullptr || newMountpoint.length() > mountpoint.length() )
        {
            res = d;
            mountpoint = std::move( newMountpoint );
        }
    }
    return res;
}

bool FileSystemFactory::isStarted() const
{
    return m_cb != nullptr;
}

bool FileSystemFactory::waitForDevice( const std::string& mrl, uint32_t timeout ) const
{
    assert( isStarted() == true );
    std::unique_lock<compat::Mutex> lock( m_devicesLock );
    return m_devicesCond.wait_for( lock, std::chrono::milliseconds{ timeout },
                            [this, &mrl](){
        for ( const auto& d : m_devices )
        {
            auto match = d->matchesMountpoint( mrl );
            if ( std::get<0>( match ) == true )
                return true;
        }
        return false;
    });
}

}
}
}
