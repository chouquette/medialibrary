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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <memory>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "medialibrary/filesystem/Errors.h"
#include "utils/Filename.h"

#include "mocks/filesystem/MockDevice.h"
#include "mocks/filesystem/MockDirectory.h"
#include "mocks/filesystem/MockFile.h"

namespace mock
{

struct FileSystemFactory : public fs::IFileSystemFactory
{
    static const std::string Root;
    static const std::string SubFolder;
    static const std::string RootDeviceUuid;
    static const std::string NoopDeviceUuid;

    FileSystemFactory()
        : m_cb( nullptr )
    {
        // Add a root device unremovable
        auto rootDevice = addDevice( Root, RootDeviceUuid, false );
        rootDevice->addFile( Root + "video.avi" );
        rootDevice->addFile( Root + "audio.mp3" );
        rootDevice->addFile( Root + "not_a_media.something" );
        rootDevice->addFile( Root + "some_other_file.seaotter" );
        rootDevice->addFolder( SubFolder );
        rootDevice->addFile( SubFolder + "subfile.mp4" );
    }

    std::shared_ptr<Device> addDevice( const std::string& mountpointMrl, const std::string& uuid,
                                       bool removable )
    {
        auto dev = std::make_shared<Device>( mountpointMrl, uuid, removable );
        dev->setupRoot();
        addDevice( dev );
        return dev;
    }

    bool addDeviceMountpoint( const std::string& mountpoint, const std::string& uuid )
    {
        auto it = std::find_if( begin( devices ), end( devices ), [uuid]( const std::shared_ptr<Device>& d ) {
            return d->uuid() == uuid;
        } );
        if ( it == end( devices ) )
            return false;
        (*it)->addMountpoint( mountpoint );
        return true;
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
        assert( ret->isRemovable() == true );
        ret->setPresent( false );
        // Now flag the mountpoint as belonging to its containing device, since it's now
        // just a regular folder
        auto d = device( ret->mountpoints()[0] );
        d->invalidateMountpoint( ret->mountpoints()[0] );
        m_cb->onDeviceUnmounted( *ret );
        return ret;
    }

    void unmountDevice( const std::string& uuid )
    {
        auto it = std::find_if( begin( devices ), end( devices ), [uuid]( const std::shared_ptr<Device>& d ) {
            return d->uuid() == uuid;
        } );
        auto d = *it;
        // Mark the device as being removed
        d->setPresent( false );
        // And now fetch the device that contains the mountpoint of the device
        // we just removed.
        auto mountpointDevice = device( d->mountpoints()[0] );
        mountpointDevice->invalidateMountpoint( d->mountpoints()[0] );
        m_cb->onDeviceUnmounted( *d );
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
        auto mountpointDevice = device( d->mountpoints()[0] );
        d->setPresent( true );
        mountpointDevice->setMountpointRoot( d->mountpoints()[0], d->root() );
        m_cb->onDeviceMounted( *d );
    }

    void addDevice( std::shared_ptr<Device> dev )
    {
        auto d = device( dev->mountpoints()[0] );
        if ( d != nullptr )
            d->setMountpointRoot( dev->mountpoints()[0], dev->root() );
        devices.push_back( dev );
        dev->setPresent( true );
        if ( m_cb != nullptr )
            m_cb->onDeviceMounted( *dev );
    }

    void addFile( const std::string& mrl )
    {
        auto d = device( mrl );
        d->addFile( mrl );
    }

    void addFolder( const std::string& mrl )
    {
        auto d = device( mrl );
        d->addFolder( mrl );
    }

    void removeFile( const std::string& mrl )
    {
        auto d = device( mrl );
        d->removeFile( mrl );
    }

    void removeFolder( const std::string& mrl )
    {
        auto d = device( mrl );
        d->removeFolder( mrl );
    }

    std::shared_ptr<fs::IFile> file( const std::string& mrl )
    {
        auto d = device( mrl );
        return d->file( mrl );
    }

    std::shared_ptr<Directory> directory( const std::string& mrl )
    {
        auto d = device( mrl );
        return d->directory( mrl );
    }

    virtual std::shared_ptr<fs::IDirectory> createDirectory( const std::string& mrl ) override
    {
        auto d = device( mrl );
        if ( d == nullptr )
            throw fs::errors::System{ ENOENT, "Mock directory" };
        auto dir = d->directory( mrl );
        if ( dir == nullptr )
            throw fs::errors::System{ ENOENT, "Mock directory" };
        return dir;
    }

