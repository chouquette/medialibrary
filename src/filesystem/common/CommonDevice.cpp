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
# include "config.h"
#endif

#include "CommonDevice.h"
#include "utils/Filename.h"

#include <cassert>

namespace medialibrary
{
namespace fs
{

CommonDevice::CommonDevice( const std::string& uuid, const std::string& mountpoint, bool isRemovable )
    : m_uuid( uuid )
    , m_mountpoints( { utils::file::toFolderPath( mountpoint ) } )
    , m_present( true )
    , m_removable( isRemovable )
{
}

const std::string& CommonDevice::uuid() const
{
    return m_uuid;
}

bool CommonDevice::isRemovable() const
{
    return m_removable;
}

bool CommonDevice::isPresent() const
{
    return m_present;
}

void CommonDevice::setPresent( bool present )
{
    m_present = present;
}

const std::string& CommonDevice::mountpoint() const
{
    assert( m_mountpoints.empty() == false );
    return m_mountpoints[0];
}

void CommonDevice::addMountpoint( std::string mountpoint )
{
    m_mountpoints.push_back( std::move( mountpoint ) );
}

std::tuple<bool, std::string>
CommonDevice::matchesMountpoint( const std::string& mrl ) const
{
    for ( const auto& m : m_mountpoints )
    {
        if ( mrl.find( m ) == 0 )
            return { true, m };
    }
    return { false, "" };
}

std::string CommonDevice::relativeMrl( const std::string& absoluteMrl ) const
{
    assert( m_mountpoints.empty() == false );
    return utils::file::removePath( absoluteMrl, m_mountpoints[0] );
}

std::string CommonDevice::absoluteMrl( const std::string& relativeMrl ) const
{
    assert( m_mountpoints.empty() == false );
    return m_mountpoints[0] + relativeMrl;
}

}
}
