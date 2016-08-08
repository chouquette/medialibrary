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

#include "Device.h"
#include "Directory.h"
#include "logging/Logger.h"
#include "medialibrary/IDeviceLister.h"

namespace
{
    // Allow private ctors to be used from make_shared
    struct DeviceBuilder : public medialibrary::fs::Device
    {
        template <typename... Args>
        DeviceBuilder( Args&&... args ) : Device( std::forward<Args>( args )... ) {}
    };
}

namespace medialibrary
{

namespace fs
{

Cache<Device::DeviceCacheMap> Device::DeviceCache;

Device::Device( const std::string& uuid, const std::string& mountpoint, bool isRemovable )
    : m_uuid( uuid )
    , m_mountpoint( mountpoint )
    , m_present( true )
    , m_removable( isRemovable )
{
    if ( *m_mountpoint.crbegin() != '/' )
        m_mountpoint += '/';
}

const std::string& Device::uuid() const
{
    return m_uuid;
}

bool Device::isRemovable() const
{
    return m_removable;
}

bool Device::isPresent() const
{
    return m_present;
}

const std::string&Device::mountpoint() const
{
    return m_mountpoint;
}

std::shared_ptr<IDevice> Device::fromPath( const std::string& path )
{
    auto lock = DeviceCache.lock();
    std::shared_ptr<IDevice> res;
    for ( const auto& p : DeviceCache.get() )
    {
        if ( path.find( p.second->mountpoint() ) == 0 )
        {
            if ( res == nullptr || res->mountpoint().length() < p.second->mountpoint().length() )
                res = p.second;
        }
    }
    return res;
}

std::shared_ptr<IDevice> Device::fromUuid( const std::string& uuid )
{
    auto lock = DeviceCache.lock();

    auto it = DeviceCache.get().find( uuid );
    if ( it != end( DeviceCache.get() ) )
        return it->second;
    return nullptr;
}

void Device::setDeviceLister( DeviceListerPtr lister )
{
    auto lock = DeviceCache.lock();
    DeviceLister = lister;
    refreshDeviceCacheLocked();
}

void Device::refreshDeviceCache()
{
    auto lock = DeviceCache.lock();
    refreshDeviceCacheLocked();
}

void Device::refreshDeviceCacheLocked()
{
    if ( DeviceCache.isCached() == false )
        DeviceCache = DeviceCacheMap{};
    DeviceCache.get().clear();
    auto devices = DeviceLister->devices();
    for ( const auto& d : devices )
    {
        const auto& uuid = std::get<0>( d );
        const auto& mountpoint = std::get<1>( d );
        const auto removable = std::get<2>( d );
        DeviceCache.get().emplace( uuid, std::make_shared<DeviceBuilder>( uuid, mountpoint, removable ) );
    }
}

}

}
