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
#include "utils/Filename.h"
#include "logging/Logger.h"

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <limits.h>
#include <mntent.h>
#include <unistd.h>
#include <sys/types.h>

namespace
{
    // Allow private ctors to be used from make_shared
    struct DeviceBuilder : public fs::Device
    {
        template <typename... Args>
        DeviceBuilder( Args&&... args ) : Device( std::forward<Args>( args )... ) {}
    };
}

namespace fs
{

Device::DeviceMap Device::Devices;
Device::MountpointMap Device::Mountpoints;
Device::DeviceCacheMap Device::DeviceCache;

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
    std::shared_ptr<IDevice> res;
    for ( const auto& p : DeviceCache )
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
    auto it = DeviceCache.find( uuid );
    if ( it != end( DeviceCache ) )
        return it->second;
    return nullptr;
}

bool Device::populateCache()
{
    Devices = listDevices();
    Mountpoints = listMountpoints();
    DeviceCache = populateDeviceCache();
    return true;
}

Device::DeviceMap Device::listDevices()
{

    static const std::vector<std::string> deviceBlacklist = { "loop", "dm-" };
    const std::string devPath = "/dev/disk/by-uuid/";
    // Don't use fs::Directory to iterate, as it resolves the symbolic links automatically.
    // We need the link name & what it points to.
    std::unique_ptr<DIR, int(*)(DIR*)> dir( opendir( devPath.c_str() ), &closedir );
    if ( dir == nullptr )
    {
        std::stringstream err;
        err << "Failed to open /dev/disk/by-uuid: " << strerror(errno);
        throw std::runtime_error( err.str() );
    }
    DeviceMap res;
    dirent* result = nullptr;

    while ( ( result = readdir( dir.get() ) ) != nullptr )
    {
        if ( strcmp( result->d_name, "." ) == 0 ||
             strcmp( result->d_name, ".." ) == 0 )
        {
            continue;
        }
        std::string path = devPath + result->d_name;
        char linkPath[PATH_MAX] = {};
        if ( readlink( path.c_str(), linkPath, PATH_MAX ) < 0 )
        {
            std::stringstream err;
            err << "Failed to resolve uuid -> device link: "
                << result->d_name << " (" << strerror(errno) << ')';
            throw std::runtime_error( err.str() );
        }
        auto deviceName = utils::file::fileName( linkPath );
        if ( std::find_if( begin( deviceBlacklist ), end( deviceBlacklist ), [&deviceName]( const std::string& pattern ) {
                return deviceName.length() >= pattern.length() && deviceName.find( pattern ) == 0;
            }) != end( deviceBlacklist ) )
            continue;
        auto uuid = result->d_name;
        LOG_INFO( "Discovered device ", deviceName, " -> {", uuid, '}' );
        res[deviceName] = uuid;
    }
    return res;
}

Device::MountpointMap Device::listMountpoints()
{
    static const std::vector<std::string> allowedFsType = { "vfat", "exfat", "sdcardfs", "fuse",
                                                            "ntfs", "fat32", "ext3", "ext4", "esdfs" };
    MountpointMap res;
    FILE* f = setmntent("/etc/mtab", "r");
    if ( f == nullptr )
        throw std::runtime_error( "Failed to read /etc/mtab" );
    std::unique_ptr<FILE, int(*)(FILE*)> fPtr( f, &endmntent );
    char buff[512];
    mntent s;
    errno = 0;
    while ( getmntent_r( f, &s, buff, sizeof(buff) ) != nullptr )
    {
        if ( std::find( begin( allowedFsType ), end( allowedFsType ), s.mnt_type ) == end( allowedFsType ) )
            continue;
        auto deviceName = s.mnt_fsname;
        LOG_INFO( "Discovered mountpoint ", deviceName, " mounted on ", s.mnt_dir, " (", s.mnt_type, ')' );
        res[deviceName] = s.mnt_dir;
        errno = 0;
    }
    if ( errno != 0 )
    {
        LOG_ERROR( "Failed to read mountpoints: ", strerror( errno ) );
    }
    return res;
}

Device::DeviceCacheMap Device::populateDeviceCache()
{
    Device::DeviceCacheMap res;
    for ( const auto& p : Mountpoints )
    {
        const auto& devicePath = p.first;
        auto deviceName = utils::file::fileName( devicePath );
        const auto& mountpoint = p.second;
        auto it = Devices.find( deviceName );
        std::string uuid;
        if ( it != end( Devices ) )
            uuid = it->second;
        else
        {
            deviceName = deviceFromDeviceMapper( devicePath );
            it = Devices.find( deviceName );
            if ( it != end( Devices ) )
                uuid = it->second;
            else
            {
                LOG_ERROR( "Failed to resolve mountpoint ", mountpoint, " to any known device" );
                continue;
            }
        }
        auto removable = isRemovable( deviceName, mountpoint );
        LOG_INFO( "Adding device to cache: {", uuid, "} mounted on ", mountpoint, " Removable: ", removable );
        res[uuid] = std::make_shared<DeviceBuilder>( uuid, mountpoint, removable );
    }
    return res;
}

std::string Device::deviceFromDeviceMapper( const std::string& devicePath )
{
    if ( devicePath.find( "/dev/mapper" ) != 0 )
        return {};
    char linkPath[PATH_MAX];
    if ( readlink( devicePath.c_str(), linkPath, PATH_MAX ) < 0 )
    {
        std::stringstream err;
        err << "Failed to resolve device -> mapper link: "
            << devicePath << " (" << strerror(errno) << ')';
        throw std::runtime_error( err.str() );
    }
    const auto dmName = utils::file::fileName( linkPath );
    std::string dmSlavePath = "/sys/block/" + dmName + "/slaves";
    std::unique_ptr<DIR, int(*)(DIR*)> dir( opendir( dmSlavePath.c_str() ), &closedir );
    std::string res;
    if ( dir == nullptr )
        return {};
    dirent* result;
    while ( ( result = readdir( dir.get() ) ) != nullptr )
    {
        if ( strcmp( result->d_name, "." ) == 0 ||
             strcmp( result->d_name, ".." ) == 0 )
        {
            continue;
        }
        if ( res.empty() == true )
            res = result->d_name;
        else
            LOG_WARN( "More than one slave for device mapper ", linkPath );
    }
    LOG_INFO( "Device mapper ", dmName, " maps to ", res );
    return res;
}

bool Device::isRemovable( const std::string& deviceName, const std::string& mountpoint )
{
#ifndef TIZEN
    (void)mountpoint;
    std::stringstream removableFilePath;
    removableFilePath << "/sys/block/" << deviceName << "/removable";
    std::unique_ptr<FILE, int(*)(FILE*)> removableFile( fopen( removableFilePath.str().c_str(), "r" ), &fclose );
    // Assume the file isn't removable by default
    if ( removableFile != nullptr )
    {
        char buff;
        fread(&buff, sizeof(buff), 1, removableFile.get() );
        return buff == '1';
    }
#else
    (void)deviceName;
    static const std::vector<std::string> SDMountpoints = { "/opt/storage/sdcard" };
    auto it = std::find_if( begin( SDMountpoints ), end( SDMountpoints ), [mountpoint]( const std::string& pattern ) {
        return mountpoint.length() >= pattern.length() && mountpoint.find( pattern ) == 0;
    });
    if ( it != end( SDMountpoints ) )
    {
        LOG_INFO( "Considering mountpoint ", mountpoint, " a removable SDCard" );
        return true;
    }
#endif
    return false;
}

}
