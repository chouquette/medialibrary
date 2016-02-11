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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <memory>
#include <algorithm>

#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "filesystem/IDevice.h"
#include "factory/IFileSystem.h"
#include "utils/Filename.h"

#include "mocks/filesystem/MockDevice.h"
#include "mocks/filesystem/MockDirectory.h"
#include "mocks/filesystem/MockFile.h"

namespace mock
{

struct FileSystemFactory : public factory::IFileSystem
{
    static const std::string Root;
    static const std::string SubFolder;
    static const std::string RootDeviceUuid;

    FileSystemFactory()
    {
        // Add a root device unremovable
        auto rootDevice = addDevice( Root, RootDeviceUuid );
        rootDevice->addFile( Root + "video.avi" );
        rootDevice->addFile( Root + "audio.mp3" );
        rootDevice->addFile( Root + "not_a_media.something" );
        rootDevice->addFile( Root + "some_other_file.seaotter" );
        rootDevice->addFolder( SubFolder );
        rootDevice->addFile( SubFolder + "subfile.mp4" );
    }

    std::shared_ptr<Device> addDevice( const std::string& mountpoint, const std::string& uuid )
    {
        auto dev = std::make_shared<Device>( mountpoint, uuid );
        dev->setupRoot();
        auto d = device( mountpoint );
        if ( d != nullptr )
            d->setMountpointRoot( mountpoint, dev->root() );
        devices.push_back( dev );
        return dev;
    }

    std::shared_ptr<Device> removeDevice( const std::string& uuid )
    {
        auto it = std::find_if( begin( devices ), end( devices ), [uuid]( const std::shared_ptr<Device>& d ) {
            return d->uuid() == uuid;
        } );
        if ( it == end( devices ) )
            return nullptr;
        auto ret = *it;
        devices.erase( it );
        // Now flag the mountpoint as belonging to its containing device, since it's now
        // just a regular folder
        auto d = device( ret->mountpoint() );
        d->invalidateMountpoint( ret->mountpoint() );
        return ret;
    }

    void unmountDevice( const std::string& uuid )
    {
        auto it = std::find_if( begin( devices ), end( devices ), [uuid]( const std::shared_ptr<Device>& d ) {
            return d->uuid() == uuid;
        } );
        if ( it == end( devices ) )
            return;
        auto d = *it;
        d->setPresent( false );
        auto mountpointDevice = device( d->mountpoint() );
        mountpointDevice->invalidateMountpoint( d->mountpoint() );
    }

    void remountDevice( const std::string& uuid )
    {
        auto it = std::find_if( begin( devices ), end( devices ), [uuid]( const std::shared_ptr<Device>& d ) {
            return d->uuid() == uuid;
        } );
        if ( it == end( devices ) )
            return;
        auto d = *it;
        // Look for the containing device before marking the actual device back as present.
        // otherwise, we will get the device mountpoint itself, instead of the device that contains
        // the mountpoint
        auto mountpointDevice = device( d->mountpoint() );
        d->setPresent( true );
        mountpointDevice->setMountpointRoot( d->mountpoint(), d->root() );
    }

    void addDevice( std::shared_ptr<Device> dev )
    {
        auto d = device( dev->mountpoint() );
        if ( d != nullptr )
            d->setMountpointRoot( dev->mountpoint(), dev->root() );
        devices.push_back( dev );
    }

    void addFile( const std::string& filePath )
    {
        auto d = device( filePath );
        d->addFile( filePath );
    }

    void addFolder( const std::string& path )
    {
        auto d = device( path );
        d->addFolder( path );
    }

    void removeFile( const std::string& filePath )
    {
        auto d = device( filePath );
        d->removeFile( filePath );
    }

    void removeFolder( const std::string& path )
    {
        auto d = device( path );
        d->removeFolder( path );
    }

    std::shared_ptr<File> file( const std::string& filePath )
    {
        auto d = device( filePath );
        return d->file( filePath );
    }

    std::shared_ptr<Directory> directory( const std::string& path )
    {
        auto d = device( path );
        return d->directory( path );
    }

    virtual std::shared_ptr<fs::IDirectory> createDirectory( const std::string& path ) override
    {
        auto d = device( path );
        if ( d == nullptr )
            return nullptr;
        return d->directory( path );
    }

    virtual std::shared_ptr<fs::IDevice> createDevice( const std::string& uuid ) override
    {
        auto it = std::find_if( begin( devices ), end( devices ), [uuid]( const std::shared_ptr<Device>& d ) {
            return d->uuid() == uuid;
        } );
        if ( it == end( devices ) )
            return nullptr;
        return *it;
    }

    virtual void refresh() override {}

    std::shared_ptr<Device> device( const std::string& path )
    {
        std::shared_ptr<Device> ret;
        for ( auto& d : devices )
        {
            if ( path.find( d->mountpoint() ) == 0 && d->isPresent() == true &&
                 ( ret == nullptr || ret->mountpoint().length() < d->mountpoint().length() ) )
                ret = d;
        }
        return ret;
    }

    std::vector<std::shared_ptr<Device>> devices;
};

// Noop FS (basically just returns file names, and don't try to access those.)
class NoopFile : public fs::IFile
{
    std::string m_path;
    std::string m_fileName;
    std::string m_extension;
    unsigned int m_lastModifDate;

public:
    NoopFile( const std::string& file )
        : m_path( file )
        , m_fileName( utils::file::fileName( file ) )
        , m_extension( utils::file::extension( file ) )
        , m_lastModifDate( 123 )
    {
    }

    virtual const std::string& name() const
    {
        return m_fileName;
    }

    virtual const std::string& path() const
    {
        return m_path;
    }

    virtual const std::string& fullPath() const
    {
        return m_path;
    }

    virtual const std::string& extension() const
    {
        return m_extension;
    }

    virtual unsigned int lastModificationDate() const
    {
        // Ensure a non-0 value so tests can easily verify that the value
        // is initialized
        return m_lastModifDate;
    }

    void setLastModificationDate( unsigned int date )
    {
        m_lastModifDate = date;
    }
};

class NoopDevice : public fs::IDevice
{
public:
    virtual const std::string& uuid() const override
    {
        abort();
    }

    virtual bool isRemovable() const override
    {
        return false;
    }

    virtual bool isPresent() const override
    {
        return true;
    }

    virtual const std::string& mountpoint() const override
    {
        abort();
    }
};

// We just need a valid instance of this one
class NoopDirectory : public fs::IDirectory
{
    virtual const std::string& path() const override
    {
        abort();
    }

    virtual const std::vector<std::shared_ptr<fs::IFile>>& files() const override
    {
        abort();
    }

    virtual const std::vector<std::shared_ptr<fs::IDirectory>>& dirs() const override
    {
        abort();
    }

    virtual std::shared_ptr<fs::IDevice> device() const override
    {
        return std::make_shared<NoopDevice>();
    }
};

class NoopFsFactory : public factory::IFileSystem
{
public:
    virtual std::shared_ptr<fs::IDirectory> createDirectory( const std::string& ) override
    {
        return nullptr;
    }

    virtual std::shared_ptr<fs::IDevice> createDevice( const std::string& ) override
    {
        return nullptr;
    }

    virtual void refresh() override
    {
    }
};

}


#endif // FILESYSTEM_H

