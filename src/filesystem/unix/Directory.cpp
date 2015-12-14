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

#include "Directory.h"
#include "Media.h"
#include "Mountpoint.h"

#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs
{

Directory::Directory( const std::string& path )
    : m_path( toAbsolute( path ) )
    , m_lastModificationDate( 0 )
{
}

const std::string&Directory::path() const
{
    return m_path;
}

const std::vector<std::string>& Directory::files()
{
    if ( m_dirs.size() == 0 && m_files.size() == 0 )
        read();
    return m_files;
}

const std::vector<std::string>&Directory::dirs()
{
    if ( m_dirs.size() == 0 && m_files.size() == 0 )
        read();
    return m_dirs;
}

unsigned int Directory::lastModificationDate() const
{
    if ( m_lastModificationDate == 0 )
    {
        struct stat s;
        lstat( m_path.c_str(), &s );
        if ( S_ISDIR( s.st_mode ) == false )
            throw std::runtime_error( "The provided path isn't a directory" );
        m_lastModificationDate = s.st_mtim.tv_sec;
    }
    return m_lastModificationDate;
}

std::shared_ptr<IMountpoint> Directory::mountpoint() const
{
    //FIXME: Cache this?
    return Mountpoint::fromPath( m_path );
}

bool Directory::isRemovable() const
{
    //FIXME
    return false;
}

std::string Directory::toAbsolute(const std::string& path)
{
    auto abs = std::unique_ptr<char[]>( new char[PATH_MAX] );
    if ( realpath( path.c_str(), abs.get() ) == nullptr )
    {
        std::string err( "Failed to convert to absolute path" );
        err += "(" + path + "): ";
        err += strerror(errno);
        throw std::runtime_error( err );
    }
    return std::string{ abs.get() };
}

void Directory::read()
{
    auto dir = std::unique_ptr<DIR, int(*)(DIR*)>( opendir( m_path.c_str() ), closedir );
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

#if defined(_DIRENT_HAVE_D_TYPE) && defined(_BSD_SOURCE)
        if ( result->d_type == DT_DIR )
        {
#else
        struct stat s;
        if ( lstat( path.c_str(), &s ) != 0 )
        {
            if ( errno == EACCES )
                continue;
            std::string err( "Failed to get file info " );
            err += path + ": ";
            err += strerror(errno);
            throw std::runtime_error( err );
        }
        if ( S_ISDIR( s.st_mode ) )
        {
#endif
            m_dirs.emplace_back( toAbsolute( path ) );
        }
        else
        {
            m_files.emplace_back( toAbsolute( path ) );
        }
    }
}

}
