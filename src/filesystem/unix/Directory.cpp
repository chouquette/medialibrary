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
#include "Media.h"
#include "Device.h"
#include "filesystem/unix/File.h"
#include "logging/Logger.h"
#include "utils/Filename.h"
#include "utils/Directory.h"
#include "utils/Url.h"
#include "medialibrary/filesystem/Errors.h"

#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace medialibrary
{

namespace fs
{

Directory::Directory( const std::string& mrl, fs::IFileSystemFactory& fsFactory )
    : CommonDirectory( fsFactory )
{
    m_path = utils::file::toFolderPath(
                utils::fs::toAbsolute( utils::file::toLocalPath( mrl ) ) );
    assert( *m_path.crbegin() == '/' );
    m_mrl = utils::file::toMrl( m_path );
}

const std::string& Directory::mrl() const
{
    return m_mrl;
}

void Directory::read() const
{
    std::unique_ptr<DIR, int(*)(DIR*)> dir( opendir( m_path.c_str() ), closedir );
    if ( dir == nullptr )
    {
        LOG_ERROR( "Failed to open directory ", m_path );
        throw errors::System( errno, "Failed to open directory" );
    }

    dirent* result = nullptr;

    while ( ( result = readdir( dir.get() ) ) != nullptr )
    {
        if ( result->d_name[0] == '.' && strcasecmp( result->d_name, ".nomedia" ) != 0 )
            continue;

        std::string path = m_path + result->d_name;

        struct stat s;
        if ( lstat( path.c_str(), &s ) != 0 )
        {
            if ( errno == EACCES )
                continue;
            // some Android devices will list folder content, but will yield
            // ENOENT when accessing those.
            // See https://trac.videolan.org/vlc/ticket/19909
            if ( errno == ENOENT )
            {
                LOG_WARN( "Ignoring unexpected ENOENT while listing folder content." );
                continue;
            }
            LOG_ERROR( "Failed to get file ", path, " info" );
            throw errors::System{ errno, "Failed to get file info" };
        }
        try
        {
            if ( S_ISDIR( s.st_mode ) )
                m_dirs.emplace_back( std::make_shared<Directory>(
                            m_mrl + utils::url::encode( result->d_name ), m_fsFactory ) );
            else
                m_files.emplace_back( std::make_shared<File>( path, s ) );
        }
        catch ( const errors::System& err )
        {
            if ( err.code() == std::errc::no_such_file_or_directory )
            {
                LOG_WARN( "Ignoring ", path, ": ", err.what() );
                continue;
            }
            LOG_ERROR( "Fatal error while reading ", path, ": ", err.what() );
            throw;
        }
    }
}

}

}
