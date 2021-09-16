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

#pragma once

#include <string>
#include <functional>
#include <cassert>
#include <cstring>

#include "utils/XxHasher.h"

namespace medialibrary
{
namespace utils
{

/**
 * @brief The StringKey class is a wrapper around a std::string to allow
 * heterogeneous lookups in associated containers.
 * This also leverages xxhash instead of the default hash function
 */
class StringKey
{
public:
    StringKey( std::string k );
    StringKey( const char* cstr ) noexcept;

    bool operator==( const StringKey& sk ) const;

private:
    std::string m_key;
    const char* m_cstr;
    friend struct std::hash<StringKey>;
};

}
}

namespace std
{
template <>
struct hash<medialibrary::utils::StringKey>
{
    size_t operator()( const medialibrary::utils::StringKey& k ) const noexcept
    {
        namespace hash = medialibrary::utils::hash;
        if ( k.m_cstr != nullptr )
            return hash::xxFromBuff( reinterpret_cast<const uint8_t*>( k.m_cstr ),
                                     strlen( k.m_cstr ) );
        return hash::xxFromBuff( reinterpret_cast<const uint8_t*>( k.m_key.c_str() ),
                                 k.m_key.length() );
    }
};
}
