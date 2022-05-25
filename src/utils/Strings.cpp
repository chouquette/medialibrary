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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Strings.h"

#include <algorithm>
#include <cctype>
#include <cassert>

namespace medialibrary
{
namespace utils
{
namespace str
{

std::string trim( std::string value )
{
    value.erase( begin( value ), std::find_if( begin( value ), end( value ), []( char c ) {
            return isspace( c ) == false;
        }));
    value.erase( std::find_if( value.rbegin(), value.rend(), []( char c ) {
            return isspace( c ) == false;
        }).base(), value.end() );
    return value;
}

namespace utf8
{

size_t nbChars( const std::string& value )
{
    uint32_t nbChars = 0u;
    for ( auto i = 0u; i < value.size(); )
    {
        if ( ( value[i] & 0x80 ) == 0 )
        {
            ++i;
            ++nbChars;
        }
        else
        {
            uint8_t c = value[i];
            /*
             * Skip over the leading unicode byte, and skip an extra byte
             * for each leading bit set to 1 in the first byte
             */
            ++i;
            c <<= 1;
            while ( ( c & 0x80 ) != 0 )
            {
                if ( i >= value.size() )
                    return 0;
                uint8_t nextByte = value[i];
                if ( ( nextByte & 0x80 ) != 0x80 )
                    return 0;
                ++i;
                c <<= 1;
            }
            ++nbChars;
        }
    }
    return nbChars;
}

size_t nbBytes( const std::string& input, size_t offset, size_t nbChars )
{
    if ( offset >= input.size() )
        return 0;
    size_t i = offset;
    size_t res = 0;

    while ( nbChars > 0 && i < input.size() )
    {
        /* Skip over non-terminal UTF8 bytes */
        uint8_t c = input[i];
        uint8_t currentCharNbBytes = 1;
        if ( ( c & 0x80 ) != 0 )
        {
            c <<= 1;
            ++i;
            while ( ( c & 0x80 ) != 0 )
            {
                if ( i >= input.size() )
                    return 0;
                uint8_t nextByte = input[i];
                if ( ( nextByte & 0x80 ) != 0x80 )
                    return 0;
                ++i;
                currentCharNbBytes++;
                c <<= 1;
            }
        }
        else
            ++i;

        res += currentCharNbBytes;
        --nbChars;
    }
    return res;
}

std::string commonPattern( const std::string& lhs, size_t lhsOffset,
                           const std::string& rhs, size_t rhsOffset,
                           size_t minPatternSize )
{
    auto lhsIdx = lhsOffset;
    auto rhsIdx = rhsOffset;
    auto patternSize = 0u;
    while ( lhsIdx < lhs.size() && rhsIdx < rhs.size() )
    {
        /* For ascii, we can do case-insensitive comparison */
        if ( ( lhs[lhsIdx] & 0x80 ) == 0 &&
             ( rhs[rhsIdx] & 0x80 ) == 0 )
        {
            if ( tolower( lhs[lhsIdx] ) != tolower( rhs[rhsIdx] ) )
                break;
            ++lhsIdx;
            ++rhsIdx;
            ++patternSize;
        }
        else
        {
            /*
             * We *need* to work with unsigned, be it for the bits operations
             * or the comparisons
             * Using std::string::operator[] without cast will return a signed char.
             */
            uint8_t lhsC = lhs[lhsIdx];
            if ( lhsC != static_cast<uint8_t>( rhs[rhsIdx] ) )
                break;
            lhsC <<= 1;
            uint8_t multiByteOffset = 1;
            while ( ( lhsC & 0x80 ) != 0 )
            {
                if ( lhsIdx + multiByteOffset >= lhs.size() ||
                     rhsIdx + multiByteOffset >= rhs.size() )
                {
                    /*
                     * In case something is invalid or we reached the end of an
                     * input string, we don't want to return a potentially
                     * partial codepoint, so we reset the offset and break out
                     */
                    multiByteOffset = 0;
                    break;
                }
                uint8_t nextByte = lhs[lhsIdx + multiByteOffset];
                if ( ( nextByte & 0x80 ) != 0x80 ||
                     nextByte != static_cast<uint8_t>( rhs[rhsIdx + multiByteOffset] ) )
                {
                    multiByteOffset = 0;
                    break;
                }
                lhsC <<= 1;
                ++multiByteOffset;
            }
            /*
             * If the multi byte offset has been reset to 0, it means we either
             * found an invalid codepoint, or a multi byte comparison failed
             * in any of its bytes. In anycase, we don't want to return this
             * partial code point
             */
            if ( multiByteOffset == 0 )
                break;
            lhsIdx += multiByteOffset;
            rhsIdx += multiByteOffset;
            ++patternSize;
        }
    }
    if ( patternSize < minPatternSize )
        return {};
    return lhs.substr( lhsOffset, lhsIdx - lhsOffset );
}

}

}
}
}
