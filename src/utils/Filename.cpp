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

#include "utils/Filename.h"
#include "utils/Url.h"

#include <stdexcept>

#ifdef _WIN32
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

namespace medialibrary
{

namespace utils
{

namespace file
{

std::string extension( const std::string& fileName )
{
    auto pos = fileName.find_last_of( '.' );
    if ( pos == std::string::npos )
        return {};
    return fileName.substr( pos + 1 );
}

std::string directory( const std::string& filePath )
{
    auto pos = filePath.find_last_of( DIR_SEPARATOR );
    if ( pos == std::string::npos )
#ifdef _WIN32
    {
        pos = filePath.find_last_of( '/' );
        if ( pos == std::string::npos )
            return {};
    }
#else
        return {};
#endif
    return filePath.substr( 0, pos + 1 );
}

std::string parentDirectory( const std::string& path )
{
    auto pos = path.find_last_of( DIR_SEPARATOR );
#ifdef _WIN32
    // If the \ appears right after the drive letter, or there's no backward /
    if ( ( pos == 2 && path[1] == ':' ) || pos == std::string::npos )
    {
        // Fall back to forward slashes
        pos = path.find_last_of( '/' );
    }
#endif
    if ( pos == path.length() - 1 )
    {
#ifdef _WIN32
        auto oldPos = pos;
#endif
        pos = path.find_last_of( DIR_SEPARATOR, pos - 1 );
#ifdef _WIN32
        if ( ( pos == 2 && path[1] == ':' ) || pos == std::string::npos )
            pos = path.find_last_of( '/', oldPos - 1 );
#endif
    }
    return path.substr( 0, pos + 1 );
}

std::string fileName(const std::string& filePath)
{
    auto pos = filePath.find_last_of( DIR_SEPARATOR );
#ifdef _WIN32
    if ( pos == std::string::npos )
        pos = filePath.find_last_of( '/' );
#endif
    if ( pos == std::string::npos )
        return filePath;
    return filePath.substr( pos + 1 );
}

std::string firstFolder( const std::string& path )
{
    size_t offset = 0;
    while ( path[offset] == DIR_SEPARATOR
#ifdef _WIN32
            || path[offset] == '/'
#endif
            )
        offset++;
    auto pos = path.find_first_of( DIR_SEPARATOR, offset );
#ifdef _WIN32
    if ( pos == std::string::npos )
        pos = path.find_first_of( '/', offset );
#endif
    if ( pos == std::string::npos )
        return {};
    return path.substr( offset, pos - offset );
}

std::string removePath( const std::string& fullPath, const std::string& toRemove )
{
    if ( toRemove.length() == 0 )
        return fullPath;
    auto pos = fullPath.find( toRemove ) + toRemove.length();
    while ( fullPath[pos] == DIR_SEPARATOR
#ifdef _WIN32
                || fullPath[pos] == '/'
#endif
            )
        pos++;
    if ( pos >= fullPath.length() )
        return {};
    return fullPath.substr( pos );
}

std::string& toFolderPath( std::string& path )
{
    if ( *path.crbegin() != DIR_SEPARATOR
#ifdef _WIN32
            && *path.crbegin() != '/'
#endif
         )
        path += DIR_SEPARATOR;
    return path;
}

std::string toFolderPath( const std::string& path )
{
    auto p = path;
    if ( *p.crbegin() != DIR_SEPARATOR
#ifdef _WIN32
            && *p.crbegin() != '/'
#endif
         )
        p += DIR_SEPARATOR;
    return p;
}

std::string toLocalPath( const std::string& mrl )
{
    if ( mrl.compare( 0, 7, "file://" ) != 0 )
        throw std::runtime_error( mrl + " is not representing a local path" );
    return utils::url::decode( mrl.substr( 7 ) );
}

std::string toPath( const std::string& mrl )
{
    return utils::url::decode( stripScheme( mrl ) );
}

std::string stripScheme( const std::string& mrl )
{
    auto pos = mrl.find( "://" );
    return pos == std::string::npos ? mrl : mrl.substr( pos + 3 );
}

std::string toMrl( const std::string& path )
{
    return "file://" + utils::url::encode( path );
}

std::string scheme( const std::string& mrl )
{
    auto pos = mrl.find( "://" );
    if ( pos == std::string::npos )
        throw std::runtime_error( "Invalid MRL provided" );
    return mrl.substr( 0, pos + 3 );
}

std::stack<std::string> splitPath( const std::string& path, bool isDirectory )
{
    std::stack<std::string> res;
    std::string currPath = isDirectory ? utils::file::toFolderPath( path )
                                       : utils::file::directory( path );
    auto firstFolder = utils::file::firstFolder( path );
    if ( isDirectory == false )
        res.push( utils::file::fileName( path ) );
    do
    {
        res.push( utils::file::directoryName( currPath ) );
        currPath = utils::file::parentDirectory( currPath );
    } while ( res.top() != firstFolder );
    return res;
}

bool schemeIs( const std::string& scheme, const std::string& mrl )
{
    return mrl.compare( 0, scheme.size(), scheme ) == 0;
}

}

}

}