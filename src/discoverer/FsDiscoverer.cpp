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
#include "Media.h"
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
        m_fsFactory.reset( new factory::FileSystemDefaultFactory );
}

bool FsDiscoverer::discover( const std::string &entryPoint )
{
    // Assume :// denotes a scheme that isn't a file path, and refuse to discover it.
    if ( entryPoint.find( "://" ) != std::string::npos )
        return false;

    {
        auto f = Folder::fetch( m_dbConn, entryPoint );
        // If the folder exists, we assume it is up to date
        if ( f != nullptr )
            return true;
    }
    // Otherwise, create a directory, and check it for modifications
    std::unique_ptr<fs::IDirectory> fsDir;
    try
    {
        fsDir = m_fsFactory->createDirectory( entryPoint );
    }
    catch (std::exception& ex)
    {
        LOG_ERROR("Failed to create an IDirectory for ", entryPoint, ": ", ex.what());
        return false;
    }
    // Force <0> as lastModificationDate, so this folder is detected as outdated
    // by the modification checking code
    auto f = Folder::create( m_dbConn, fsDir->path(), 0, fsDir->isRemovable(), 0 );
    if ( f == nullptr )
        return false;
    checkFiles( fsDir.get(), f );
    checkSubfolders( fsDir.get(), f );
    f->setLastModificationDate( fsDir->lastModificationDate() );
    return true;
}

void FsDiscoverer::reload()
{
    //FIXME: This should probably be in a sql transaction
    //FIXME: This shouldn't be done for "removable"/network files
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE id_parent IS NULL";
    auto rootFolders = Folder::fetchAll( m_dbConn, req );
    for ( const auto& it : rootFolders )
    {
        auto f = std::static_pointer_cast<Folder>( it );
        auto folder = m_fsFactory->createDirectory( f->path() );
        if ( folder->lastModificationDate() == f->lastModificationDate() )
            continue;
        checkSubfolders( folder.get(), f );
        checkFiles( folder.get(), f );
        f->setLastModificationDate( folder->lastModificationDate() );
    }
}

bool FsDiscoverer::checkSubfolders( fs::IDirectory* folder, FolderPtr parentFolder )
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
    auto subFoldersInDB = Folder::fetchAll( m_dbConn, req, parentFolder->id() );
    for ( const auto& subFolderPath : folder->dirs() )
    {
        auto subFolder = m_fsFactory->createDirectory( subFolderPath );

        auto it = std::find_if( begin( subFoldersInDB ), end( subFoldersInDB ), [subFolderPath](const std::shared_ptr<IFolder>& f) {
            return f->path() == subFolderPath;
        });
        // We don't know this folder, it's a new one
        if ( it == end( subFoldersInDB ) )
        {
            LOG_INFO( "New folder detected: ", subFolderPath );
            // Force a scan by setting lastModificationDate to 0
            auto f = Folder::create( m_dbConn, subFolder->path(), 0, subFolder->isRemovable(), parentFolder->id() );
            checkFiles( subFolder.get(), f );
            checkSubfolders( subFolder.get(), f );
            f->setLastModificationDate( subFolder->lastModificationDate() );
            continue;
        }
        auto folderInDb = std::static_pointer_cast<Folder>( *it );
        if ( subFolder->lastModificationDate() == folderInDb->lastModificationDate() )
        {
            // Remove all folders that still exist in FS. That way, the list of folders that
            // will still be in subFoldersInDB when we're done is the list of folders that have
            // been deleted from the FS
            subFoldersInDB.erase( it );
            continue;
        }
        // This folder was modified, let's recurse
        checkSubfolders( subFolder.get(), folderInDb );
        checkFiles( subFolder.get(), folderInDb );
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

void FsDiscoverer::checkFiles( fs::IDirectory* folder, FolderPtr parentFolder )
{
    LOG_INFO( "Checking file in ", folder->path() );
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name
            + " WHERE folder_id = ?";
    auto files = Media::fetchAll( m_dbConn, req, parentFolder->id() );
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
        auto fileInDb = std::static_pointer_cast<Media>( *it );
        if ( file->lastModificationDate() == fileInDb->lastModificationDate() )
        {
            // Unchanged file
            files.erase( it );
            continue;
        }
        LOG_INFO( "Forcing file refresh ", filePath );
        m_ml->deleteFile( filePath );
        m_ml->addFile( filePath, parentFolder );
        files.erase( it );
    }
    for ( auto file : files )
    {
        LOG_INFO( "File ", file->mrl(), " not found on filesystem, deleting it" );
        m_ml->deleteFile( file );
    }
}
