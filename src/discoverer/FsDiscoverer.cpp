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

#include "FsDiscoverer.h"

#include <algorithm>
#include <queue>

#include "factory/FileSystem.h"
#include "filesystem/IDevice.h"
#include "Media.h"
#include "Device.h"
#include "Folder.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "utils/Filename.h"

FsDiscoverer::FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory, MediaLibrary* ml, DBConnection dbConn )
    : m_ml( ml )
    , m_dbConn( dbConn )
{
    if ( fsFactory != nullptr )
        m_fsFactory = fsFactory;
    else
        m_fsFactory.reset( new factory::FileSystemFactory );
}

bool FsDiscoverer::discover( const std::string &entryPoint )
{
    // Assume :// denotes a scheme that isn't a file path, and refuse to discover it.
    if ( entryPoint.find( "://" ) != std::string::npos )
        return false;

    std::shared_ptr<fs::IDirectory> fsDir = m_fsFactory->createDirectory( entryPoint );
    {
        auto f = Folder::fromPath( m_dbConn, entryPoint );
        // If the folder exists, we assume it will be handled by reload()
        if ( f != nullptr )
            return true;
    }
    // Otherwise, create a directory and check it for modifications
    if ( fsDir == nullptr )
    {
        LOG_ERROR("Failed to create an IDirectory for ", entryPoint );
        return false;
    }
    auto blist = blacklist();
    if ( isBlacklisted( *fsDir, blist ) == true )
        return false;
    return addFolder( *fsDir, nullptr, blist );
}

void FsDiscoverer::reload()
{
    // Start by checking if previously known devices have been plugged/unplugged
    checkDevices();
    auto rootFolders = Folder::fetchAll( m_dbConn, 0 );
    auto blist = blacklist();
    for ( const auto& f : rootFolders )
    {
        auto folder = m_fsFactory->createDirectory( f->path() );
        if ( folder == nullptr )
        {
            LOG_INFO( "Removing folder ", f->path() );
            m_ml->deleteFolder( f.get() );
            continue;
        }
        checkSubfolders( *folder, *f, blist );
        checkFiles( *folder, *f );
    }
}

void FsDiscoverer::checkDevices()
{
    auto devices = Device::fetchAll( m_dbConn );
    for ( auto& d : devices )
    {
        auto deviceFs = m_fsFactory->createDevice( d->uuid() );
        auto fsDevicePresent = deviceFs != nullptr && deviceFs->isPresent();
        if ( d->isPresent() != fsDevicePresent )
        {
            LOG_INFO( "Device ", d->uuid(), " changed presence state: ", d->isPresent(), " -> ", fsDevicePresent );
            d->setPresent( fsDevicePresent );
        }
        else
        {
            LOG_INFO( "Device ", d->uuid(), " unchanged" );
        }
    }
}

bool FsDiscoverer::checkSubfolders( fs::IDirectory& folder, Folder& parentFolder, const std::vector<std::shared_ptr<Folder>> blacklist ) const
{
    // Load the folders we already know of:
    LOG_INFO( "Checking for modifications in ", folder.path() );
    auto subFoldersInDB = Folder::fetchAll( m_dbConn, parentFolder.id() );
    for ( const auto& subFolderPath : folder.dirs() )
    {
        auto subFolder = m_fsFactory->createDirectory( subFolderPath );
        if ( subFolder == nullptr )
            continue;

        auto it = std::find_if( begin( subFoldersInDB ), end( subFoldersInDB ), [subFolderPath](const std::shared_ptr<Folder>& f) {
            return f->path() == subFolderPath;
        });
        // We don't know this folder, it's a new one
        if ( it == end( subFoldersInDB ) )
        {
            // Check if it is blacklisted
            if ( isBlacklisted( *subFolder, blacklist ) == true )
            {
                LOG_INFO( "Ignoring blacklisted folder: ", subFolderPath );
                continue;
            }
            LOG_INFO( "New folder detected: ", subFolderPath );
            addFolder( *subFolder, &parentFolder, blacklist );
            continue;
        }
        auto folderInDb = *it;
        // In any case, check for modifications, as a change related to a mountpoint might
        // not update the folder modification date.
        // Also, relying on the modification date probably isn't portable
        checkSubfolders( *subFolder, *folderInDb, blacklist );
        checkFiles( *subFolder, *folderInDb );
        subFoldersInDB.erase( it );
    }
    // Now all folders we had in DB but haven't seen from the FS must have been deleted.
    for ( auto f : subFoldersInDB )
    {
        LOG_INFO( "Folder ", f->path(), " not found in FS, deleting it" );
        m_ml->deleteFolder( f.get() );
    }
    LOG_INFO( "Done checking subfolders in ", folder.path() );
    return true;
}

