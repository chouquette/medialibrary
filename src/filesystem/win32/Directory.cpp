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
#include "utils/Charsets.h"
#include "factory/IFileSystem.h"
#include "File.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>

namespace medialibrary
{

namespace fs
{

Directory::Directory(const std::string& path , factory::IFileSystem& fsFactory)
    : CommonDirectory( toAbsolute( path ), fsFactory )
{
}

void Directory::read() const
{
    WIN32_FIND_DATA f;
    auto pattern = m_path + '*';
    auto wpattern = charset::ToWide( pattern.c_str() );
    auto h = FindFirstFile( wpattern.get(), &f );
    if ( h == INVALID_HANDLE_VALUE )
    {
        LOG_ERROR( "Failed to browse ", m_path );
        std::system_error( GetLastError(), std::generic_category(), "Failed to browse through directory" );
    }
    do
    {
        auto file = charset::FromWide( f.cFileName );
        if ( file[0] == '.' )
            continue;
        auto path = m_path + file.get();
        if ( ( f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
            m_dirs.emplace_back( m_fsFactory.createDirectory( path ) );
        else
            m_files.emplace_back( std::make_shared<File>( path ) );
    } while ( FindNextFile( h, &f ) != 0 );
    FindClose( h );
}

std::string Directory::toAbsolute( const std::string& path )
{
    TCHAR buff[MAX_PATH];
    auto wpath = charset::ToWide( path.c_str() );
    if ( GetFullPathName( wpath.get(), MAX_PATH, buff, nullptr ) == 0 )
    {
        LOG_ERROR( "Failed to convert ", path, " to absolute path" );
        std::system_error( GetLastError(), std::generic_category(), "Failed to convert to absolute path" );
    }
    auto upath = charset::FromWide( buff );
    return std::string( upath.get() );
}

}

}
