/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2017-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *          Alexandre Fernandez <nerf@boboop.fr>
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

#include "Directory.h"

#include <cstring>
#include <stdexcept>
#include <system_error>
#include "utils/Filename.h"
#include "logging/Logger.h"

#ifdef _WIN32
# include <windows.h>
#include "utils/Charsets.h"
#else
# include <cerrno>
# include <sys/types.h>
# include <sys/stat.h>
# include <limits.h>
# include <cstdlib>
#endif

namespace medialibrary
{

namespace utils
{

namespace fs
{

namespace
{
    const auto ERR_FS_OBJECT_ACCESS = "Error accessing file-system object at ";
}

bool isDirectory( const std::string& path )
{
#ifdef _WIN32
    auto wpath = charset::ToWide( path.c_str() );
    auto attr = GetFileAttributes( wpath.get() );
    if ( attr == INVALID_FILE_ATTRIBUTES )
    {
        DWORD errVal = GetLastError();
        std::error_code ec( errVal, std::system_category() );
        throw std::system_error( ec, ERR_FS_OBJECT_ACCESS + path );
    }
    return attr & FILE_ATTRIBUTE_DIRECTORY;
#else
    struct stat s;
    if ( lstat( path.c_str(), &s ) != 0 )
        throw std::system_error( errno, std::system_category(), ERR_FS_OBJECT_ACCESS + path );
    return S_ISDIR( s.st_mode );
#endif
}

std::string toAbsolute( const std::string& path )
{
#ifndef _WIN32
    char abs[PATH_MAX];
    if ( realpath( path.c_str(), abs ) == nullptr )
    {
        LOG_ERROR( "Failed to convert ", path, " to absolute path" );
        throw std::system_error( errno, std::generic_category(), "Failed to convert to absolute path" );
    }
    return file::toFolderPath( abs );
#else
    wchar_t buff[MAX_PATH];
    auto wpath = charset::ToWide( path.c_str() );
    if ( GetFullPathName( wpath.get(), MAX_PATH, buff, nullptr ) == 0 )
    {
        LOG_ERROR( "Failed to convert ", path, " to absolute path" );
        throw std::system_error( GetLastError(), std::generic_category(), "Failed to convert to absolute path" );
    }
    auto upath = charset::FromWide( buff );
    return file::toFolderPath( upath.get() );
#endif
}

}

}

}
