/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2018-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

namespace medialibrary
{
namespace utils
{
namespace str
{

/**
 * @brief trim Trim the provided string in place
 */
std::string& trim(std::string& value );

/**
 * @brief trim Returns a copy of the trimmed provided string
 */
std::string trim( const std::string& value );

namespace utf8
{

/**
 * @brief nbChars Counts the number of characters (as opposed to bytes) in an utf8 string
 * @param value The string to analyze
 * @return The number of characters
 */
size_t nbChars( const std::string& value );

/**
 * @brief nbBytes Counts the number of bytes that contain the specified number
 *                of characters
 * @param input The string to analyze
 * @param offset An offset from the begining of the string, in bytes
 * @param nbChars The number of characters to check
 * @return The number of bytes that holds the first nbChars after the provided offset
 */
size_t nbBytes( const std::string& input, size_t offset, size_t nbChars );

/**
 * @brief commonPattern Returns the common pattern between lhs & rhs
 * @param lhs The first string to compare
 * @param lhsOffset An offset in the first string, in bytes
 * @param rhs The second string to compare
 * @param rhsOffset An offset in the second string, in bytes
 * @param minPatternSize The minimum common pattern size in characters
 * @return The largest common pattern between the 2 strings, or an empty string
 * if they don't share the minimum number of characters
 */
std::string commonPattern( const std::string& lhs, size_t lhsOffset,
                           const std::string& rhs, size_t rhsOffset,
                           size_t minPatternSize );

}

}
}
}
