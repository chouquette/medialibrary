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
#include <utility>

#include "factory/FileSystemFactory.h"
#include "filesystem/IDevice.h"
#include "Media.h"
#include "File.h"
#include "Device.h"
#include "Folder.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "utils/Filename.h"

namespace
{

class DeviceRemovedException : public std::runtime_error
{
public:
    DeviceRemovedException() noexcept
        : std::runtime_error( "A device was removed during the discovery" )
    {
    }
};

}

namespace medialibrary
{

FsDiscoverer::FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory, MediaLibrary* ml, IMediaLibraryCb* cb )
    : m_ml( ml )
    , m_fsFactory( std::move( fsFactory ))
    , m_cb( cb )
{
}

bool FsDiscoverer::discover( const std::string &entryPoint )
{
    LOG_INFO( "Adding to discovery list: ", entryPoint );

    if ( m_fsFactory->isMrlSupported( entryPoint ) == false )
        return false;

    std::shared_ptr<fs::IDirectory> fsDir = m_fsFactory->createDirectory( entryPoint );
    auto f = Folder::fromMrl( m_ml, fsDir->mrl() );
    // If the folder exists, we assume it will be handled by reload()
    if ( f != nullptr )
        return true;
    try
    {
        if ( hasDotNoMediaFile( *fsDir ) )
            return true;
        return addFolder( std::move( fsDir ), nullptr );
    }
    catch ( std::system_error& ex )
    {
        LOG_WARN( entryPoint, " discovery aborted because of a filesystem error: ", ex.what() );
    }
    catch ( sqlite::errors::ConstraintViolation& ex )
    {
        LOG_WARN( entryPoint, " discovery aborted (assuming blacklisted folder): ", ex.what() );
    }
    catch ( DeviceRemovedException& )
    {
        // Simply ignore, the device has already been marked as removed and the DB updated accordingly
        LOG_INFO( "Discovery of ", fsDir->mrl(), " was stopped after the device was removed" );
    }
    return true;
}

void FsDiscoverer::reloadFolder( std::shared_ptr<Folder> f )
{
    auto folder = m_fsFactory->createDirectory( f->mrl() );
    try
    {
        checkFolder( std::move( folder ), std::move( f ), false );
    }
    catch ( DeviceRemovedException& )
    {
        LOG_INFO( "Reloading of ", f->mrl(), " was stopped after the device was removed" );
    }
}

bool FsDiscoverer::reload()
{
    LOG_INFO( "Reloading all folders" );
    auto rootFolders = Folder::fetchRootFolders( m_ml );
    for ( const auto& f : rootFolders )
        reloadFolder( f );
    return true;
}

bool FsDiscoverer::reload( const std::string& entryPoint )
{
    if ( m_fsFactory->isMrlSupported( entryPoint ) == false )
        return false;
    LOG_INFO( "Reloading folder ", entryPoint );
    auto folder = Folder::fromMrl( m_ml, entryPoint );
    if ( folder == nullptr )
    {
        LOG_ERROR( "Can't reload ", entryPoint, ": folder wasn't found in database" );
        return false;
    }
    reloadFolder( std::move( folder ) );
    return true;
}

void FsDiscoverer::checkFolder( std::shared_ptr<fs::IDirectory> currentFolderFs,
                                std::shared_ptr<Folder> currentFolder,
                                bool newFolder ) const
{
    try
    {
        // We already know of this folder, though it may now contain a .nomedia file.
        // In this case, simply delete the folder.
        if ( hasDotNoMediaFile( *currentFolderFs ) )
        {
            if ( newFolder == false )
            {
                LOG_INFO( "Deleting folder ", currentFolderFs->mrl(), " due to a .nomedia file" );
                m_ml->deleteFolder( *currentFolder );
            }
            else
                LOG_INFO( "Ignoring folder ", currentFolderFs->mrl(), " due to a .nomedia file" );
            return;
        }
    }
    // Only check once for a system_error. They are bound to happen when we list the files/folders
    // within, and hasDotMediaFile is the first place when this is done
    catch ( std::system_error& ex )
    {
        LOG_WARN( "Failed to browse ", currentFolderFs->mrl(), ": ", ex.what() );
        // Even when we're discovering a new folder, we want to rule out device removal as the cause of
        // an IO error. If this is the cause, simply abort the discovery. All the folder we have
        // discovered so far will be marked as non-present through sqlite hooks, and we'll resume the
        // discovery when the device gets plugged back in
        if ( currentFolderFs->device()->isRemovable() )
        {
            // If the device is removable, check if it was indeed removed.
            LOG_INFO( "The device containing ", currentFolderFs->mrl(), " is removable. Checking for device removal..." );
            m_ml->refreshDevices( *m_fsFactory );
            // The device presence flag will be changed in place, so simply retest it
            if ( currentFolderFs->device()->isPresent() == false )
                throw DeviceRemovedException();
            LOG_INFO( "Device was not removed" );
        }
        // However if the device isn't removable, we want to:
        // - ignore it when we're discovering a new folder.
        // - delete it when it was discovered in the past. This is likely to be due to a permission change
        //   as we would not check the folder if it wasn't present during the parent folder browsing
        //   but it might also be that we're checking an entry point.
        //   The error won't arise earlier, as we only perform IO when reading the folder from this function.
        if ( newFolder == false )
        {
            // If we ever came across this folder, its content is now unaccessible: let's remove it.
            m_ml->deleteFolder( *currentFolder );
        }
        return;
    }

    m_cb->onDiscoveryProgress( currentFolderFs->mrl() );
    // Load the folders we already know of:
    LOG_INFO( "Checking for modifications in ", currentFolderFs->mrl() );
    // Don't try to fetch any potential sub folders if the folder was freshly added
    std::vector<std::shared_ptr<Folder>> subFoldersInDB;
    if ( newFolder == false )
        subFoldersInDB = currentFolder->folders();
    for ( const auto& subFolder : currentFolderFs->dirs() )
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
                addFolder( subFolder, currentFolder.get() );
                continue;
            }
            catch ( sqlite::errors::ConstraintViolation& ex )
            {
                // Best attempt to detect a foreign key violation, indicating the parent folders have been
                // deleted due to blacklisting
                if ( strstr( ex.what(), "foreign key" ) != nullptr )
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
        checkFolder( subFolder, folderInDb, false );
        subFoldersInDB.erase( it );
    }
    // Now all folders we had in DB but haven't seen from the FS must have been deleted.
    for ( const auto& f : subFoldersInDB )
    {
        LOG_INFO( "Folder ", f->mrl(), " not found in FS, deleting it" );
        m_ml->deleteFolder( *f );
    }
    checkFiles( currentFolderFs, currentFolder );
    LOG_INFO( "Done checking subfolders in ", currentFolderFs->mrl() );
}

