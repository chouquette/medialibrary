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
#include "Filename.h"
#include "medialibrary/filesystem/Errors.h"

#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <iterator>

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

inline uint16_t charToVal( char c )
{
    if ( c >= '0' && c <= '9' )
        return c - '0';
    if ( c >= 'a' && c <= 'f' )
        return c - 'a' + 10;
    if ( c >= 'A' && c <= 'F' )
        return c - 'A' + 10;
    throw std::runtime_error( "Incomplete/invalid character sequence" );
}

}

namespace medialibrary
{
namespace utils
{
namespace url
{

parts split( const std::string& url )
{
    parts res{};
    auto schemePos = url.find( "://" );
    if ( schemePos == std::string::npos )
    {
        res.path = url;
        return res;
    }
    std::copy( url.cbegin(), url.cbegin() + schemePos,
               std::back_inserter( res.scheme ) );


    if ( res.scheme == "file" )
    {
        std::copy( url.cbegin() + schemePos + 3, url.cend(),
                   std::back_inserter( res.path ) );
        return res;
    }

    const auto authorityBegin = url.cbegin() + schemePos + 3;
    auto authorityEnd = std::find( authorityBegin, url.cend(), '/' );
    auto queryBegin = std::find( authorityBegin, url.cend(), '?' );
    const auto fragmentBegin = std::find( authorityBegin, url.cend(), '#' );

    /*
     * The fragment must come after the query parameters. Since we use both
     * values at a later point, sanitize them now.
     */
    if ( fragmentBegin != url.cend() && queryBegin != url.cend() &&
         fragmentBegin < queryBegin )
    {
        queryBegin = url.cend();
    }

    /* RFC 3986 §3.2:
     * The authority component is preceded by a double slash ("//") and is
     * terminated by the next slash ("/"), question mark ("?"), or number
     * sign ("#") character, or by the end of the URI.
     */
    if ( authorityEnd == url.cend() )
    {
        if ( queryBegin != url.cend() )
            authorityEnd = queryBegin;
        else
            authorityEnd = fragmentBegin;
    }
    else
    {
        if ( queryBegin != url.cend() && queryBegin < authorityEnd )
            authorityEnd = queryBegin;
        if ( fragmentBegin != url.cend() && fragmentBegin < authorityEnd )
            authorityEnd = fragmentBegin;
    }

    {
        /* Split the authority into its actual components */
        auto userInfoEnd = std::find( authorityBegin, authorityEnd, '@' );
        if ( userInfoEnd != authorityEnd )
        {
            std::copy( authorityBegin, userInfoEnd,
                       std::back_inserter( res.userInfo ) );
            /* Skip the '@' when copying the host */
            ++userInfoEnd;
            res.hostMarker = true;
        }
        else
            userInfoEnd = authorityBegin;

        auto portBegin = std::find( userInfoEnd, authorityEnd, ':' );
        if ( portBegin != authorityEnd )
            std::copy( portBegin + 1, authorityEnd, std::back_inserter( res.port ) );
        else
            portBegin = authorityEnd;
        std::copy( userInfoEnd, portBegin, std::back_inserter( res.host ) );

    }
    if ( authorityEnd == url.cend() )
    {
        /*
         * If we don't have a clear end for the authority segment, it means the
         * end is the url end, so we don't have anything else to split
         */
        return res;
    }

    auto pathEnd = queryBegin != url.cend() ? queryBegin : fragmentBegin;
    std::copy( authorityEnd, pathEnd,
               std::back_inserter( res.path ) );
    if ( queryBegin != url.cend() )
    {
        std::copy( queryBegin + 1,
                   fragmentBegin != url.cend() ? fragmentBegin : url.cend(),
                   std::back_inserter( res.query ) );
    }
    if ( fragmentBegin != url.cend() )
    {
        std::copy( fragmentBegin + 1, url.cend(),
                   std::back_inserter( res.fragments ) );
    }
    return res;
}

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
            const auto c1 = charToVal( *it );
            const auto c2 = charToVal( *( it + 1 ) );
            auto val = c1 * 16 + c2;
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
    auto parts = split( str );
    std::string res{};
    res.reserve( str.length() );
    /*
     * If the file is a local path, we need to encode everything as the
     * characters won't have any URL related meaning
     */
    if ( parts.scheme == "file" || parts.scheme.empty() == true )
    {
        if ( parts.scheme.empty() == false )
            res += "file://";
        auto pathBegin = parts.path.cbegin();
#ifdef _WIN32
        if ( *pathBegin == '/' && isalpha( *(pathBegin + 1) ) && *(pathBegin + 2) == ':' )
        {
            // Don't encode the ':' after the drive letter.
            // All other ':' need to be encoded, there are not allowed in a file path
            // on windows, but they might appear in urls
            std::copy( pathBegin, pathBegin + 3, std::back_inserter( res ) );
            pathBegin += 3;
        }
#endif
        encodeSegment( res, pathBegin, parts.path.cend(), nullptr );
        return res;
    }
    /*
     * We already accept any character in ".-_~/" through isSafe(), but we need
     * to accept more depending on the URL segments:
     */
    encodeSegment( res, parts.scheme.cbegin(), parts.scheme.cend(), "+" );
    res += "://";
    if ( parts.userInfo.empty() == false )
    {
        encodeSegment( res, parts.userInfo.cbegin(), parts.userInfo.cend(),
                       "!$&'()*+,;=:" );
    }
    if ( parts.hostMarker == true )
        res += '@';
    encodeSegment( res, parts.host.cbegin(), parts.host.cend(), "[]" );
    if ( parts.port.empty() == false )
    {
        res += ':';
        res += parts.port;
    }

