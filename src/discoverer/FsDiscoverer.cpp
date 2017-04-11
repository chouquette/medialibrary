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

#include "FsDiscoverer.h"

#include <algorithm>
#include <queue>

#include "factory/FileSystemFactory.h"
#include "filesystem/IDevice.h"
#include "Media.h"
#include "File.h"
#include "Device.h"
#include "Folder.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "utils/Filename.h"

namespace medialibrary
{

FsDiscoverer::FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory, MediaLibrary* ml, IMediaLibraryCb* cb )
    : m_ml( ml )
    , m_fsFactory( fsFactory )
    , m_cb( cb )
{
}

bool FsDiscoverer::discover( const std::string &entryPoint )
{
    LOG_INFO( "Adding to discovery list: ", entryPoint );

    if ( m_fsFactory->isMrlSupported( entryPoint ) == false )
        return false;

    std::shared_ptr<fs::IDirectory> fsDir = m_fsFactory->createDirectory( entryPoint );
    // Otherwise, create a directory and check it for modifications
    if ( fsDir == nullptr )
    {
        LOG_ERROR("Failed to create an IDirectory for ", entryPoint );
        return false;
    }
    auto f = Folder::fromMrl( m_ml, fsDir->mrl() );
    // If the folder exists, we assume it will be handled by reload()
    if ( f != nullptr )
        return true;
    try
    {
        if ( hasDotNoMediaFile( *fsDir ) )
            return true;
        return addFolder( *fsDir, nullptr );
    }
    catch ( std::system_error& ex )
    {
        LOG_WARN( entryPoint, " discovery aborted because of a filesystem error: ", ex.what() );
    }

    catch ( sqlite::errors::ConstraintViolation& ex )
    {
        LOG_WARN( entryPoint, " discovery aborted (assuming blacklisted folder): ", ex.what() );
    }
    return true;
}

bool FsDiscoverer::reload()
{
    LOG_INFO( "Reloading all folders" );
    // Start by checking if previously known devices have been plugged/unplugged
    if ( checkDevices() == false )
    {
        LOG_ERROR( "Refusing to reloading files with no storage device" );
        return false;
    }
    auto rootFolders = Folder::fetchRootFolders( m_ml );
    for ( const auto& f : rootFolders )
    {
        auto folder = m_fsFactory->createDirectory( f->mrl() );
        if ( folder == nullptr )
        {
            LOG_INFO( "Removing folder ", f->mrl() );
            m_ml->deleteFolder( *f );
            continue;
        }
        checkFolder( *folder, *f, false );
    }
    return true;
}

bool FsDiscoverer::reload( const std::string& entryPoint )
{
    if ( m_fsFactory->isMrlSupported( entryPoint ) == false )
        return false;
    LOG_INFO( "Reloading folder ", entryPoint );
    // Start by checking if previously known devices have been plugged/unplugged
    if ( checkDevices() == false )
    {
        LOG_ERROR( "Refusing to reloading files with no storage device" );
        return false;
    }
    auto folder = Folder::fromMrl( m_ml, entryPoint );
    if ( folder == nullptr )
    {
        LOG_ERROR( "Can't reload ", entryPoint, ": folder wasn't found in database" );
        return false;
    }
    auto folderFs = m_fsFactory->createDirectory( folder->mrl() );
    if ( folderFs == nullptr )
    {
        LOG_ERROR(" Failed to create a fs::IDirectory representing ", folder->mrl() );
        return false;
    }
    checkFolder( *folderFs, *folder, false );
    return true;
}

bool FsDiscoverer::checkDevices()
{
    if ( m_fsFactory->refreshDevices() == false )
        return false;
    auto devices = Device::fetchAll( m_ml );
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
    return true;
}

