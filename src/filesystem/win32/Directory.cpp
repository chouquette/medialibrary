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

#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>

namespace fs
{

Directory::Directory( const std::string& path )
    : m_path( toAbsolute( path ) )
    , m_lastModificationDate( 0 )
{
}

const std::string& Directory::path() const
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
        struct _stat32 s;
        _stat32( m_path.c_str(), &s );
        if ( ( S_IFDIR & s.st_mode ) == 0 )
            throw std::runtime_error( "The provided path isn't a directory" );
        m_lastModificationDate = s.st_mtime;
    }
    return m_lastModificationDate;
}

bool Directory::isRemovable() const
{
    return false;
}

void Directory::read()
{
    WIN32_FIND_DATA f;
    auto h = FindFirstFile( m_path.c_str(), &f );
    if ( h == INVALID_HANDLE_VALUE )
        throw std::runtime_error( "Failed to browse through " + m_path );
    try
    {
        do
        {
            if ( ( f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
            {
                m_dirs.emplace_back( f.cFileName );
            }
            else
            {
                m_dirs.emplace_back( f.cFileName );
            }
        } while ( FindNextFile( h, &f ) != 0 );
    }
    catch(...){}

    FindClose( h );
}



std::string Directory::toAbsolute( const std::string& path )
{
    TCHAR buff[MAX_PATH];
    if ( GetFullPathName( path.c_str(), MAX_PATH, buff, nullptr ) == 0 )
    {
        throw std::runtime_error("Failed to convert " + path + " to absolute path (" +
                                 std::to_string( GetLastError() ) + ")" );
    }
    return std::string( buff );
}

}
