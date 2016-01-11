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
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cassert>

#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "filesystem/IDevice.h"
#include "factory/IFileSystem.h"
#include "utils/Filename.h"

//REMOVE ME
#include "logging/Logger.h"

namespace mock
{

class File : public fs::IFile
{
public:
    File( const std::string& filePath )
        : m_name( utils::file::fileName( filePath ) )
        , m_path( utils::file::directory( filePath ) )
        , m_fullPath( filePath )
        , m_extension( utils::file::extension( filePath ) )
        , m_lastModification( 0 )
    {
    }

    File( const File& ) = default;

    virtual const std::string& name() const override
    {
        return m_name;
    }

    virtual const std::string& path() const override
    {
        return m_path;
    }

    virtual const std::string& fullPath() const override
    {
        return m_fullPath;
    }

    virtual const std::string& extension() const override
    {
        return m_extension;
    }

    virtual unsigned int lastModificationDate() const override
    {
        return m_lastModification;
    }

    void markAsModified()
    {
        m_lastModification++;
    }

    std::string m_name;
    std::string m_path;
    std::string m_fullPath;
    std::string m_extension;
    unsigned int m_lastModification;
};

class Device;

class Directory : public fs::IDirectory
{
public:
    Directory( const std::string& path, std::shared_ptr<Device> device )
        : m_path( path )
        , m_device( device )
    {
        if ( ( *m_path.crbegin() ) != '/' )
            m_path += '/';
    }

    virtual const std::string& path() const override
    {
        return m_path;
    }

    virtual const std::vector<std::string>& files() override
    {
        if ( m_filePathes.size() == 0 )
        {
            for ( const auto& f : m_files )
                m_filePathes.push_back( m_path + f.first );
        }
        return m_filePathes;
    }

    virtual const std::vector<std::string>& dirs() override
    {
        if ( m_dirPathes.size() == 0 )
        {
            for ( const auto& d : m_dirs )
                m_dirPathes.push_back( m_path + d.first + "/" );
        }
        return m_dirPathes;
    }

    virtual std::shared_ptr<fs::IDevice> device() const override
    {
        return std::static_pointer_cast<fs::IDevice>( m_device.lock() );
    }

    void addFile( const std::string& filePath )
    {
        auto subFolder = utils::file::firstFolder( filePath );
        if ( subFolder.empty() == true )
        {
            m_files[filePath] = std::make_shared<File>( m_path + filePath );
            m_filePathes.clear();
        }
        else
        {
            auto it = m_dirs.find( subFolder );
            assert( it != end( m_dirs ) );
            auto remainingPath = utils::file::removePath( filePath, subFolder );
            it->second->addFile( remainingPath );
        }
    }

    void addFolder( const std::string& folder )
    {
        auto subFolder = utils::file::firstFolder( folder );
        auto remainingPath = utils::file::removePath( folder, subFolder );
        if ( remainingPath.empty() == true )
        {
            auto dir = std::make_shared<Directory>( m_path + subFolder, m_device.lock() );
            m_dirs[subFolder] = dir;
            m_dirPathes.clear();
        }
        else
        {
            auto it = m_dirs.find( subFolder );
            assert( it != end( m_dirs ) );
            it->second->addFolder( remainingPath );
        }
    }

    void removeFile( const std::string& filePath  )
    {
        auto subFolder = utils::file::firstFolder( filePath );
        if ( subFolder.empty() == true )
        {
            auto it = m_files.find( filePath );
            assert( it != end( m_files ) );
            m_files.erase( it );
            m_filePathes.clear();
        }
        else
        {
            auto it = m_dirs.find( subFolder );
            assert( it != end( m_dirs ) );
            auto remainingPath = utils::file::removePath( filePath, subFolder );
            it->second->removeFile( remainingPath );
        }
    }

    std::shared_ptr<File> file( const std::string& filePath )
    {
        auto subFolder = utils::file::firstFolder( filePath );
        if ( subFolder.empty() == true )
        {
            auto it = m_files.find( filePath );
            assert( it != end( m_files ) );
            return it->second;
        }
        else
        {
            auto it = m_dirs.find( subFolder );
            assert( it != end( m_dirs ) );
            auto remainingPath = utils::file::removePath( filePath, subFolder );
            return it->second->file( remainingPath );
        }
    }

    std::shared_ptr<Directory> directory( const std::string& path )
    {
        auto subFolder = utils::file::firstFolder( path );
        auto remainingPath = utils::file::removePath( path, subFolder );
        if ( remainingPath.empty() == true )
        {
            auto it = m_dirs.find( subFolder );
            if ( it == end( m_dirs ) )
                return nullptr;
            return it->second;
        }
        else
        {
            auto it = m_dirs.find( subFolder );
            assert( it != end( m_dirs ) );
            return it->second->directory( remainingPath );
        }
    }

    void removeFolder( const std::string& path )
    {
        auto subFolder = utils::file::firstFolder( path );
        auto remainingPath = utils::file::removePath( path, subFolder );
        if ( remainingPath.empty() == true )
        {
            auto it = m_dirs.find( subFolder );
            assert( it != end( m_dirs ) );
            m_dirs.erase( it );
            m_dirPathes.clear();
        }
        else
        {
            auto it = m_dirs.find( subFolder );
            assert( it != end( m_dirs ) );
            it->second->removeFolder( remainingPath );
        }
    }

