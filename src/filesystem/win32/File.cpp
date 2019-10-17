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

#include "File.h"
#include "logging/Logger.h"
#include "utils/Charsets.h"
#include "utils/Filename.h"
#include "medialibrary/filesystem/Errors.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <stdexcept>

#include <windows.h>

namespace medialibrary
{

namespace fs
{

File::File( const std::string& filePath )
    : CommonFile( utils::file::toMrl( filePath ) )
{
    struct _stat64 s;
    if ( _wstat64( charset::ToWide( filePath.c_str() ).get(), &s ) != 0 )
    {
        LOG_ERROR( "Failed to get ", filePath, " stats" );
        throw errors::System{ errno, "Failed to get stats" };
    }

    m_lastModificationDate = s.st_mtime;
    m_size = s.st_size;
}

unsigned int File::lastModificationDate() const
{
    return m_lastModificationDate;
}

int64_t File::size() const
{
    return m_size;
}

}

}
