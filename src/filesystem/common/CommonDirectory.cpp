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

#include "CommonDirectory.h"
#include "factory/IFileSystem.h"
#include "utils/Filename.h"
#include <dirent.h>
#include <cerrno>
#include <cstring>

namespace medialibrary
{
namespace fs
{

medialibrary::fs::CommonDirectory::CommonDirectory( const std::string& path,
                                                    factory::IFileSystem& fsFactory )
    : m_path( utils::file::toFolderPath( path ) )
    , m_fsFactory( fsFactory )
{
}

const std::string& CommonDirectory::path() const
{
    return m_path;
}

const std::vector<std::shared_ptr<IFile>>& CommonDirectory::files() const
{
    if ( m_dirs.size() == 0 && m_files.size() == 0 )
        read();
    return m_files;
}

const std::vector<std::shared_ptr<IDirectory>>& CommonDirectory::dirs() const
{
    if ( m_dirs.size() == 0 && m_files.size() == 0 )
        read();
    return m_dirs;
}

std::shared_ptr<IDevice> CommonDirectory::device() const
{
    auto lock = m_device.lock();
    if ( m_device.isCached() == false )
        m_device = m_fsFactory.createDeviceFromPath( m_path );
    return m_device.get();
}

}
}
