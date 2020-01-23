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

#include "Directory.h"
#include "utils/Charsets.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "utils/Directory.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "medialibrary/filesystem/Errors.h"
#include "File.h"
#include "logging/Logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <cassert>
#include <windows.h>
#include <winapifamily.h>

namespace medialibrary
{

namespace fs
{

Directory::Directory( const std::string& mrl , fs::IFileSystemFactory& fsFactory )
    : CommonDirectory( fsFactory )
    , m_path( utils::file::toFolderPath(
                  utils::fs::toAbsolute( utils::file::toLocalPath( mrl ) ) ) )
    , m_mrl( utils::file::toMrl( m_path ) )
{
    assert( *m_path.crbegin() == '/' || *m_path.crbegin() == '\\' );
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
        throw errors::System( GetLastError(), "Failed to browse through directory" );
    }
    do
    {
        auto file = charset::FromWide( f.cFileName );
        if ( ( ( f.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) != 0 &&
                    strcasecmp( file.get(), ".nomedia" ) != 0 ) ||
              strcmp( file.get(), "." ) == 0 ||
              strcmp( file.get(), ".." ) == 0 )
            continue;
        auto fullpath = m_path + file.get();
        try
        {
            if ( ( f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
                m_dirs.emplace_back( m_fsFactory.createDirectory(
                                         m_mrl + utils::url::encode( file.get() ) ) );
            else
                m_files.emplace_back( std::make_shared<File>( fullpath ) );
        }
        catch ( const errors::System& ex )
        {
            LOG_WARN( "Failed to access a listed file/dir: ", ex.what() ,
                      ". Ignoring this entry." );
        }
    } while ( FindNextFile( h, &f ) != 0 );
    FindClose( h );
#else
    // We must remove the trailing /
    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
    // «Do not use a trailing backslash (\), which indicates the root directory of a drive»
    auto tmpPath = m_path.substr( 0, m_path.length() - 1 );
    auto wpath = charset::ToWide( tmpPath.c_str() );

    CREATEFILE2_EXTENDED_PARAMETERS params{};
    params.dwFileFlags = FILE_FLAG_BACKUP_SEMANTICS;
    auto handle = CreateFile2( wpath.get(), GENERIC_READ, FILE_SHARE_READ,
                               OPEN_EXISTING, &params );
    if ( handle == INVALID_HANDLE_VALUE )
    {
        LOG_ERROR( "Failed to open directory ", m_path );
        throw errors::System{ GetLastError(), "Failed to open directory" };
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
        auto h = GetFileInformationByHandleEx( handle, FileFullDirectoryInfo,
                                               dirInfo.get(), buffSize );
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
            LOG_ERROR( "Failed to browse ", m_path, ". GetLastError(): ",
                       GetLastError() );
            throw errors::System{ GetLastError(), "Failed to browse through directory" };
        }

        auto file = charset::FromWide( dirInfo->FileName );
        if ( ( dirInfo->FileAttributes & FILE_ATTRIBUTE_HIDDEN ) != 0 &&
                strcasecmp( file.get(), ".nomedia" ) ||
             strcmp( file.get(), "." ) == 0 ||
             strcmp( file.get(), ".." ) == 0 )
            continue;
        try
        {
            if ( ( dirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
                m_dirs.emplace_back( m_fsFactory.createDirectory(
                                         m_path + utils::url::encode( file.get() ) ) );
            else
                m_files.emplace_back( std::make_shared<File>( m_path + file.get()) );
        }
        catch ( const errors::System& ex )
        {
            LOG_WARN( "Failed to access a listed file/dir: ", ex.what() ,
                      ". Ignoring this entry." );
        }
    }
#endif
}

}

}
