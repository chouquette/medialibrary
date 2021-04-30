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

#include "utils/Filename.h"
#include "utils/File.h"
#include "utils/Url.h"
#include "logging/Logger.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/Errors.h"

#ifdef _WIN32
# include <windows.h>
# include <direct.h>
#include "utils/Charsets.h"
#else
# include <cerrno>
# include <sys/stat.h>
# include <limits.h>
# include <cstdlib>
# include <unistd.h>
# include <dirent.h>
# include <sys/types.h>
# include <cstring>
#endif

namespace medialibrary
{

namespace errors = fs::errors;

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
        throw errors::System{ GetLastError(), ERR_FS_OBJECT_ACCESS + path };
    return attr & FILE_ATTRIBUTE_DIRECTORY;
#else
    struct stat s;
    if ( lstat( path.c_str(), &s ) != 0 )
        throw errors::System{ errno, ERR_FS_OBJECT_ACCESS + path };
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
        throw errors::System{ errno, "Failed to convert to absolute path" };
    }
    return file::toFolderPath( abs );
#else
    wchar_t buff[MAX_PATH];
    auto wpath = charset::ToWide( path.c_str() );
    if ( GetFullPathNameW( wpath.get(), MAX_PATH, buff, nullptr ) == 0 )
    {
        LOG_ERROR( "Failed to convert ", path, " to absolute path" );
        throw errors::System{ GetLastError(), "Failed to convert to absolute path" };
    }
    auto upath = charset::FromWide( buff );
    return file::toFolderPath( upath.get() );
#endif
}

bool mkdir( const std::string& path )
{
    auto paths = utils::file::splitPath( path, true );
#ifndef _WIN32
    std::string fullPath{ "/" };
#else
    std::string fullPath;
#endif
    while ( paths.empty() == false )
    {
        fullPath += paths.top();

#ifdef _WIN32
        // Don't try to create C: or various other drives
        if ( isalpha( fullPath[0] ) && fullPath[1] == ':' && fullPath.length() == 2 )
        {
            fullPath += "\\";
            paths.pop();
            continue;
        }
        auto wFullPath = charset::ToWide( fullPath.c_str() );
        if ( _wmkdir( wFullPath.get() ) != 0 )
#else
        if ( ::mkdir( fullPath.c_str(), S_IRWXU ) != 0 )
#endif
        {
            if ( errno != EEXIST )
                return false;
        }
        paths.pop();
#ifndef _WIN32
        fullPath += "/";
#else
        fullPath += "\\";
#endif
    }
    return true;
}

#ifndef _WIN32
bool rmdir( std::string path )
{
    path = utils::file::toFolderPath( path );

    std::unique_ptr<DIR, int(*)(DIR*)> dir{
        opendir( path.c_str() ), &closedir
    };
    if ( dir == nullptr )
        return false;

    for ( auto d = readdir( dir.get() );
          d != nullptr; d = readdir( dir.get() ) )
    {
        if ( !strcmp( d->d_name, "." ) ||
             !strcmp( d->d_name, ".." ) )
            continue;

        auto p = path + d->d_name;
        struct stat s;
        if ( lstat( p.c_str(), &s ) < 0 )
            return false;
        switch ( s.st_mode & S_IFMT )
        {
            case S_IFDIR:
                if ( rmdir( p ) == false )
                    return false;
                break;
            case S_IFLNK:
            case S_IFREG:
                unlink( p.c_str() );
                break;
            default:
                LOG_WARN( "Unhandled file type during folder removal: ", p );
                break;
        }
    }
    ::rmdir( path.c_str() );
    return true;
}

#else

bool rmdir( std::string path )
{
    path = utils::file::toFolderPath( path );
    LOG_ERROR( "Removing directory ", path );
    WIN32_FIND_DATA f;
    auto pattern = path + '*';
    auto wpattern = charset::ToWide( pattern.c_str() );
    auto h = FindFirstFile( wpattern.get(), &f );
    if ( h == INVALID_HANDLE_VALUE )
    {
        return false;
    }
    do
    {
        auto file = charset::FromWide( f.cFileName );
        if ( strcmp( file.get(), "." ) == 0 ||
             strcmp( file.get(), ".." ) == 0 )
            continue;
        auto fullpath = path + file.get();
        if ( ( f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
        {
            if ( rmdir( fullpath ) == false )
                return false;
        }
        else
        {
            LOG_ERROR( "Removing file ", fullpath );
            auto res = _unlink( fullpath.c_str() );
            LOG_ERROR( "Removal result: ", res, " errno: ", errno );
        }
    } while ( FindNextFile( h, &f ) != 0 );
    auto wPath = charset::ToWide( path.c_str() );
    auto res = _wrmdir( wPath.get() );
    if ( res == 0 )
        return true;
    return errno == ENOENT;
}

#endif

}

}

}