void FsDiscoverer::checkFolder( fs::IDirectory& currentFolderFs, Folder& currentFolder, bool newFolder ) const
{
    try
    {
        // We already know of this folder, though it may now contain a .nomedia file.
        // In this case, simply delete the folder.
        if ( hasDotNoMediaFile( currentFolderFs ) )
        {
            if ( newFolder == false )
            {
                LOG_INFO( "Deleting folder ", currentFolderFs.mrl(), " due to a .nomedia file" );
                m_ml->deleteFolder( currentFolder );
            }
            return;
        }
    }
    // Only check once for a system_error. They are bound to happen when we list the files/folders
    // within, and hasDotMediaFile is the first place when this is done (except in case the root
    // entry point fails to be browsed, in which case the failure would happen before)
    catch ( std::system_error& ex )
    {
        LOG_WARN( "Failed to browse ", currentFolderFs.mrl(), ": ", ex.what() );
        if ( newFolder == false )
        {
            // If we ever came across this folder, its content is now unaccessible: let's remove it.
            m_ml->deleteFolder( currentFolder );
        }
        return;
    }

    m_cb->onDiscoveryProgress( currentFolderFs.mrl() );
    // Load the folders we already know of:
    LOG_INFO( "Checking for modifications in ", currentFolderFs.mrl() );
    // Don't try to fetch any potential sub folders if the folder was freshly added
    std::vector<std::shared_ptr<Folder>> subFoldersInDB;
    if ( newFolder == false )
        subFoldersInDB = currentFolder.folders();
    for ( const auto& subFolder : currentFolderFs.dirs() )
    {
        auto it = std::find_if( begin( subFoldersInDB ), end( subFoldersInDB ), [&subFolder](const std::shared_ptr<Folder>& f) {
            return f->mrl() == subFolder->mrl();
        });
        // We don't know this folder, it's a new one
        if ( it == end( subFoldersInDB ) )
        {
            if ( hasDotNoMediaFile( *subFolder ) )
            {
                LOG_INFO( "Ignoring folder with a .nomedia file" );
                continue;
            }
            LOG_INFO( "New folder detected: ", subFolder->mrl() );
            try
            {
                addFolder( *subFolder, &currentFolder );
                continue;
            }
            catch ( sqlite::errors::ConstraintViolation& ex )
            {
                // Best attempt to detect a foreign key violation, indicating the parent folders have been
                // deleted due to blacklisting
                if ( strstr( ex.what(), "foreign key" ) != NULL )
                {
                    LOG_WARN( "Creation of a folder failed because the parent is non existing: ", ex.what(),
                              ". Assuming it was deleted due to blacklisting" );
                    return;
                }
                LOG_WARN( "Creation of a duplicated folder failed: ", ex.what(), ". Assuming it was blacklisted" );
                continue;
            }
        }
        auto folderInDb = *it;
        // In any case, check for modifications, as a change related to a mountpoint might
        // not update the folder modification date.
        // Also, relying on the modification date probably isn't portable
        checkFolder( *subFolder, *folderInDb, false );
        subFoldersInDB.erase( it );
    }
    // Now all folders we had in DB but haven't seen from the FS must have been deleted.
    for ( auto f : subFoldersInDB )
    {
        LOG_INFO( "Folder ", f->mrl(), " not found in FS, deleting it" );
        m_ml->deleteFolder( *f );
    }
    checkFiles( currentFolderFs, currentFolder );
    LOG_INFO( "Done checking subfolders in ", currentFolderFs.mrl() );
}

void FsDiscoverer::checkFiles( fs::IDirectory& parentFolderFs, Folder& parentFolder ) const
{
    LOG_INFO( "Checking file in ", parentFolderFs.mrl() );
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE folder_id = ?";
    auto files = File::fetchAll<File>( m_ml, req, parentFolder.id() );
    std::vector<std::shared_ptr<fs::IFile>> filesToAdd;
    std::vector<std::shared_ptr<File>> filesToRemove;
    for ( const auto& fileFs: parentFolderFs.files() )
    {
        auto it = std::find_if( begin( files ), end( files ), [fileFs](const std::shared_ptr<File>& f) {
            return f->mrl() == fileFs->mrl();
        });
        if ( it == end( files ) )
        {
            filesToAdd.push_back( std::move( fileFs ) );
            continue;
        }
        if ( fileFs->lastModificationDate() == (*it)->lastModificationDate() )
        {
            // Unchanged file
            files.erase( it );
            continue;
        }
        auto& file = (*it);
        LOG_INFO( "Forcing file refresh ", fileFs->mrl() );
        // Pre-cache the file's media, since we need it to remove. However, better doing it
        // out of a write context, since that way, other threads can also read the database.
        file->media();
        filesToRemove.push_back( std::move( file ) );
        filesToAdd.push_back( std::move( fileFs ) );
        files.erase( it );
    }
    auto t = m_ml->getConn()->newTransaction();
    for ( auto file : files )
    {
        LOG_INFO( "File ", file->mrl(), " not found on filesystem, deleting it" );
        file->media()->removeFile( *file );
    }
    for ( auto& f : filesToRemove )
        f->media()->removeFile( *f );
    // Insert all files at once to avoid SQL write contention
    for ( auto& p : filesToAdd )
        m_ml->addFile( *p, parentFolder, parentFolderFs );
    t->commit();
    LOG_INFO( "Done checking files in ", parentFolderFs.mrl() );
}

bool FsDiscoverer::hasDotNoMediaFile( const fs::IDirectory& directory )
{
    const auto& files = directory.files();
    return std::find_if( begin( files ), end( files ), []( const std::shared_ptr<fs::IFile>& file ){
        return file->name() == ".nomedia";
    }) != end( files );
}

bool FsDiscoverer::addFolder( fs::IDirectory& folder, Folder* parentFolder ) const
{
    auto deviceFs = folder.device();
    // We are creating a folder, there has to be a device containing it.
    assert( deviceFs != nullptr );
    auto device = Device::fromUuid( m_ml, deviceFs->uuid() );
    if ( device == nullptr )
    {
        LOG_INFO( "Creating new device in DB ", deviceFs->uuid() );
        device = Device::create( m_ml, deviceFs->uuid(), utils::file::scheme( folder.mrl() ),
                                 deviceFs->isRemovable() );
    }

    auto f = Folder::create( m_ml, folder.mrl(),
                             parentFolder != nullptr ? parentFolder->id() : 0,
                             *device, *deviceFs );
    if ( f == nullptr )
        return false;
    checkFolder( folder, *f, true );
    return true;
}

}
