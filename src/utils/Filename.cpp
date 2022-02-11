/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "utils/Filename.h"
#include "utils/Url.h"
#include "medialibrary/filesystem/Errors.h"

#ifdef _WIN32
# include <algorithm>
# define DIR_SEPARATOR "\\/"
#else
# define DIR_SEPARATOR '/'
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

std::string stripExtension( const std::string& fileName )
{
    auto pos = fileName.find_last_of( '.' );
    if ( pos == std::string::npos )
        return fileName;
    return fileName.substr( 0, pos );
}

std::string directory( const std::string& filePath )
{
    auto pos = filePath.find_last_of( DIR_SEPARATOR );
    if ( pos == std::string::npos )
        return {};
    return filePath.substr( 0, pos + 1 );
}

std::string directoryName( const std::string& directoryPath )
{
    auto pos = directoryPath.find_last_of( DIR_SEPARATOR );
    if ( pos == std::string::npos )
        return directoryPath;
    if ( pos == 0 )
        return directoryPath.substr( 1 );
    if ( pos != directoryPath.size() - 1 )
        return directoryPath.substr( pos + 1 );
    auto res = directoryPath;
    res.pop_back();
    pos = res.find_last_of( DIR_SEPARATOR );
    return res.substr( pos + 1 );
}

std::string parentDirectory( const std::string& path )
{
    auto pos = path.find_last_of( DIR_SEPARATOR );
    if ( pos == path.length() - 1 )
        pos = path.find_last_of( DIR_SEPARATOR, pos - 1 );
    if ( pos == std::string::npos )
        return {};
    return path.substr( 0, pos + 1 );
}

std::string fileName(const std::string& filePath)
{
    auto pos = filePath.find_last_of( DIR_SEPARATOR );
    if ( pos == std::string::npos )
        return filePath;
    return filePath.substr( pos + 1 );
}

std::string firstFolder( const std::string& path )
{
    size_t offset = path.find_first_not_of( DIR_SEPARATOR );
    auto pos = path.find_first_of( DIR_SEPARATOR, offset );
    if ( pos == std::string::npos )
        return {};
    return path.substr( offset, pos - offset );
}

std::string removePath( const std::string& fullPath, const std::string& toRemove )
{
    if ( toRemove.length() == 0 || toRemove.length() > fullPath.length() )
        return fullPath;
    auto pos = fullPath.find( toRemove );
    if ( pos == std::string::npos )
        return fullPath;
    pos += toRemove.length();
    // Skip over potentially duplicated DIR_SEPARATOR
    while ( pos < fullPath.length() &&
            ( fullPath[pos] == '/'
#ifdef _WIN32
                || fullPath[pos] == '\\'
#endif
            ) )
        pos++;
    if ( pos >= fullPath.length() )
        return {};
    return fullPath.substr( pos );
}

std::string toFolderPath( std::string path )
{
#ifdef _WIN32
    if ( *path.crbegin() == '\\' )
        *path.rbegin() = '/';
    else
#endif
    if ( *path.crbegin() != '/' )
        path += '/';
    return path;
}

#ifndef _WIN32

std::string toMrl( const std::string& path )
{
    return "file://" + utils::url::encode( path );
}

#else

std::string toMrl( const std::string& path )
{
    auto normalized = path;
    std::replace( begin( normalized ), end( normalized ), '\\', '/' );
    if ( isalpha( normalized[0] ) )
        normalized = "/" + normalized;
    return std::string{ "file://" } +
            utils::url::encode( normalized );
}

#endif

std::stack<std::string> splitPath( const std::string& path, bool isDirectory )
{
    std::stack<std::string> res;
    std::string currPath = isDirectory ? utils::file::toFolderPath( path )
                                       : utils::file::directory( path );
    auto ff = utils::file::firstFolder( path );
    if ( isDirectory == false )
        res.push( utils::file::fileName( path ) );
    do
    {
        res.push( utils::file::directoryName( currPath ) );
        currPath = utils::file::parentDirectory( currPath );
    } while ( res.top() != ff );
    return res;
}

}

}

}