void FsDiscoverer::checkFiles( fs::IDirectory& folder, Folder& parentFolder ) const
{
    LOG_INFO( "Checking file in ", folder.path() );
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name
            + " WHERE folder_id = ?";
    auto files = Media::fetchAll<Media>( m_dbConn, req, parentFolder.id() );
    for ( const auto& filePath : folder.files() )
    {        
        auto it = std::find_if( begin( files ), end( files ), [filePath](const std::shared_ptr<IMedia>& f) {
            return f->mrl() == filePath;
        });
        if ( it == end( files ) )
        {
            m_ml->addFile( filePath, parentFolder, folder );
            continue;
        }
        auto file = m_fsFactory->createFile( filePath );
        if ( file->lastModificationDate() == (*it)->lastModificationDate() )
        {
            // Unchanged file
            files.erase( it );
            continue;
        }
        LOG_INFO( "Forcing file refresh ", filePath );
        m_ml->deleteFile( (*it).get() );
        m_ml->addFile( filePath, parentFolder, folder );
        files.erase( it );
    }
    for ( auto file : files )
    {
        LOG_INFO( "File ", file->mrl(), " not found on filesystem, deleting it" );
        m_ml->deleteFile( file.get() );
    }
    LOG_INFO( "Done checking files ", folder.path() );
}

std::vector<std::shared_ptr<Folder> > FsDiscoverer::blacklist() const
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name + " WHERE is_blacklisted = 1";
    return sqlite::Tools::fetchAll<Folder, Folder>( m_dbConn, req );
}

bool FsDiscoverer::isBlacklisted( const fs::IDirectory& directory, const std::vector<std::shared_ptr<Folder>>& blacklist ) const
{
    auto deviceFs = directory.device();
    auto device = Device::fromUuid( m_dbConn, deviceFs->uuid() );
    // When blacklisting, we would insert the device if we haven't encoutered it yet.
    // So when reading, a missing device means a non-blacklisted device.
    if ( device == nullptr )
        return false;
    auto deviceId = device->id();

    return std::find_if( begin( blacklist ), end( blacklist ), [&directory, deviceId]( const std::shared_ptr<Folder>& f ) {
        return f->path() == directory.path() && f->deviceId() == deviceId;
    }) != end( blacklist );
}

bool FsDiscoverer::addFolder( fs::IDirectory& folder, Folder* parentFolder, const std::vector<std::shared_ptr<Folder>>& blacklist ) const
{
    auto deviceFs = folder.device();
    // We are creating a folder, there has to be a device containing it.
    assert( deviceFs != nullptr );
    auto device = Device::fromUuid( m_dbConn, deviceFs->uuid() );
    if ( device == nullptr )
    {
        LOG_INFO( "Creating new device in DB ", deviceFs->uuid() );
        device = Device::create( m_dbConn, deviceFs->uuid(), deviceFs->isRemovable() );
    }

    auto f = Folder::create( m_dbConn, folder.path(),
                             parentFolder != nullptr ? parentFolder->id() : 0,
                             *device, *deviceFs );
    if ( f == nullptr )
        return false;
    checkFiles( folder, *f );
    checkSubfolders( folder, *f, blacklist );
    return true;
}

