/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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
#include <tuple>

namespace medialibrary
{

namespace utils
{

namespace title
{

std::string sanitize( const std::string& title );

/**
 * @brief analyze Attempts to extract information about a media based on its title
 * @param title The title, preferably sanitized.
 * @return A tuple containing: success, season number, episode number, show name, episode title
 *
 * The returned title will not be sanitized, so it preferably should be sanitized
 * prior to calling this
 */
std::tuple<bool, uint32_t, uint32_t, std::string, std::string>
analyze( const std::string& title );

}

}

}
