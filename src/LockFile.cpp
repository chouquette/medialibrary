/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2021 Videolabs, VideoLAN
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

#include <cassert>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "LockFile.h"
#include "logging/Logger.h"
#include "utils/Charsets.h"
#include "utils/Directory.h"
#include "utils/Filename.h"

namespace medialibrary
{

std::unique_ptr<LockFile> LockFile::lock( const std::string& mlFolderPath )
{
    auto dir = utils::file::toFolderPath( mlFolderPath );
    if ( utils::fs::mkdir( dir ) == false )
    {
        LOG_ERROR( "Could not create ml folder path: ", dir );
        return {};
    }

    auto lockFile = dir + "ml.lock";
    Handle handle;
#ifdef _WIN32
    auto wide = charset::ToWide( lockFile.c_str() );
    handle = CreateFileW(wide.get(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if ( handle == INVALID_HANDLE_VALUE )
    {
        LOG_ERROR( "Could not open lockfile: ", lockFile );
        return {};
    }
#else
    handle = ::open( lockFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
    if ( handle == -1 )
    {
        LOG_ERROR( "Could not open lock file: ", lockFile );
        return {};
    }

    int ret = ::flock( handle, LOCK_EX | LOCK_NB );
    if ( ret == -1 )
    {
        LOG_ERROR( "Could not lock medialibrary (", lockFile, "), ",
                   "another process is probably using it." );
        return {};
    }
#endif
    return std::unique_ptr<LockFile>( new LockFile( handle ) );
}

void LockFile::unlock()
{
#ifdef _WIN32
    CloseHandle( m_handle );
#else
    ::flock( m_handle, LOCK_UN );
    ::close( m_handle );
#endif
}

LockFile::LockFile( Handle handle )
    : m_handle ( handle )
{
    assert( handle != NO_HANDLE );
}

LockFile::~LockFile()
{
    assert( m_handle != NO_HANDLE );
    unlock();
}

}
