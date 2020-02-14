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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "DeviceLister.h"
#include "logging/Logger.h"
#include "utils/Filename.h"
#include "utils/Directory.h"
#include "medialibrary/filesystem/Errors.h"

#include <dirent.h>
#include <mntent.h>
#include <vector>
#include <memory>
#include <cerrno>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <cassert>

namespace medialibrary
{
namespace fs
{

DeviceLister::DeviceMap DeviceLister::listDevices() const
{
    static const std::vector<std::string> bannedDevice = { "loop" };
    const std::string devPath = "/dev/disk/by-uuid/";
    // Don't use fs::Directory to iterate, as it resolves the symbolic links automatically.
    // We need the link name & what it points to.
    std::unique_ptr<DIR, int(*)(DIR*)> dir( opendir( devPath.c_str() ),
                                            &closedir );
    if ( dir == nullptr )
    {
        std::stringstream err;
        err << "Failed to open /dev/disk/by-uuid: " << strerror(errno);
        throw fs::errors::DeviceListing{ err.str() };
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
        char linkPath[PATH_MAX + 1];
        auto nbBytes = readlink( path.c_str(), linkPath, PATH_MAX );
        if ( nbBytes < 0 )
        {
            std::stringstream err;
            err << "Failed to resolve uuid -> device link: "
                << result->d_name << " (" << strerror(errno) << ')';
            throw fs::errors::DeviceListing{ err.str() };
        }
        linkPath[nbBytes] = 0;
        auto deviceName = utils::file::fileName( linkPath );
        if ( std::find_if( begin( bannedDevice ), end( bannedDevice ),
                        [&deviceName]( const std::string& pattern ) {
                            return deviceName.length() >= pattern.length() &&
                                   deviceName.find( pattern ) == 0;
                            }) != end( bannedDevice ) )
            continue;
        auto uuid = result->d_name;
        LOG_INFO( "Discovered device ", deviceName, " -> {", uuid, '}' );
        res[deviceName] = uuid;
    }
    return res;
}

DeviceLister::MountpointMap DeviceLister::listMountpoints() const
{
    const std::vector<std::string> allowedFsType = getAllowedFsTypes();
    MountpointMap res;
    errno = 0;
    mntent* s;
    char buff[512];
    mntent smnt;
    FILE* f = setmntent("/proc/mounts", "r");
    if ( f == nullptr )
        throw fs::errors::DeviceListing{ "Failed to read /proc/mounts" };
    std::unique_ptr<FILE, int(*)(FILE*)> fPtr( f, &endmntent );
    while ( getmntent_r( f, &smnt, buff, sizeof(buff) ) != nullptr )
    {
        s = &smnt;
        if ( std::find( begin( allowedFsType ), end( allowedFsType ),
                        s->mnt_type ) == end( allowedFsType ) )
            continue;
        auto deviceName = s->mnt_fsname;
        if ( strncmp( "/dev/loop", deviceName, strlen( "/dev/loop") ) == 0 )
            continue;
        auto& mountpoints = res[deviceName];
        LOG_INFO( "Discovered mountpoint ", deviceName, " mounted on ",
                  s->mnt_dir, " (", s->mnt_type, ')' );
        mountpoints.emplace_back( utils::file::toMrl( s->mnt_dir ) );
        errno = 0;
    }
    if ( errno != 0 )
    {
        LOG_ERROR( "Failed to read mountpoints: ", strerror( errno ) );
    }
    return res;
}

std::pair<std::string,std::string>
DeviceLister::deviceFromDeviceMapper( const std::string& devicePath ) const
{
    if ( devicePath.compare( 0, strlen( "/dev/mapper" ), "/dev/mapper" ) != 0 )
        return {};
    char linkPath[PATH_MAX + 1];
    auto linkSize = readlink( devicePath.c_str(), linkPath, PATH_MAX );
    if ( linkSize < 0 )
    {
        std::stringstream err;
        err << "Failed to resolve device -> mapper link: "
            << devicePath << " (" << strerror(errno) << ')';
        throw fs::errors::DeviceMapper{ err.str() };
    }
    linkPath[linkSize] = 0;
    LOG_DEBUG( "Resolved ", devicePath, " to ", linkPath, " device mapper" );
    const auto dmName = utils::file::fileName( linkPath );
    std::string dmSlavePath = "/sys/block/" + dmName + "/slaves";
    std::unique_ptr<DIR, int(*)(DIR*)> dir( opendir( dmSlavePath.c_str() ),
                                            &closedir );
    std::string res;
    if ( dir == nullptr )
    {
        std::stringstream err;
        err << "Failed to open device-mapper slaves directory (" <<
               linkPath << ')';
        throw fs::errors::DeviceMapper{ err.str() };
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
    return { dmName, res };
}

bool DeviceLister::isRemovable( const std::string& partitionPath ) const
{
    // We have a partition, such as /dev/sda1. We need to find the associated
    // device

    struct stat s;
    if ( stat( partitionPath.c_str(), &s ) != 0 )
        return false;

    std::string partitionSymlink = "/sys/dev/block/" +
            std::to_string( major( s.st_rdev ) ) + ":" +
            std::to_string( minor( s.st_rdev ) );

    // This path is a symlink to a /sys/devices/....../block/device/partition folder
    // We are interested in the <device> part
    std::string partitionBlockPath;
    try
    {
        partitionBlockPath = utils::fs::toAbsolute( partitionSymlink );
    }
    catch ( const fs::errors::System& ex )
    {
        LOG_WARN( "Failed to absolute path from block symlink: ", partitionSymlink,
                  " ", ex.what() );
        return false;
    }
    auto deviceName = utils::file::directoryName(
                        utils::file::parentDirectory( partitionBlockPath ) );

    std::string removableFilePath = "/sys/block/" + deviceName + "/removable";
    std::unique_ptr<FILE, decltype(&fclose)> removableFile(
                fopen( removableFilePath.c_str(), "r" ), &fclose );
    // Assume the file isn't removable by default
    if ( removableFile != nullptr )
    {
        char buff;
        if ( fread(&buff, sizeof(buff), 1, removableFile.get() ) == 1 )
            return buff == '1';
        return false;
    }
    return false;
}

std::vector<std::string> DeviceLister::getAllowedFsTypes() const
{
    std::unique_ptr<FILE, int(*)(FILE*)> fsFile( fopen( "/proc/filesystems", "r" ), &fclose );
    if ( fsFile == nullptr )
    {
        // In the unlikely event there are no procfs support, return a best guess
        return { "vfat", "exfat", "sdcardfs", "fuse", "ntfs", "fat32", "ext3",
                 "ext4", "esdfs", "xfs"
        };
    }
    std::vector<std::string> res;
    size_t buffSize = 128u;
    auto buff = static_cast<char*>( malloc( buffSize ) );
    std::unique_ptr<char, decltype(&free)> buffPtr( buff, &free );
    while ( getline( &buff, &buffSize, fsFile.get() ) != -1 )
    {
        std::istringstream iss{ buff };
        std::string fsType;
        iss >> fsType;
        if ( fsType == "nodev" )
            continue;
        res.push_back( std::move( fsType ) );
    }
    return res;
}

std::vector<DeviceLister::Device> DeviceLister::devices() const
{
    std::vector<Device> res;
    try
    {
        MountpointMap mountpoints = listMountpoints();
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
            assert( p.second.empty() == false );

            const auto& partitionPath = p.first;
            auto deviceName = utils::file::fileName( partitionPath );
            auto it = devices.find( deviceName );
            std::string uuid;
            if ( it != end( devices ) )
                uuid = it->second;
            else
            {
                std::pair<std::string, std::string> dmPair;
                LOG_INFO( "Failed to find known device with name ", deviceName,
                          ". Attempting to resolve using device mapper" );
                try
                {
                    // Fetch a pair containing the device-mapper device name and
                    // the block device it points to
                    dmPair = deviceFromDeviceMapper( partitionPath );
                }
                catch( fs::errors::DeviceMapper& ex )
                {
                    LOG_WARN( ex.what() );
                    continue;
                }
                // First try with the block device
                it = devices.find( dmPair.second );
                if ( it != end( devices ) )
                    uuid = it->second;
                else
                {
                    // Otherwise try with the device mapper name.
                    it = devices.find( dmPair.first );
                    if( it != end( devices ) )
                        uuid = it->second;
                    else
                    {
                        LOG_ERROR( "Failed to resolve device ", deviceName,
                                   " to any known device" );
                        continue;
                    }
                }
            }
            auto removable = isRemovable( partitionPath );

            res.emplace_back( uuid, std::move( p.second ), removable );
        }
    }
    catch( const fs::errors::DeviceListing& ex )
    {
        LOG_WARN( ex.what(), ". Falling back to a dummy device containing '/'" );
        res.emplace_back( "{dummy-device}",
                          std::vector<std::string> { utils::file::toMrl( "/" ) },
                          false );
    }
    return res;
}

}
}
