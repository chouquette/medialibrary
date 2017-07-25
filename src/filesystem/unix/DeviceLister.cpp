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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "DeviceLister.h"
#include "logging/Logger.h"
#include "utils/Filename.h"

#include <dirent.h>
#include <mntent.h>
#include <sys/types.h>
#include <vector>
#include <memory>
#include <cerrno>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <limits.h>
#include <unistd.h>

namespace medialibrary
{
namespace fs
{

DeviceLister::DeviceMap DeviceLister::listDevices() const
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

DeviceLister::MountpointMap DeviceLister::listMountpoints() const
{
    static const std::vector<std::string> allowedFsType = { "vfat", "exfat", "sdcardfs", "fuse",
                                                            "ntfs", "fat32", "ext3", "ext4", "esdfs" };
    MountpointMap res;
    errno = 0;
    mntent* s;
    char buff[512];
    mntent smnt;
    FILE* f = setmntent("/etc/mtab", "r");
    if ( f == nullptr )
        throw std::runtime_error( "Failed to read /etc/mtab" );
    std::unique_ptr<FILE, int(*)(FILE*)> fPtr( f, &endmntent );
    while ( getmntent_r( f, &smnt, buff, sizeof(buff) ) != nullptr )
    {
        s = &smnt;
        if ( std::find( begin( allowedFsType ), end( allowedFsType ), s->mnt_type ) == end( allowedFsType ) )
            continue;
        auto deviceName = s->mnt_fsname;
        if ( res.count( deviceName ) == 0 )
        {
            LOG_INFO( "Discovered mountpoint ", deviceName, " mounted on ", s->mnt_dir, " (", s->mnt_type, ')' );
            res[deviceName] = s->mnt_dir;
        }
        else
            LOG_INFO( "Ignoring duplicated mountpoint (", s->mnt_dir, ") for device ", deviceName );
        errno = 0;
    }
    if ( errno != 0 )
    {
        LOG_ERROR( "Failed to read mountpoints: ", strerror( errno ) );
    }
    return res;
}

std::string DeviceLister::deviceFromDeviceMapper( const std::string& devicePath ) const
{
    if ( devicePath.find( "/dev/mapper" ) != 0 )
        return {};
    char linkPath[PATH_MAX + 1];
    auto linkSize = readlink( devicePath.c_str(), linkPath, PATH_MAX );
    if ( linkSize < 0 )
    {
        std::stringstream err;
        err << "Failed to resolve device -> mapper link: "
            << devicePath << " (" << strerror(errno) << ')';
        throw std::runtime_error( err.str() );
    }
    linkPath[linkSize] = 0;
    LOG_INFO( "Resolved ", devicePath, " to ", linkPath, " device mapper" );
    const auto dmName = utils::file::fileName( linkPath );
    std::string dmSlavePath = "/sys/block/" + dmName + "/slaves";
    std::unique_ptr<DIR, int(*)(DIR*)> dir( opendir( dmSlavePath.c_str() ), &closedir );
    std::string res;
    if ( dir == nullptr )
    {
        std::stringstream err;
        err << "Failed to open device-mapper slaves directory (" << linkPath << ')';
        throw std::runtime_error( err.str() );
    }
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

bool DeviceLister::isRemovable( const std::string& deviceName, const std::string& mountpoint ) const
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
        if ( fread(&buff, sizeof(buff), 1, removableFile.get() ) != 1 )
            return buff == '1';
        return false;
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

std::vector<std::tuple<std::string, std::string, bool>> DeviceLister::devices() const
{
    std::vector<std::tuple<std::string, std::string, bool>> res;
    try
    {
        DeviceMap mountpoints = listMountpoints();
        if ( mountpoints.empty() == true )
        {
            LOG_WARN( "Failed to detect any mountpoint" );
            return res;
        }
        auto devices = listDevices();
        if ( devices.empty() == true )
        {
            LOG_WARN( "Failed to detect any device" );
            return res;
        }
        for ( const auto& p : mountpoints )
        {
            const auto& devicePath = p.first;
            auto deviceName = utils::file::fileName( devicePath );
            const auto& mountpoint = p.second;
            auto it = devices.find( deviceName );
            std::string uuid;
            if ( it != end( devices ) )
                uuid = it->second;
            else
            {
                LOG_INFO( "Failed to find device for mountpoint ", mountpoint, ". Attempting to resolve"
                          " using device mapper" );
                try
                {
                    deviceName = deviceFromDeviceMapper( devicePath );
                }
                catch( std::runtime_error& ex )
                {
                    LOG_WARN( "Failed to resolve using device mapper: ", ex.what() );
                    continue;
                }
                it = devices.find( deviceName );
                if ( it != end( devices ) )
                    uuid = it->second;
                else
                {
                    LOG_ERROR( "Failed to resolve mountpoint ", mountpoint, " to any known device" );
                    continue;
                }
            }
            auto removable = isRemovable( deviceName, mountpoint );
            res.emplace_back( std::make_tuple( uuid, "file://" + mountpoint, removable ) );
        }
    }
    catch(std::runtime_error& ex)
    {
        LOG_WARN( "Failed to list devices: ", ex.what(), ". Falling back to a dummy device containing '/'");
        res.emplace_back( std::make_tuple( "{dummy-device}", "file:///", false ) );
    }

    return res;
}

}
}
