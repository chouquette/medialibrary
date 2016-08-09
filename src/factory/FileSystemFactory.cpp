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

#include "factory/FileSystemFactory.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "logging/Logger.h"

#if defined(__linux__) || defined(__APPLE__)
# include "filesystem/unix/Directory.h"
# include "filesystem/unix/File.h"
# include "filesystem/unix/Device.h"
#elif defined(_WIN32)
# include "filesystem/win32/Directory.h"
# include "filesystem/win32/File.h"
#else
# error No filesystem implementation for this architecture
#endif

namespace medialibrary
{

namespace factory
{

FileSystemFactory::FileSystemFactory( DeviceListerPtr lister )
{
    fs::Device::setDeviceLister( lister );
}

std::shared_ptr<fs::IDirectory> FileSystemFactory::createDirectory( const std::string& path )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    const auto it = m_dirs.find( path );
    if ( it != end( m_dirs ) )
        return it->second;
    try
    {
        auto dir = std::make_shared<fs::Directory>( path );
        m_dirs[path] = dir;
        return dir;
    }
    catch(std::exception& ex)
    {
        LOG_ERROR( "Failed to create fs::IDirectory for ", path, ": ", ex.what());
        return nullptr;
    }
}

std::shared_ptr<fs::IDevice> FileSystemFactory::createDevice( const std::string& uuid )
{
    return fs::Device::fromUuid( uuid );
}

void FileSystemFactory::refresh()
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_dirs.clear();
}

}

}
