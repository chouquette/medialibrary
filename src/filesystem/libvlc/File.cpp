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
#include "config.h"
#endif

#ifndef HAVE_LIBVLC
# error This file requires libvlc
#endif

#include "File.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "utils/Filename.h"
#include "medialibrary/filesystem/Errors.h"

namespace medialibrary
{
namespace fs
{
namespace libvlc
{

File::File( std::string mrl, fs::IFileSystemFactory& fsFactory,
            uint32_t lastModificationDate, int64_t size )
    : CommonFile( std::move( mrl ) )
    , m_lastModificationDate( lastModificationDate )
    , m_size( size )
    , m_isNetwork( fsFactory.isNetworkFileSystem() )
{

}

unsigned int File::lastModificationDate() const
{
    return m_lastModificationDate;
}

int64_t File::size() const
{
    return m_size;
}

bool File::isNetwork() const
{
    return m_isNetwork;
}

}
}
}
