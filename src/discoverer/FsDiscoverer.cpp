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
#include "File.h"
#include "Device.h"
#include "Folder.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "utils/Filename.h"

FsDiscoverer::FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory, MediaLibrary* ml, DBConnection dbConn )
    : m_ml( ml )
    , m_dbConn( dbConn )
    , m_fsFactory( fsFactory )
{
}

bool FsDiscoverer::discover( const std::string &entryPoint )
{
    LOG_INFO( "Adding to discovery list: ", entryPoint );
    // Assume :// denotes a scheme that isn't a file path, and refuse to discover it.
    if ( entryPoint.find( "://" ) != std::string::npos )
        return false;

    std::shared_ptr<fs::IDirectory> fsDir = m_fsFactory->createDirectory( entryPoint );
    // Otherwise, create a directory and check it for modifications
    if ( fsDir == nullptr )
    {
        LOG_ERROR("Failed to create an IDirectory for ", entryPoint );
        return false;
    }
    auto f = Folder::fromPath( m_dbConn, fsDir->path() );
    // If the folder exists, we assume it will be handled by reload()
    if ( f != nullptr )
        return true;
    auto blist = blacklist();
    if ( isBlacklisted( *fsDir, blist ) == true )
        return false;
    if ( hasDotNoMediaFile( *fsDir ) )
        return false;
    return addFolder( *fsDir, nullptr, blist );
}

void FsDiscoverer::reload()
{
    LOG_INFO( "Reloading all folders" );
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
            m_ml->deleteFolder( *f );
            continue;
        }
        checkFolder( *folder, *f, blist );
    }
}

void FsDiscoverer::reload( const std::string& entryPoint )
{
    LOG_INFO( "Reloading folder ", entryPoint );
    auto folder = Folder::fromPath( m_dbConn, entryPoint );
    if ( folder == nullptr )
    {
        LOG_ERROR( "Can't reload ", entryPoint, ": folder wasn't found in database" );
        return;
    }
    auto folderFs = m_fsFactory->createDirectory( folder->path() );
    if ( folderFs == nullptr )
    {
        LOG_ERROR(" Failed to create a fs::IDirectory representing ", folder->path() );
    }
    auto blist = blacklist();
    checkFolder( *folderFs, *folder, blist );
}

void FsDiscoverer::checkDevices()
{
    m_fsFactory->refresh();
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

void FsDiscoverer::checkFolder( fs::IDirectory& currentFolderFs, Folder& currentFolder, const std::vector<std::shared_ptr<Folder>> blacklist ) const
{
    // We already know of this folder, though it may now contain a .nomedia file.
    // In this case, simply delete the folder.
    if ( hasDotNoMediaFile( currentFolderFs ) )
    {
        LOG_INFO( "Deleting folder ", currentFolderFs.path(), " due to a .nomedia file" );
        m_ml->deleteFolder( currentFolder );
        return;
    }
    // Load the folders we already know of:
    LOG_INFO( "Checking for modifications in ", currentFolderFs.path() );
    auto subFoldersInDB = Folder::fetchAll( m_dbConn, currentFolder.id() );
    for ( const auto& subFolderPath : currentFolderFs.dirs() )
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
            if ( hasDotNoMediaFile( *subFolder ) )
            {
                LOG_INFO( "Ignoring folder with a .nomedia file" );
                continue;
            }
            LOG_INFO( "New folder detected: ", subFolderPath );
            addFolder( *subFolder, &currentFolder, blacklist );
            continue;
        }
        auto folderInDb = *it;
        // In any case, check for modifications, as a change related to a mountpoint might
        // not update the folder modification date.
        // Also, relying on the modification date probably isn't portable
        checkFolder( *subFolder, *folderInDb, blacklist );
        subFoldersInDB.erase( it );
    }
    // Now all folders we had in DB but haven't seen from the FS must have been deleted.
    for ( auto f : subFoldersInDB )
    {
        LOG_INFO( "Folder ", f->path(), " not found in FS, deleting it" );
        m_ml->deleteFolder( *f );
    }
    checkFiles( currentFolderFs, currentFolder );
    LOG_INFO( "Done checking subfolders in ", currentFolderFs.path() );
}

void FsDiscoverer::checkFiles( fs::IDirectory& parentFolderFs, Folder& parentFolder ) const
{
    LOG_INFO( "Checking file in ", parentFolderFs.path() );
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE folder_id = ?";
    auto files = File::fetchAll<File>( m_dbConn, req, parentFolder.id() );
    std::vector<std::string> filesToAdd;
    std::vector<std::shared_ptr<File>> filesToRemove;
    for ( const auto& filePath : parentFolderFs.files() )
    {        
        auto it = std::find_if( begin( files ), end( files ), [filePath](const std::shared_ptr<File>& f) {
            return f->mrl() == filePath;
        });
        if ( it == end( files ) )
        {
            filesToAdd.push_back( filePath );
            continue;
        }
        auto fileFs = m_fsFactory->createFile( filePath );
        if ( fileFs->lastModificationDate() == (*it)->lastModificationDate() )
        {
            // Unchanged file
            files.erase( it );
            continue;
        }
        auto& file = (*it);
        LOG_INFO( "Forcing file refresh ", filePath );
        // Pre-cache the file's media, since we need it to remove. However, better doing it
        // out of a write context, since that way, other threads can also read the database.
        file->media();
        filesToRemove.push_back( file );
        filesToAdd.push_back( filePath );
        files.erase( it );
    }
    auto t = m_dbConn->newTransaction();
    for ( auto file : files )
    {
        LOG_INFO( "File ", file->mrl(), " not found on filesystem, deleting it" );
        file->media()->removeFile( *file );
    }
    for ( auto& f : filesToRemove )
        f->media()->removeFile( *f );
    // Insert all files at once to avoid SQL write contention
    for ( auto& p : filesToAdd )
        m_ml->addFile( p, parentFolder, parentFolderFs );
    t->commit();
    LOG_INFO( "Done checking files in ", parentFolderFs.path() );
}

std::vector<std::shared_ptr<Folder> > FsDiscoverer::blacklist() const
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name + " WHERE is_blacklisted = 1";
    return sqlite::Tools::fetchAll<Folder, Folder>( m_dbConn, req );
}

bool FsDiscoverer::isBlacklisted( const fs::IDirectory& directory, const std::vector<std::shared_ptr<Folder>>& blacklist ) const
{
    auto deviceFs = directory.device();
    //FIXME: We could avoid fetching the device if the directory is non removable.
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

bool FsDiscoverer::hasDotNoMediaFile( const fs::IDirectory& directory )
{
    const auto& files = directory.files();
    return std::find_if( begin( files ), end( files ), []( const std::string& filePath ){
        constexpr unsigned int endLength = strlen( "/.nomedia" );
        return filePath.compare( filePath.length() - endLength, endLength, "/.nomedia" ) == 0;
    }) != end( files );
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
    checkFolder( folder, *f, blacklist );
    return true;
}