    virtual std::shared_ptr<fs::IFile> createFile( const std::string& mrl ) override
    {
        auto dir = createDirectory( mrl );
        if ( dir == nullptr )
            return nullptr;
        return dir->file( mrl );
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

    virtual void refreshDevices() override
    {
    }

    std::shared_ptr<Device> device( const std::string& mrl )
    {
        std::shared_ptr<Device> ret;
        std::string mountpoint;
        for ( auto& d : devices )
        {
            if ( d->isPresent() == false )
                continue;
            auto match = d->matchesMountpoint( mrl );
            if ( std::get<0>( match ) == true )
            {
                const auto& newMountpoint = std::get<1>( match );
                if ( ret == nullptr || mountpoint.length() < newMountpoint.length() )
                {
                    ret = d;
                    mountpoint = newMountpoint;
                }
            }
        }
        return ret;
    }

    virtual std::shared_ptr<fs::IDevice> createDeviceFromMrl( const std::string& mrl ) override
    {
        return device( mrl );
    }

    virtual bool isMrlSupported( const std::string& mrl ) const override
    {
        return mrl.compare( 0, strlen( "file://" ), "file://" ) == 0;
    }

    virtual bool isNetworkFileSystem() const override
    {
        return false;
    }

    virtual const std::string& scheme() const override
    {
        static const std::string s = "file://";
        return s;
    }

    virtual bool start( fs::IFileSystemFactoryCb* cb ) override
    {
        m_cb = cb;
        return true;
    }
    virtual void stop() override
    {
        assert( m_cb != nullptr );
        m_cb = nullptr;
    }

    virtual bool isStarted() const override
    {
        return m_cb != nullptr;
    }

    std::vector<std::shared_ptr<Device>> devices;
    fs::IFileSystemFactoryCb* m_cb;
};

// Noop FS (basically just returns file names, and don't try to access those.)
class NoopFile : public fs::IFile
{
    std::string m_path;
    std::string m_fileName;
    std::string m_extension;
    std::string m_linkedWith;
    unsigned int m_lastModifDate;
    int64_t m_size;

public:
    NoopFile( const std::string& file )
        : m_path( file )
        , m_fileName( utils::file::fileName( file ) )
        , m_extension( utils::file::extension( file ) )
        , m_lastModifDate( 123 )
        , m_size( 321 )
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

    virtual const std::string& mrl() const
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

    virtual bool isNetwork() const
    {
        return false;
    }

    virtual int64_t size() const
    {
        return m_size;
    }

    LinkedFileType linkedType() const
    {
        return LinkedFileType::None;
    }

    const std::string& linkedWith() const
    {
        return m_linkedWith;
    }

    void setLastModificationDate( unsigned int date )
    {
        m_lastModifDate = date;
    }

    void setSize( unsigned int size )
    {
        m_size = size;
    }
};

class NoopDevice : public fs::IDevice
{
public:
    NoopDevice()
    {
        m_uuid = mock::FileSystemFactory::NoopDeviceUuid;
        m_scheme = "file://";
    }

    virtual const std::string& uuid() const override
    {
        return m_uuid;
    }

    virtual const std::string& scheme() const override
    {
        return m_scheme;
    }

    virtual bool isRemovable() const override
    {
        return false;
    }

    virtual bool isPresent() const override
    {
        return true;
    }

    virtual bool isNetwork() const override
    {
        return false;
    }

    const std::string& mountpoint() const
    {
        return m_mountpoint;
    }

    virtual std::vector<std::string> mountpoints() const override
    {
        return std::vector<std::string>{ { m_mountpoint } };
    }

    virtual void addMountpoint( std::string ) override
    {
        assert( false );
    }

    virtual void removeMountpoint( const std::string& ) override
    {
        assert( false );
    }

    virtual std::string relativeMrl( const std::string& mrl ) const override
    {
        return mrl;
    }

    virtual std::string absoluteMrl( const std::string& mrl ) const override
    {
        return mrl;
    }

    std::tuple<bool, std::string> matchesMountpoint( const std::string& ) const override
    {
        return { false, "" };
    }

private:
    std::string m_uuid;
    std::string m_scheme;
    std::string m_mountpoint;
};

// We just need a valid instance of this one
class NoopDirectory : public fs::IDirectory
{
    virtual const std::string& mrl() const override
    {
        abort();
    }

    virtual const std::vector<std::shared_ptr<fs::IFile>>& files() const override
    {
        abort();
    }

    virtual std::shared_ptr<fs::IFile> file( const std::string& ) const override
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

class NoopFsFactory : public fs::IFileSystemFactory
{
public:
    virtual std::shared_ptr<fs::IDirectory> createDirectory( const std::string& ) override
    {
        throw fs::errors::System{ ENOENT, "Mock directory" };
    }

    virtual std::shared_ptr<fs::IFile> createFile( const std::string& ) override
    {
        throw fs::errors::System{ ENOENT, "Mock directory" };
    }

    virtual std::shared_ptr<fs::IDevice> createDevice( const std::string& uuid ) override
    {
        if ( uuid == mock::FileSystemFactory::NoopDeviceUuid )
            return std::make_shared<mock::NoopDevice>();
        return nullptr;
    }

    virtual std::shared_ptr<fs::IDevice> createDeviceFromMrl( const std::string& ) override
    {
        return std::make_shared<NoopDevice>();
    }

    virtual void refreshDevices() override
    {
    }

    virtual bool isMrlSupported( const std::string& mrl ) const override
    {
        auto it = mrl.find( "://" );
        if ( it == std::string::npos )
            return true;
        return mrl.compare( 0, 7, "file://" ) == 0;
    }

    virtual const std::string& scheme() const override
    {
        static const std::string s = "file://";
        return s;
    }

    virtual bool isNetworkFileSystem() const override
    {
        return false;
    }
    virtual bool start( fs::IFileSystemFactoryCb* ) override { return true; }
    virtual void stop() override {}
    virtual bool isStarted() const override { return true; }
};

}


#endif // FILESYSTEM_H

