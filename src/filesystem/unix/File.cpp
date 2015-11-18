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

#include "File.h"

#include <stdexcept>
#include <sys/stat.h>

namespace fs
{

File::File( const std::string& filePath )
    : CommonFile( filePath )
{
    struct stat s;
    if ( lstat( m_fullPath.c_str(), &s ) )
        throw std::runtime_error( "Failed to get file stats" );
    m_lastModificationDate = s.st_mtim.tv_sec;
}

unsigned int File::lastModificationDate() const
{
    return m_lastModificationDate;
}

}