void FsDiscoverer::checkFiles( std::shared_ptr<fs::IDirectory> parentFolderFs,
                               std::shared_ptr<Folder> parentFolder ) const
{
    LOG_INFO( "Checking file in ", parentFolderFs->mrl() );
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE folder_id = ?";
    auto files = File::fetchAll<File>( m_ml, req, parentFolder->id() );
    std::vector<std::shared_ptr<fs::IFile>> filesToAdd;
    std::vector<std::shared_ptr<File>> filesToRemove;
    for ( const auto& fileFs: parentFolderFs->files() )
    {
        auto it = std::find_if( begin( files ), end( files ), [fileFs](const std::shared_ptr<File>& f) {
            return f->mrl() == fileFs->mrl();
        });
        if ( it == end( files ) )
        {
            if ( MediaLibrary::isExtensionSupported( fileFs->extension().c_str() ) == true )
                filesToAdd.push_back( fileFs );
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
        filesToAdd.push_back( fileFs );
        files.erase( it );
    }
    using FilesT = decltype( files );
    using FilesToRemoveT = decltype( filesToRemove );
    using FilesToAddT = decltype( filesToAdd );
    sqlite::Tools::withRetries( 3, [this, &parentFolder, &parentFolderFs]
                            ( FilesT files, FilesToAddT filesToAdd, FilesToRemoveT filesToRemove ) {
        auto t = m_ml->getConn()->newTransaction();
        for ( auto file : files )
        {
            LOG_INFO( "File ", file->mrl(), " not found on filesystem, deleting it" );
            auto media = file->media();
            if ( media != nullptr && media->isDeleted() == false )
                media->removeFile( *file );
            else if ( file->isDeleted() == false )
            {
                // This is unexpected, as the file should have been deleted when the media was
                // removed.
                LOG_WARN( "Deleting a file without an associated media." );
                file->destroy();
            }
        }
        for ( auto& f : filesToRemove )
        {
            auto media = f->media();
            if ( media != nullptr )
                media->removeFile( *f );
            else
            {
                // If there is no media associated with this file, the file had to be removed through
                // a trigger
                assert( f->isDeleted() );
            }
        }
        // Insert all files at once to avoid SQL write contention
        for ( auto& p : filesToAdd )
            m_ml->addFile( p, parentFolder, parentFolderFs );
        t->commit();
        LOG_INFO( "Done checking files in ", parentFolderFs->mrl() );
    }, std::move( files ), std::move( filesToAdd ), std::move( filesToRemove ) );
}

bool FsDiscoverer::hasDotNoMediaFile( const fs::IDirectory& directory )
{
    const auto& files = directory.files();
    return std::find_if( begin( files ), end( files ), []( const std::shared_ptr<fs::IFile>& file ){
        return strcasecmp( file->name().c_str(), ".nomedia" ) == 0;
    }) != end( files );
}

bool FsDiscoverer::addFolder( std::shared_ptr<fs::IDirectory> folder,
                              Folder* parentFolder ) const
{
    auto deviceFs = folder->device();
    // We are creating a folder, there has to be a device containing it.
    assert( deviceFs != nullptr );
    auto device = Device::fromUuid( m_ml, deviceFs->uuid() );
    if ( device == nullptr )
    {
        LOG_INFO( "Creating new device in DB ", deviceFs->uuid() );
        device = Device::create( m_ml, deviceFs->uuid(),
                                 utils::file::scheme( folder->mrl() ),
                                 deviceFs->isRemovable() );
        if ( device == nullptr )
            return false;
    }

    auto f = Folder::create( m_ml, folder->mrl(),
                             parentFolder != nullptr ? parentFolder->id() : 0,
                             *device, *deviceFs );
    if ( f == nullptr )
        return false;
    checkFolder( std::move( folder ), std::move( f ), true );
    return true;
}

}
