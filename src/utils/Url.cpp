/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2017-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Url.h"

#include <stdexcept>
#include <cstdlib>
#include <cstring>

namespace
{
inline bool isSafe( char c )
{
    return strchr( ".-_~/"
#ifdef _WIN32
                    "\\"
#endif
                   , c ) != nullptr;
}

std::string::const_iterator encodeSegment( std::string& res,
                                           std::string::const_iterator begin,
                                           std::string::const_iterator end,
                                           const char* extraChars )
{
    for ( auto i = begin; i < end; ++i )
    {
        // This must be an unsigned char for the bits operations below to work.
        // auto will yield a signed char here.
        const unsigned char c = *i;
        if ( ( c >= 32 && c <= 126 ) && (
                 ( c >= 'a' && c <= 'z' ) ||
                 ( c >= 'A' && c <= 'Z' ) ||
                 ( c >= '0' && c <= '9' ) ||
                 isSafe( c ) == true ||
                 ( extraChars != nullptr && strchr( extraChars, c ) != nullptr ) )
             )
        {
            res.push_back( c );
        }
        else
            res.append({ '%', "0123456789ABCDEF"[c >> 4], "0123456789ABCDEF"[c & 0xF] });
    }
    return end;
}

}

namespace medialibrary
{
namespace utils
{
namespace url
{

std::string decode( const std::string& str )
{
    std::string res;
    res.reserve( str.size() );
    auto it = str.cbegin();
    auto ite = str.cend();
    for ( ; it != ite; ++it )
    {
        if ( *it == '%' )
        {
            ++it;
            char hex[3];
            if ( ( hex[0] = *it) == 0 || ( hex[1] = *(it + 1) ) == 0 )
                throw std::runtime_error( str + ": Incomplete character sequence" );
            hex[2] = 0;
            auto val = strtoul( hex, nullptr, 16 );
            res.push_back( static_cast<std::string::value_type>( val ) );
            ++it;
        }
        else
            res.push_back( *it );
    }
    return res;
}

std::string encode( const std::string& str )
{
    std::string res;

    res.reserve( str.size() );
    auto schemePos = str.find( "://" );
    auto i = str.cbegin();
    if ( schemePos != std::string::npos )
    {
        i += schemePos + 3;
        std::copy( str.cbegin(), i, std::back_inserter( res ) );
        if ( str.compare( 0, schemePos, "file" ) != 0 )
        {
            /* We need to allow some special characters in the host segment */
            auto endHost = str.find( '/', schemePos + 3 );
            if ( endHost != std::string::npos )
                i = encodeSegment( res, i, str.cbegin() + endHost, "@:" );
            else
            {
                encodeSegment( res, i, str.cend(), "@:" );
                return res;
            }
        }
    }
#ifdef _WIN32
    if ( *i == '/' && isalpha( *(i + 1) ) && *(i + 2) == ':' )
    {
        // Don't encode the ':' after the drive letter.
        // All other ':' need to be encoded, there are not allowed in a file path
        // on windows, but they might appear in urls
        std::copy( i, i + 3, std::back_inserter( res ) );
        i += 3;
    }
#endif
    encodeSegment( res, i, str.end(), nullptr );
    return res;
}

}
}
}
