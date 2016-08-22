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
    auto pos = filePath.find_last_of( '/' );
    if ( pos == std::string::npos )
        return {};
    return filePath.substr( 0, pos + 1 );
}

std::string parentDirectory( const std::string& path )
{
    auto pos = path.find_last_of( '/' );
    if ( pos == path.length() - 1 )
        pos = path.find_last_of( '/', pos - 1 );
    return path.substr( 0, pos + 1 );
}

std::string fileName(const std::string& filePath)
{
    auto pos = filePath.find_last_of( '/' );
    if ( pos == std::string::npos )
        return filePath;
    return filePath.substr( pos + 1 );
}

std::string firstFolder( const std::string& path )
{
    size_t offset = 0;
    while ( path[offset] == '/' )
        offset++;
    auto pos = path.find_first_of( '/', offset );
    if ( pos == std::string::npos )
        return {};
    return path.substr( offset, pos - offset );
}

std::string removePath( const std::string& fullPath, const std::string& toRemove )
{
    if ( toRemove.length() == 0 )
        return fullPath;
    auto pos = fullPath.find( toRemove ) + toRemove.length();
    while ( fullPath[pos] == '/' )
        pos++;
    if ( pos >= fullPath.length() )
        return {};
    return fullPath.substr( pos );
}

std::string& toFolderPath( std::string& path )
{
    if ( *path.crbegin() != DIR_SEPARATOR )
        path += DIR_SEPARATOR;
    return path;
}

std::string toFolderPath( const std::string& path )
{
    auto p = path;
    if ( *p.crbegin() != DIR_SEPARATOR )
        p += DIR_SEPARATOR;
    return p;
}

}

}

}
