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

#include "Directory.h"
#include "Media.h"
#include "Device.h"
#include "filesystem/unix/File.h"

#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

namespace medialibrary
{

namespace fs
{

Directory::Directory( const std::string& path )
    : m_path( toAbsolute( path ) )
{
    if ( *m_path.crbegin() != '/' )
        m_path += '/';
}

const std::string&Directory::path() const
{
    return m_path;
}

const std::vector<std::shared_ptr<IFile>>& Directory::files() const
{
    if ( m_dirs.size() == 0 && m_files.size() == 0 )
        read();
    return m_files;
}

const std::vector<std::shared_ptr<IDirectory>>& Directory::dirs() const
{
    if ( m_dirs.size() == 0 && m_files.size() == 0 )
        read();
    return m_dirs;
}

std::shared_ptr<IDevice> Directory::device() const
{
    auto lock = m_device.lock();
    if ( m_device.isCached() == false )
        m_device = Device::fromPath( m_path );
    return m_device.get();
}

std::string Directory::toAbsolute( const std::string& path )
{
    char abs[PATH_MAX];
    if ( realpath( path.c_str(), abs ) == nullptr )
    {
        std::string err( "Failed to convert to absolute path" );
        err += "(" + path + "): ";
        err += strerror(errno);
        throw std::runtime_error( err );
    }
    return std::string{ abs };
}

void Directory::read() const
{
    std::unique_ptr<DIR, int(*)(DIR*)> dir( opendir( m_path.c_str() ), closedir );
    if ( dir == nullptr )
    {
        std::string err( "Failed to open directory " );
        err += m_path;
        err += strerror(errno);
        throw std::runtime_error( err );
    }

    dirent* result = nullptr;

    while ( ( result = readdir( dir.get() ) ) != nullptr )
    {
        if ( strcmp( result->d_name, "." ) == 0 ||
             strcmp( result->d_name, ".." ) == 0 )
        {
            continue;
        }
        std::string path = m_path + "/" + result->d_name;

        struct stat s;
        if ( lstat( path.c_str(), &s ) != 0 )
        {
            if ( errno == EACCES )
                continue;
            // Ignore EOVERFLOW since we are not (yet?) interested in the file size
            if ( errno != EOVERFLOW )
            {
                std::string err( "Failed to get file info " );
                err += path + ": ";
                err += strerror(errno);
                throw std::runtime_error( err );
            }
        }
        if ( S_ISDIR( s.st_mode ) )
        {
            auto dirPath = toAbsolute( path );
            if ( *dirPath.crbegin() != '/' )
                dirPath += '/';
            //FIXME: This will use toAbsolute again in the constructor.
            m_dirs.emplace_back( std::make_shared<Directory>( dirPath ) );
        }
        else
        {
            auto filePath = toAbsolute( path );
            m_files.emplace_back( std::make_shared<File>( filePath, s ) );
        }
    }
}

}

}
