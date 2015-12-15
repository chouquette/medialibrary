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

    {
        auto f = Folder::fromPath( m_dbConn, entryPoint );
        // If the folder exists, we assume it will be handled by reload()
        if ( f != nullptr )
            return true;
    }
    // Otherwise, create a directory, and check it for modifications
    std::shared_ptr<fs::IDirectory> fsDir;
    try
    {
        fsDir = m_fsFactory->createDirectory( entryPoint );
    }
    catch (std::exception& ex)
    {
        LOG_ERROR("Failed to create an IDirectory for ", entryPoint, ": ", ex.what());
        return false;
    }
    auto blist = blacklist();
    if ( isBlacklisted( fsDir->path(), blist ) == true )
        return false;
    // Force <0> as lastModificationDate, so this folder is detected as outdated
    // by the modification checking code
    auto f = Folder::create( m_dbConn, fsDir->path(), 0, fsDir->isRemovable(), 0 );
    if ( f == nullptr )
        return false;
    // Since this is the "root" folder, we will always try to add the device it's located on
    // to our devices list. We can then check if the subfolders containing device are different
    auto deviceFs = fsDir->device();
    auto device = Device::fromUuid( m_dbConn, deviceFs->uuid() );
    if ( device == nullptr )
    {
        LOG_INFO( "Creating new device in DB ", deviceFs->uuid() );
        device = Device::create( m_dbConn, deviceFs->uuid(), deviceFs->isRemovable() );
    }
    checkFiles( fsDir.get(), f.get() );
    checkSubfolders( fsDir.get(), f.get(), blist );
    f->setLastModificationDate( fsDir->lastModificationDate() );
    return true;
}

void FsDiscoverer::reload()
{
    //FIXME: This should probably be in a sql transaction
    //FIXME: This shouldn't be done for "removable"/network files
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE id_parent IS NULL";
    auto rootFolders = Folder::fetchAll<Folder>( m_dbConn, req );
    auto blist = blacklist();
    for ( const auto& f : rootFolders )
    {
        auto folder = m_fsFactory->createDirectory( f->path() );
        if ( folder->lastModificationDate() == f->lastModificationDate() )
            continue;
        checkSubfolders( folder.get(), f.get(), blist );
        checkFiles( folder.get(), f.get() );
        f->setLastModificationDate( folder->lastModificationDate() );
    }
}

bool FsDiscoverer::checkSubfolders( fs::IDirectory* folder, Folder* parentFolder, const std::vector<std::shared_ptr<Folder>> blacklist )
{
    // From here we can have:
    // - New subfolder(s)
    // - Deleted subfolder(s)
    // - New file(s)
    // - Deleted file(s)
    // - Changed file(s)
    // ... in this folder, or in all the sub folders.

    // Load the folders we already know of:
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE id_parent = ?";
    LOG_INFO( "Checking for modifications in ", folder->path() );
    auto subFoldersInDB = Folder::fetchAll<Folder>( m_dbConn, req, parentFolder->id() );
    for ( const auto& subFolderPath : folder->dirs() )
    {
        auto subFolder = m_fsFactory->createDirectory( subFolderPath );

        auto it = std::find_if( begin( subFoldersInDB ), end( subFoldersInDB ), [subFolderPath](const std::shared_ptr<IFolder>& f) {
            return f->path() == subFolderPath;
        });
        // We don't know this folder, it's a new one
        if ( it == end( subFoldersInDB ) )
        {
            // Check if it is blacklisted
            if ( isBlacklisted( subFolderPath, blacklist ) == true )
            {
                LOG_INFO( "Ignoring blacklisted folder: ", subFolderPath );
                continue;
            }
            LOG_INFO( "New folder detected: ", subFolderPath );
            // Force a scan by setting lastModificationDate to 0
            auto f = Folder::create( m_dbConn, subFolder->path(), 0, subFolder->isRemovable(), parentFolder->id() );
            checkFiles( subFolder.get(), f.get() );
            checkSubfolders( subFolder.get(), f.get(), blacklist );
            f->setLastModificationDate( subFolder->lastModificationDate() );
            continue;
        }
        auto folderInDb = *it;
        if ( subFolder->lastModificationDate() == folderInDb->lastModificationDate() )
        {
            // Remove all folders that still exist in FS. That way, the list of folders that
            // will still be in subFoldersInDB when we're done is the list of folders that have
            // been deleted from the FS
            subFoldersInDB.erase( it );
            continue;
        }
        // This folder was modified, let's recurse
        checkSubfolders( subFolder.get(), folderInDb.get(), blacklist );
        checkFiles( subFolder.get(), folderInDb.get() );
        folderInDb->setLastModificationDate( subFolder->lastModificationDate() );
        subFoldersInDB.erase( it );
    }
    // Now all folders we had in DB but haven't seen from the FS must have been deleted.
    for ( auto f : subFoldersInDB )
    {
        LOG_INFO( "Folder ", f->path(), " not found in FS, deleting it" );
        m_ml->deleteFolder( f );
    }
    return true;
}

void FsDiscoverer::checkFiles( fs::IDirectory* folder, Folder* parentFolder )
{
    LOG_INFO( "Checking file in ", folder->path() );
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name
            + " WHERE folder_id = ?";
    auto files = Media::fetchAll<Media>( m_dbConn, req, parentFolder->id() );
    for ( const auto& filePath : folder->files() )
    {        
        auto it = std::find_if( begin( files ), end( files ), [filePath](const std::shared_ptr<IMedia>& f) {
            return f->mrl() == filePath;
        });
        if ( it == end( files ) )
        {
            m_ml->addFile( filePath, parentFolder );
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
        m_ml->addFile( filePath, parentFolder );
        files.erase( it );
    }
    for ( auto file : files )
    {
        LOG_INFO( "File ", file->mrl(), " not found on filesystem, deleting it" );
        m_ml->deleteFile( file.get() );
    }
}

std::vector<std::shared_ptr<Folder> > FsDiscoverer::blacklist() const
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name + " WHERE is_blacklisted = 1";
    return sqlite::Tools::fetchAll<Folder, Folder>( m_dbConn, req );
}

bool FsDiscoverer::isBlacklisted(const std::string& path, const std::vector<std::shared_ptr<Folder>>& blacklist ) const
{
    return std::find_if( begin( blacklist ), end( blacklist ), [&path]( const std::shared_ptr<Folder>& f ) {
        return path == f->path();
    }) != end( blacklist );
}
