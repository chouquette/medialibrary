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

#include <mntent.h>
#include <cstring>

namespace
{
    // Allow private ctors to be used from make_shared
    struct DeviceBuilder : fs::Device
    {
        DeviceBuilder( const std::string& path ) : Device( path ) {}
    };
}

namespace fs
{

const Device::DeviceMap Device::Cache = Device::listDevices();

Device::Device( const std::string& devicePath )
    : m_device( devicePath )
    , m_uuid( "fake uuid" )
    , m_mountpoint( "/mnt/fixme" )
{
}

const std::string& Device::uuid() const
{
    return m_uuid;
}

bool Device::isRemovable() const
{
    return false;
}

const std::string&Device::mountpoint() const
{
    return m_mountpoint;
}

std::shared_ptr<IDevice> Device::fromPath( const std::string& path )
{
    for ( const auto& p : Cache )
    {
        if ( path.find( p.first ) == 0 )
            return p.second;
    }
    return nullptr;
}

std::shared_ptr<IDevice> Device::fromUuid( const std::string& uuid )
{
    for ( const auto& p : Cache )
    {
        const auto& d = p.second;
        if ( d->uuid() == uuid )
            return d;
    }
    return nullptr;
}

Device::DeviceMap Device::listDevices()
{
    DeviceMap res;
    FILE* f = setmntent("/etc/mtab", "r");
    if ( f == nullptr )
        throw std::runtime_error( "Failed to read /etc/mtab" );
    std::unique_ptr<FILE, int(*)(FILE*)>( f, &endmntent );
    char buff[512];
    mntent s;
    while ( getmntent_r( f, &s, buff, sizeof(buff) ) != nullptr )
    {
        // Ugly work around for mountpoints we don't care
        if ( strcmp( s.mnt_type, "proc" ) == 0
                || strcmp( s.mnt_type, "devtmpfs" ) == 0
                || strcmp( s.mnt_type, "devpts" ) == 0
                || strcmp( s.mnt_type, "sysfs" ) == 0
                || strcmp( s.mnt_type, "cgroup" ) == 0
                || strcmp( s.mnt_type, "debugfs" ) == 0
                || strcmp( s.mnt_type, "hugetlbfs" ) == 0
                || strcmp( s.mnt_type, "efivarfs" ) == 0
                || strcmp( s.mnt_type, "securityfs" ) == 0
                || strcmp( s.mnt_type, "mqueue" ) == 0
                || strcmp( s.mnt_type, "pstore" ) == 0
                || strcmp( s.mnt_type, "autofs" ) == 0
                || strcmp( s.mnt_type, "binfmt_misc" ) == 0
                || strcmp( s.mnt_type, "tmpfs" ) == 0 )
            continue;
        res[s.mnt_dir] = std::make_shared<DeviceBuilder>( s.mnt_fsname );
    }
    return res;
}

}
