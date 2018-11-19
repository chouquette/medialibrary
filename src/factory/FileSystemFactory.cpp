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
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "logging/Logger.h"
#include "utils/Filename.h"
#include "utils/Directory.h"
#include "utils/Url.h"

#if defined(__linux__) || defined(__APPLE__)
# include "filesystem/unix/Directory.h"
# include "filesystem/unix/File.h"
# include "filesystem/unix/Device.h"
#elif defined(_WIN32)
# include "filesystem/win32/Directory.h"
# include "filesystem/win32/File.h"
# include "filesystem/win32/Device.h"
#else
# error No filesystem implementation for this architecture
#endif

#include "medialibrary/IDeviceLister.h"
#include "compat/Mutex.h"

#include <algorithm>

namespace medialibrary
{

namespace factory
{

FileSystemFactory::FileSystemFactory( DeviceListerPtr lister )
    : m_deviceLister( lister )
{
}

std::shared_ptr<fs::IDirectory> FileSystemFactory::createDirectory( const std::string& mrl )
{
    return std::make_shared<fs::Directory>( mrl, *this );
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
    std::string canonicalMountpoint;
    try
    {
        canonicalMountpoint = utils::fs::toAbsolute(
                    utils::url::decode( utils::file::stripScheme( mrl ) ) );
        canonicalMountpoint = scheme() + canonicalMountpoint;
    }
    catch ( const std::system_error& ex )
    {
        LOG_WARN( "Failed to canonicalize mountpoint ", mrl, ": ", ex.what() );
        return nullptr;
    }
    std::shared_ptr<fs::IDevice> res;
    for ( const auto& p : m_deviceCache )
    {
        if ( canonicalMountpoint.find( p.second->mountpoint() ) == 0 )
        {
            if ( res == nullptr || res->mountpoint().length() < p.second->mountpoint().length() )
                res = p.second;
        }
    }
    return res;
}

void FileSystemFactory::refreshDevices()
{
    LOG_INFO( "Refreshing devices from IDeviceLister" );
    auto devices = m_deviceLister->devices();

    for ( auto& devicePair : m_deviceCache )
    {
        auto it = std::find_if( begin( devices ), end( devices ),
                                [&devicePair]( decltype(devices)::value_type& deviceTuple ) {
                                    return std::get<0>( deviceTuple ) == devicePair.first;
        });
        // If the device isn't present anymore, mark it as such, but don't remove it
        // That way, the directories that cached the device still have an up to date information
        if ( it == end( devices ) )
            devicePair.second->setPresent( false );
        else
        {
            // If we already know the device, ensure it's marked as present
            devicePair.second->setPresent( true );
            devices.erase( it );
        }
    }
    // And now insert all new devices, if any
    for ( const auto& d : devices )
    {
        const auto& uuid = std::get<0>( d );
        const auto& mountpoint = std::get<1>( d );
        const auto removable = std::get<2>( d );
        LOG_INFO( "Caching device ", uuid, " mounted on ", mountpoint, ". Removable: ", removable ? "true" : "false" );
        m_deviceCache.emplace( uuid, std::make_shared<fs::Device>( uuid, mountpoint, removable ) );
    }
}

bool FileSystemFactory::isMrlSupported( const std::string& path ) const
{
    return path.compare( 0, 7, "file://" ) == 0;
}

bool FileSystemFactory::isNetworkFileSystem() const
{
    return false;
}

const std::string&FileSystemFactory::scheme() const
{
    static const std::string s = "file://";
    return s;
}

bool FileSystemFactory::start( fs::IFileSystemFactoryCb* )
{
    return true;
}

void FileSystemFactory::stop()
{
}

}

}
