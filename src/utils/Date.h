/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include <ctime>
#include <string>

namespace medialibrary
{
namespace utils
{
namespace date
{
    /**
     * @brief mktime Converts a struct tm to a UNIX timestamp
     * @param t A struct representing a broken down time
     * @return A date as a Unix timestamp expressed in UTC
     */
    time_t mktime( struct tm* t );
    /**
     * @brief fromStr Attempts to convert a string to a timestamp
     * @param str The date expressed as a string
     * @param t A pointer to a struct tm, which will contain the result in case of success
     * @return true if the conversion was successful, false otherwise
     *
     * If the conversion fails, the given struct tm's content it undefined.
     */
    bool fromStr( const std::string& str, struct tm *t );
}
}
}
