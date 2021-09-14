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
#include <cstddef>

namespace medialibrary
{
namespace utils
{
namespace hash
{
    /*
     * Given the amount of inlining and macro based magic in xxhash, we do not want
     * to include xxhash.h from this header, so we only publicly expose those
     * helpers
     */
    uint64_t xxFromBuff( const uint8_t* buff, size_t size );
    uint64_t xxFromFile( const std::string& path );
    std::string toString( uint64_t hash );
}
}
}