    void addMountpoint( const std::string& path )
    {
        auto subFolder = utils::file::firstFolder( path );
        auto remainingPath = utils::file::removePath( path, subFolder );
        if ( remainingPath.empty() == true )
        {
            m_dirs[subFolder] = nullptr;
        }
        else
        {
            auto it = m_dirs.find( subFolder );
            assert( it != end( m_dirs ) );
            it->second->addMountpoint( remainingPath );
        }
    }

private:
    std::string m_path;
    std::unordered_map<std::string, std::shared_ptr<File>> m_files;
    std::unordered_map<std::string, std::shared_ptr<Directory>> m_dirs;
    std::vector<std::string> m_filePathes;
    std::vector<std::string> m_dirPathes;
    std::weak_ptr<Device> m_device;
};

class Device : public fs::IDevice, public std::enable_shared_from_this<Device>
{
public:
    Device( const std::string& mountpoint, const std::string& uuid )
        : m_uuid( uuid )
        , m_removable( false )
        , m_present( true )
        , m_mountpoint( mountpoint )
    {
        if ( ( *m_mountpoint.crbegin() ) != '/' )
            m_mountpoint += '/';
    }

    // We need at least one existing shared ptr before calling shared_from_this.
    // Let the device be initialized and stored in a shared_ptr by the FileSystemFactory, and then
    // initialize our fake root folder
    void setupRoot()
    {
        m_root = std::make_shared<Directory>( m_mountpoint, shared_from_this() );
    }

    virtual const std::string& uuid() const override { return m_uuid; }
    virtual bool isRemovable() const override { return m_removable; }
    virtual bool isPresent() const override { return m_present; }
    virtual const std::string& mountpoint() const override { return m_mountpoint; }

    void setRemovable( bool value ) { m_removable = value; }
    void setPresent( bool value ) { m_present = value; }

    std::string relativePath( const std::string& path )
    {
        auto res = path.substr( m_mountpoint.length() );
        while ( res[0] == '/' )
            res.erase( res.begin() );
        return res;
    }

    void addFile( const std::string& filePath )
    {
        m_root->addFile( relativePath( filePath ) );
    }

    void addFolder( const std::string& path )
    {
        m_root->addFolder( relativePath( path ) );
    }

    void removeFile( const std::string& filePath )
    {
        m_root->removeFile( relativePath( filePath ) );
    }

    void removeFolder( const std::string& filePath )
    {
        auto relPath = relativePath( filePath );
        if ( relPath.empty() == true )
            m_root = nullptr;
        else
            m_root->removeFolder( relPath );
    }

    std::shared_ptr<File> file( const std::string& filePath )
    {
        if ( m_root == nullptr || m_present == false )
            return nullptr;
        return m_root->file( relativePath( filePath ) );
    }

    std::shared_ptr<Directory> directory( const std::string& path )
    {
        if ( m_root == nullptr || m_present == false )
            return nullptr;
        const auto relPath = relativePath( path );
        if ( relPath.empty() == true )
            return m_root;
        return m_root->directory( relPath );
    }

    void addMountpoint( const std::string& path )
    {
        auto relPath = relativePath( path );
        // m_root is already a mountpoint, we can't add a mountpoint to it.
        assert( relPath.empty() == false );
        m_root->addMountpoint( relPath );
    }

private:
    std::string m_uuid;
    bool m_removable;
    bool m_present;
    std::string m_mountpoint;
    std::shared_ptr<Directory> m_root;
};

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
        // If this folder is already known, mark it as a nullptr and assume this means a mountpoint
        auto d = device( mountpoint );
        if ( d != nullptr )
            d->addMountpoint( mountpoint );
        devices.push_back( dev );
        return dev;
    }

    void addDevice( std::shared_ptr<Device> device )
    {
        devices.push_back( device );
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

    virtual std::unique_ptr<fs::IFile> createFile( const std::string &filePath ) override
    {
        auto d = device( filePath );
        if ( d == nullptr )
            return nullptr;
        auto f = d->file( filePath );
        if ( f == nullptr )
            return nullptr;
        return std::unique_ptr<fs::IFile>( new File( *f ) );
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
            if ( path.find( d->mountpoint() ) == 0 &&
                 ( ret == nullptr || ret->mountpoint().length() < d->mountpoint().length() ) )
                ret = d;
        }
        return ret;
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

public:
    NoopFile( const std::string& file )
        : m_path( file )
        , m_fileName( utils::file::fileName( file ) )
        , m_extension( utils::file::extension( file ) )
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
        return 123;
    }
};

class NoopDevice : public fs::IDevice
{
public:
    virtual const std::string&uuid() const override
    {
        assert(false);
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
        assert(false);
    }
};

// We just need a valid instance of this one
class NoopDirectory : public fs::IDirectory
{
    virtual const std::string& path() const override
    {
        assert(false);
    }

    virtual const std::vector<std::string>&files() override
    {
        assert(false);
    }

    virtual const std::vector<std::string>&dirs() override
    {
        assert(false);
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

    virtual std::unique_ptr<fs::IFile> createFile( const std::string &fileName ) override
    {
        return std::unique_ptr<fs::IFile>( new NoopFile( fileName ) );
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

