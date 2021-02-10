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

#ifndef LOCKFILE_H
#define LOCKFILE_H

#ifdef _WIN32
# include <windows.h>
#endif

#include <memory>
#include <string>

namespace medialibrary
{

class LockFile
{
#ifdef _WIN32
    using Handle = HANDLE;
# define NO_HANDLE INVALID_HANDLE_VALUE
#else
    using Handle = int;
# define NO_HANDLE -1
#endif

public:
    static std::unique_ptr<LockFile> lock( const std::string& mlFolderPath );
    ~LockFile();

private:
    LockFile( Handle handle );
    void unlock();

    Handle m_handle;
};

}

#endif
