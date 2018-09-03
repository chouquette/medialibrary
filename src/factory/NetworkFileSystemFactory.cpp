/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#include "NetworkFileSystemFactory.h"
#include "filesystem/network/Directory.h"
#include "utils/VLCInstance.h"
#include "MediaLibrary.h"

#include <algorithm>

namespace medialibrary
{
namespace factory
{

NetworkFileSystemFactory::NetworkFileSystemFactory( const std::string& protocol, const std::string& name )
    : m_protocol( protocol )
    , m_discoverer( VLCInstance::get(), name )
    , m_mediaList( m_discoverer.mediaList() )
{
    auto& em = m_mediaList->eventManager();
    em.onItemAdded( [this]( VLC::MediaPtr m, int ) { onDeviceAdded( m ); } );
    em.onItemDeleted( [this]( VLC::MediaPtr m, int ) { onDeviceRemoved( m ); } );
    m_discoverer.start();
}

std::shared_ptr<fs::IDirectory> NetworkFileSystemFactory::createDirectory( const std::string& path )
{
    return std::make_shared<fs::NetworkDirectory>( path, *this );
}

std::shared_ptr<fs::IDevice> NetworkFileSystemFactory::createDevice( const std::string& mrl )
{
    std::shared_ptr<fs::IDevice> res;
    std::unique_lock<compat::Mutex> lock( m_devicesLock );

    m_deviceCond.wait_for( lock, std::chrono::seconds{ 5 }, [this, &res, &mrl]() {
        auto it = std::find_if( begin( m_devices ), end( m_devices ), [&mrl]( const Device& d ) {
            return d.mrl == mrl;
        });
        if ( it == end( m_devices ) )
            return false;
        res = it->device;
        return true;
    });
    return res;
}

std::shared_ptr<fs::IDevice> NetworkFileSystemFactory::createDeviceFromMrl( const std::string& path )
{
    std::lock_guard<compat::Mutex> lock( m_devicesLock );
    auto it = std::find_if( begin( m_devices ), end( m_devices ), [&path]( const Device& d ) {
        return path.compare( 0, d.mrl.length(), d.mrl );
    });
    if ( it == end( m_devices ) )
        return nullptr;
    return it->device;
}

void NetworkFileSystemFactory::refreshDevices()
{
}

bool NetworkFileSystemFactory::isMrlSupported( const std::string& path ) const
{
    const auto idx = m_protocol.length();
    if ( path.compare( 0, idx, m_protocol ) != 0 )
        return false;
    return path[idx] == ':' && path[idx + 1] == '/' && path[idx + 2] == '/';

}

bool NetworkFileSystemFactory::isNetworkFileSystem() const
{
    return true;
}

void NetworkFileSystemFactory::onDeviceAdded( VLC::MediaPtr media )
{
    const auto& mrl = media->mrl();
    if ( mrl.compare( 0, m_protocol.length(), m_protocol ) != 0 )
        return;

    std::lock_guard<compat::Mutex> lock( m_devicesLock );
    auto it = std::find_if( begin( m_devices ), end( m_devices ), [&mrl]( const Device& d ) {
        return d.mrl== mrl;
    });
    if ( it != end( m_devices ) )
        return;
    const auto idx = m_protocol.length();
    if ( mrl[idx] != ':' || mrl[idx + 1] != '/' || mrl[idx + 2] != '/' )
        return;

    auto name = mrl.substr( idx + 3 );

    m_devices.emplace_back( name, mrl, *media );
    m_deviceCond.notify_one();
}

void NetworkFileSystemFactory::onDeviceRemoved( VLC::MediaPtr media )
{
    std::lock_guard<compat::Mutex> lock( m_devicesLock );
    const auto& mrl = media->mrl();
    m_devices.erase( std::remove_if( begin( m_devices ), end( m_devices ), [&mrl]( const Device& d ) {
        return d.mrl == mrl;
    }), end( m_devices ) );
}

}
}
