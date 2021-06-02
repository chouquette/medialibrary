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

#include "CommonDevice.h"
#include "medialibrary/filesystem/Errors.h"
#include "utils/Filename.h"

#include <algorithm>
#include <cassert>
#include <cstring>

namespace medialibrary
{
namespace fs
{

CommonDevice::CommonDevice( const std::string& uuid, const std::string& mountpoint,
                            std::string scheme, bool isRemovable, bool isNetwork )
    : m_uuid( uuid )
    , m_scheme( std::move( scheme ) )
    , m_removable( isRemovable )
    , m_isNetwork( isNetwork )
{
    m_mountpoints.emplace_back( utils::file::toFolderPath( mountpoint ) );
}

const std::string& CommonDevice::uuid() const
{
    return m_uuid;
}

const std::string& CommonDevice::scheme() const
{
    return m_scheme;
}

bool CommonDevice::isRemovable() const
{
    return m_removable;
}

bool CommonDevice::isPresent() const
{
    std::unique_lock<compat::Mutex> lock{ m_mutex };
    return m_mountpoints.empty() == false;
}

bool CommonDevice::isNetwork() const
{
    return m_isNetwork;
}

std::vector<std::string> CommonDevice::mountpoints() const
{
    std::unique_lock<compat::Mutex> lock{ m_mutex };
    if ( m_mountpoints.empty() == true )
        throw fs::errors::DeviceRemoved();
    std::vector<std::string> res;
    res.reserve( m_mountpoints.size() );
    std::transform( cbegin( m_mountpoints ), cend( m_mountpoints ),
                    std::back_inserter( res ), []( const Mountpoint& m ) {
        return m.mrl;
    });
    return res;
}

void CommonDevice::addMountpoint( std::string mp )
{
    Mountpoint mountpoint( utils::file::toFolderPath( mp ) );
    std::unique_lock<compat::Mutex> lock{ m_mutex };
    if ( std::find( cbegin( m_mountpoints ), cend( m_mountpoints ),
                    mountpoint ) != cend( m_mountpoints ) )
        return;
    m_mountpoints.emplace_back( std::move( mountpoint ) );
}

void CommonDevice::removeMountpoint( const std::string& mp )
{
    Mountpoint mountpoint( utils::file::toFolderPath( mp ) );
    std::unique_lock<compat::Mutex> lock{ m_mutex };
    auto it = std::find( begin( m_mountpoints ), end( m_mountpoints ), mountpoint );
    if ( it != end( m_mountpoints ) )
        m_mountpoints.erase( it );
}

std::tuple<bool, std::string>
CommonDevice::matchesMountpoint( const std::string& mrl ) const
{
    Mountpoint mountpoint( utils::file::toFolderPath( mrl ) );
    std::unique_lock<compat::Mutex> lock{ m_mutex };
    auto it = std::find( cbegin( m_mountpoints ), cend( m_mountpoints ), mountpoint );
    if ( it == cend( m_mountpoints ) )
        return std::make_tuple(false, "");
    return std::make_tuple( true, (*it).mrl );
}

std::string CommonDevice::relativeMrl( const std::string& absoluteMrl ) const
{
    std::string res;
    Mountpoint mountpoint( utils::file::toFolderPath( absoluteMrl ) );
    {
        std::unique_lock<compat::Mutex> lock{ m_mutex };
        if ( m_mountpoints.empty() == true )
            throw fs::errors::DeviceRemoved{};
        auto it = std::find( cbegin( m_mountpoints ), cend( m_mountpoints ), mountpoint );
        if ( it == cend( m_mountpoints ) )
            throw errors::NotFound{ absoluteMrl, "device " + m_mountpoints[0].mrl };
        res = (*it).mrl;
    }
    return absoluteMrl.substr( res.length() );
}

std::string CommonDevice::absoluteMrl( const std::string& relativeMrl ) const
{
    std::unique_lock<compat::Mutex> lock{ m_mutex };
    if ( m_mountpoints.empty() == true )
        throw fs::errors::DeviceRemoved{};
    return m_mountpoints[0].mrl + relativeMrl;
}

bool CommonDevice::Mountpoint::operator==( const Mountpoint& lhs ) const
{
    if ( strcasecmp( url.scheme.c_str(), lhs.url.scheme.c_str() ) != 0 )
        return false;
    if ( strcasecmp( url.host.c_str(), lhs.url.host.c_str() ) != 0 )
        return false;
    if ( url.port != lhs.url.port )
    {
        if ( strcasecmp( lhs.url.scheme.c_str(), "smb" ) == 0 )
        {
            if ( ! ( url.port.empty() == true && lhs.url.port == "445" ) &&
                 ! ( lhs.url.port.empty() == true && url.port == "445" ) )
                return false;
        }
        else
            return false;
    }
    return strncasecmp( lhs.url.path.c_str(), url.path.c_str(), url.path.length() ) == 0;
}

}
}
