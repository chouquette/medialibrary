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
#include "utils/Filename.h"
#include "utils/Url.h"
#include "factory/IFileSystem.h"
#include "File.h"
#include "logging/Logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>
#include <winapifamily.h>

namespace medialibrary
{

namespace fs
{

Directory::Directory( const std::string& mrl , factory::IFileSystem& fsFactory )
    : CommonDirectory( fsFactory )
{
    m_path = utils::file::toFolderPath( toAbsolute( utils::file::toLocalPath( mrl ) ) );
    assert( *m_path.crbegin() == '/' );
    m_mrl = utils::file::toMrl( m_path );
}

const std::string& Directory::mrl() const
{
    return m_mrl;
}

void Directory::read() const
{
#if WINAPI_FAMILY_PARTITION (WINAPI_PARTITION_DESKTOP)
    WIN32_FIND_DATA f;
    auto pattern = m_path + '*';
    auto wpattern = charset::ToWide( pattern.c_str() );
    auto h = FindFirstFile( wpattern.get(), &f );
    if ( h == INVALID_HANDLE_VALUE )
    {
        LOG_ERROR( "Failed to browse ", m_path );
        throw std::system_error( GetLastError(), std::generic_category(), "Failed to browse through directory" );
    }
    do
    {
        auto file = charset::FromWide( f.cFileName );
        if ( file[0] == '.' && strcasecmp( file.get(), ".nomedia" ) )
            continue;
        auto fullpath = m_path + file.get();
        if ( ( f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
            m_dirs.emplace_back( m_fsFactory.createDirectory( m_path +
                                        utils::url::encode( fullpath ) ) );
        else
            m_files.emplace_back( std::make_shared<File>( fullpath ) );
    } while ( FindNextFile( h, &f ) != 0 );
    FindClose( h );
#else
    // We must remove the trailing /
    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
    // «Do not use a trailing backslash (\), which indicates the root directory of a drive»
    auto tmpPath = path.substr( 0, m_path.length() - 1 );
    auto wpath = charset::ToWide( tmpPath.c_str() );

    CREATEFILE2_EXTENDED_PARAMETERS params{};
    params.dwFileFlags = FILE_FLAG_BACKUP_SEMANTICS;
    auto handle = CreateFile2( wpath.get(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &params );
    if ( handle == INVALID_HANDLE_VALUE )
    {
        LOG_ERROR( "Failed to open directory ", m_path );
        throw std::system_error( GetLastError(), std::generic_category(), "Failed to open directory" );
    }

    std::unique_ptr<typename std::remove_pointer<HANDLE>::type,
            decltype(&CloseHandle)> handlePtr( handle, &CloseHandle );

    // Allocating a 32 bytes buffer to contain the file name. If more is required, we'll allocate
    size_t buffSize = sizeof( FILE_FULL_DIR_INFO ) + 32;
    std::unique_ptr<FILE_FULL_DIR_INFO, void(*)(FILE_FULL_DIR_INFO*)> dirInfo(
                reinterpret_cast<FILE_FULL_DIR_INFO*>( malloc( buffSize ) ),
                [](FILE_FULL_DIR_INFO* ptr) { free( ptr ); } );
    if ( dirInfo == nullptr )
        throw std::bad_alloc();

    while ( true )
    {
        auto h = GetFileInformationByHandleEx( handle, FileFullDirectoryInfo, dirInfo.get(), buffSize );
        if ( h == 0 )
        {
            auto error = GetLastError();
            if ( error == ERROR_FILE_NOT_FOUND )
                break;
            else if ( error == ERROR_MORE_DATA )
            {
                buffSize *= 2;
                dirInfo.reset( reinterpret_cast<FILE_FULL_DIR_INFO*>( malloc( buffSize ) ) );
                if ( dirInfo == nullptr )
                    throw std::bad_alloc();
                continue;
            }
            LOG_ERROR( "Failed to browse ", m_path, ". GetLastError(): ", GetLastError() );
            throw std::system_error( GetLastError(), std::generic_category(), "Failed to browse through directory" );
        }

        auto file = charset::FromWide( dirInfo->FileName );
        if ( file[0] == '.' && strcasecmp( file.get(), ".nomedia" ) )
            continue;
        if ( ( dirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
            m_dirs.emplace_back( m_fsFactory.createDirectory( m_mrl + utils::url::encode( file.get() ) ) );
        else
            m_files.emplace_back( std::make_shared<File>( m_path + file.get()) );
    }
#endif
}

std::string Directory::toAbsolute( const std::string& path )
{
    TCHAR buff[MAX_PATH];
    auto wpath = charset::ToWide( path.c_str() );
    if ( GetFullPathName( wpath.get(), MAX_PATH, buff, nullptr ) == 0 )
    {
        LOG_ERROR( "Failed to convert ", path, " to absolute path" );
        throw std::system_error( GetLastError(), std::generic_category(), "Failed to convert to absolute path" );
    }
    auto upath = charset::FromWide( buff );
    return std::string( upath.get() );
}

}

}
