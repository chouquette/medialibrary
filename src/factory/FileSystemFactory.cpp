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

#include "factory/FileSystemFactory.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/Errors.h"
#include "logging/Logger.h"
#include "utils/Filename.h"
#include "utils/Directory.h"
#include "MediaLibrary.h"

#if defined(__linux__) || defined(__APPLE__)
# include "filesystem/unix/Directory.h"
# include "filesystem/unix/Device.h"
#elif defined(_WIN32)
# include "filesystem/win32/Directory.h"
# include "filesystem/win32/File.h"
# include "filesystem/win32/Device.h"
#else
# error No filesystem implementation for this architecture
#endif

#include "medialibrary/IDeviceLister.h"

#include <algorithm>
#include <cassert>

namespace medialibrary
{

namespace factory
{

FileSystemFactory::FileSystemFactory( MediaLibraryPtr ml )
    : m_deviceLister( ml->deviceListerLocked( "file://" ) )
{
    if ( m_deviceLister == nullptr )
        throw std::runtime_error( "Failed to acquire a local device lister" );
}

std::shared_ptr<fs::IDirectory> FileSystemFactory::createDirectory( const std::string& mrl )
{
    return std::make_shared<fs::Directory>( mrl, *this );
}

std::shared_ptr<fs::IFile> FileSystemFactory::createFile( const std::string& mrl )
{
    auto fsDir = createDirectory( utils::file::directory( mrl ) );
    assert( fsDir != nullptr );
    return fsDir->file( mrl );
}

std::shared_ptr<fs::IDevice> FileSystemFactory::createDevice( const std::string& uuid )
{
    auto it = m_deviceCache.find( uuid );
    if ( it != end( m_deviceCache ) )
        return it->second;
    return nullptr;
}

std::shared_ptr<fs::IDevice> FileSystemFactory::createDeviceFromMrl( const std::string& mrl )
{
    std::string canonicalMrl;
    try
    {
        auto canonicalPath = utils::fs::toAbsolute( utils::file::toLocalPath( mrl ) );
        canonicalMrl = utils::file::toMrl( canonicalPath );
    }
    catch ( const fs::errors::System& ex )
    {
        LOG_WARN( "Failed to canonicalize mountpoint ", mrl, ": ", ex.what() );
        return nullptr;
    }
    std::shared_ptr<fs::IDevice> res;
    std::string mountpoint;
    for ( const auto& p : m_deviceCache )
    {
        auto match = p.second->matchesMountpoint( canonicalMrl );
        if ( std::get<0>( match ) == true )
        {
            const auto& newMountpoint = std::get<1>( match );
            if ( res == nullptr || mountpoint.length() < newMountpoint.length() )
            {
                res = p.second;
                mountpoint = newMountpoint;
            }
        }
    }
    return res;
}

void FileSystemFactory::refreshDevices()
{
    LOG_INFO( "Refreshing devices from IDeviceLister" );
    m_deviceLister->refresh();
    LOG_INFO( "Done refreshing devices from IDeviceLister" );
}

bool FileSystemFactory::isMrlSupported( const std::string& path ) const
{
    return path.compare( 0, 7, "file://" ) == 0;
}

bool FileSystemFactory::isNetworkFileSystem() const
{
    return false;
}

const std::string& FileSystemFactory::scheme() const
{
    static const std::string s = "file://";
    return s;
}

bool FileSystemFactory::start( fs::IFileSystemFactoryCb* cb )
{
    m_cb = cb;
    m_deviceLister->start( this );
    return true;
}

void FileSystemFactory::stop()
{
}

bool FileSystemFactory::onDeviceMounted( const std::string& uuid,
                                         const std::string& mp,
                                         bool removable )
{

    auto deviceIt = m_deviceCache.find( uuid );
    auto mountpoint = utils::file::toFolderPath( mp );
    LOG_DEBUG( "Device: ", uuid, "; mounted on: ", mountpoint, "; removable: ",
               removable ? "yes" : "no" );
    std::shared_ptr<fs::IDevice> device;
    if ( deviceIt == end( m_deviceCache ) )
    {
        device = std::make_shared<fs::Device>( uuid, mountpoint, removable );
        m_deviceCache.emplace( uuid, device );
    }
    else
    {
        device = deviceIt->second;
        device->addMountpoint( mountpoint );
    }
    return m_cb->onDeviceMounted( *device );
}

void FileSystemFactory::onDeviceUnmounted( const std::string& uuid,
                                           const std::string& mp )
{
    LOG_DEBUG( "Device: ", uuid, "; unmounted from: ", mp );
    auto deviceIt = m_deviceCache.find( uuid );
    if ( deviceIt == end( m_deviceCache ) )
    {
        assert( !"An unknown device was unmounted" );
        return;
    }
    auto mountpoint = utils::file::toFolderPath( mp );
    deviceIt->second->removeMountpoint( mountpoint );
    m_cb->onDeviceUnmounted( *(deviceIt->second) );
}

}

}
