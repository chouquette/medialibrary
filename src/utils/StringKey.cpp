/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2021 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "StringKey.h"

namespace medialibrary
{
namespace utils
{

StringKey::StringKey( std::string k )
    : m_key( std::move( k ) )
    , m_cstr( nullptr )
{
}

StringKey::StringKey( const char* cstr ) noexcept
    : m_cstr( cstr )
{
}

bool StringKey::operator==( const StringKey& sk ) const
{
    if ( sk.m_cstr != nullptr )
    {
        if ( m_cstr != nullptr )
            return strcmp( sk.m_cstr, m_cstr ) == 0;
        return m_key.compare( sk.m_cstr ) == 0;
    }
    assert( sk.m_cstr == nullptr );
    if ( m_cstr != nullptr )
        return sk.m_key.compare( m_cstr ) == 0;
    return m_key == sk.m_key;
}

}
}