    encodeSegment( res, parts.path.cbegin(), parts.path.cend(), "!$&'()*+,;=:@" );

    if ( parts.query.empty() == false )
    {
        res += '?';
        encodeSegment( res, parts.query.cbegin(), parts.query.cend(),
                       "!$&'()*+,;=:@?" );
    }
    if ( parts.fragments.empty() == false )
    {
        res += '#';
        encodeSegment( res, parts.query.cbegin(), parts.query.cend(),
                       "!$&'()*+,;=:@?" );
    }
    return res;
}

std::string stripScheme( const std::string& mrl )
{
    auto pos = mrl.find( "://" );
    if ( pos == std::string::npos )
        throw fs::errors::UnhandledScheme( "<empty scheme>" );
    return mrl.substr( pos + 3 );
}

std::string scheme( const std::string& mrl )
{
    auto pos = mrl.find( "://" );
    if ( pos == std::string::npos )
        throw fs::errors::UnhandledScheme( "<empty scheme>" );
    return mrl.substr( 0, pos + 3 );
}

#ifndef _WIN32

std::string toLocalPath( const std::string& mrl )
{
    if ( mrl.compare( 0, 7, "file://" ) != 0 )
        throw fs::errors::UnhandledScheme( scheme( mrl ) );
    return utils::url::decode( mrl.substr( 7 ) );
}

#else

std::string toLocalPath( const std::string& mrl )
{
    if ( mrl.compare( 0, 7, "file://" ) != 0 )
        throw fs::errors::UnhandledScheme( utils::url::scheme( mrl ) );
    auto localPath = mrl.substr( 7 );
    // If the path is a local path (ie. X:\path\to and not \\path\to) skip the
    // initial backslash, as it is only part of our representation, and not
    // understood by the win32 API functions
    // Note that the initial '/' (after the 2 forward slashes from the scheme)
    // is not a backslash, as it is not a path separator, so do not use
    // DIR_SEPARATOR_CHAR here.
    if ( localPath[0] == '/' && isalpha( localPath[1] ) )
        localPath.erase( 0, 1 );
    std::replace( begin( localPath ), end( localPath ), '/', '\\' );
    return utils::url::decode( localPath );
}

#endif

bool schemeIs( const std::string& scheme, const std::string& mrl )
{
    return mrl.compare( 0, scheme.size(), scheme ) == 0;
}

std::string path( const std::string &mrl )
{
    auto schemelessMrl = stripScheme( mrl );
    auto host = utils::file::firstFolder( schemelessMrl );
    return utils::file::removePath( schemelessMrl, host );
}

}
}
}
