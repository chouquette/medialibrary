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
# include "config.h"
#endif

#include "DeviceLister.h"
#include "medialibrary/filesystem/Errors.h"

#if WINAPI_FAMILY_PARTITION (WINAPI_PARTITION_DESKTOP)

#include "logging/Logger.h"
#include "utils/Charsets.h"
#include "utils/Filename.h"

#include <memory>
#include <sstream>

#include <windows.h>
#include <winnetwk.h>

namespace medialibrary
{
namespace fs
{

std::vector<CommonDeviceLister::Device> DeviceLister::networkDevices() const
{
    std::vector<Device> devs;
    DWORD cbBuffer = 16384;
    DWORD cEntries = -1;

    HANDLE enumHandle;
    if ( WNetOpenEnum( RESOURCE_CONNECTED, RESOURCETYPE_DISK, 0, nullptr, &enumHandle ) != NO_ERROR )
    {
        std::stringstream ss;
        ss << "WNetOpenEnum error: #" << GetLastError();
        throw fs::errors::DeviceListing{ ss.str() };
    }
    std::unique_ptr<typename std::remove_pointer<HANDLE>::type, decltype(&WNetCloseEnum)> handlePtr {
        enumHandle, &WNetCloseEnum
    };

    auto buffer = std::make_unique<char[]>( cbBuffer );
    auto netResources = reinterpret_cast<LPNETRESOURCE>( buffer.get() );
    do
    {
        auto res = WNetEnumResource( enumHandle, &cEntries, netResources, &cbBuffer );
        if ( res == ERROR_NO_MORE_ITEMS )
            break;
        if ( res != NO_ERROR )
        {
            std::stringstream ss;
            ss << "WNetEnumResource error: #" << GetLastError();
            throw fs::errors::DeviceListing{ ss.str() };
        }
        std::string mountpoint = charset::FromWide( netResources->lpLocalName ).get();
        std::string uuid = charset::FromWide( netResources->lpRemoteName ).get();
        devs.emplace_back( std::move( uuid ),
            std::vector<std::string>{ utils::file::toMrl( mountpoint ) }, true );
    } while ( true );

    return devs;
}

std::vector<CommonDeviceLister::Device> DeviceLister::localDevices() const
{
    wchar_t volumeName[MAX_PATH];
    auto handle = FindFirstVolume( volumeName, sizeof(volumeName)/sizeof(volumeName[0]) );
    if ( handle == INVALID_HANDLE_VALUE )
    {
        std::stringstream ss;
        ss << "FindFirstVolume error: #" << GetLastError();
        throw fs::errors::DeviceListing{ ss.str() };
    }
    std::unique_ptr<typename std::remove_pointer<HANDLE>::type, decltype(&FindVolumeClose)>
            uh( handle, &FindVolumeClose );
    std::vector<Device> res;
    for ( BOOL success =  TRUE; ; success = FindNextVolume( handle, volumeName, sizeof( volumeName )/sizeof(volumeName[0]) ) )
    {
        if ( success == FALSE )
        {
            auto err = GetLastError();
            if ( err == ERROR_NO_MORE_FILES )
                break;
            std::stringstream ss;
            ss << "FindNextVolume error: #" << err;
            throw fs::errors::DeviceListing{ ss.str() };
        }

        auto lastChar = wcslen( volumeName ) - 1;
        if ( volumeName[0] != L'\\' || volumeName[1] != L'\\' || volumeName[2] != L'?' ||
             volumeName[3] != L'\\' || volumeName[lastChar] != L'\\' )
            continue;

        wchar_t buffer[MAX_PATH + 1];
        DWORD buffLength = sizeof( buffer ) / sizeof( wchar_t );

        if ( GetVolumePathNamesForVolumeName( volumeName, buffer, buffLength, &buffLength ) == 0 )
            continue;
        std::string mountpoint = charset::FromWide( buffer ).get();

        // Filter out anything which isn't a removable or fixed drive. We don't care about network
        // drive here.
        auto type = GetDriveType( buffer );
        if ( type != DRIVE_REMOVABLE && type != DRIVE_FIXED && type != DRIVE_REMOTE )
            continue;

        std::string uuid =  charset::FromWide( volumeName ).get();

        LOG_INFO( "Discovered device ", uuid, "; mounted on ", mountpoint, "; removable: ",
                  type == DRIVE_REMOVABLE ? "yes" : "no" );
        res.emplace_back( uuid,
                          std::vector<std::string>{ utils::file::toMrl( mountpoint ) },
                          type == DRIVE_REMOVABLE );
    }
    return res;
}

std::vector<CommonDeviceLister::Device> DeviceLister::devices() const
{
    auto devs = localDevices();
    try
    {
        auto netDevs = networkDevices();
        devs.insert(devs.end(), std::make_move_iterator( begin( netDevs ) ),
                    std::make_move_iterator( end( netDevs ) ) );
    }
    catch ( const fs::errors::DeviceListing& ex )
    {
        LOG_DEBUG( "Failed to list network devices: ", ex.what() );
    }

    return devs;
}

}
}

#endif
