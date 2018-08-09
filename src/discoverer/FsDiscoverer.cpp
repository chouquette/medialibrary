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
#include "medialibrary/filesystem/IDevice.h"
#include "Media.h"
#include "File.h"
#include "Device.h"
#include "Folder.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "probe/CrawlerProbe.h"
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

FsDiscoverer::FsDiscoverer( std::shared_ptr<fs::IFileSystemFactory> fsFactory, MediaLibrary* ml, IMediaLibraryCb* cb, std::unique_ptr<prober::IProbe> probe )
    : m_ml( ml )
    , m_fsFactory( std::move( fsFactory ))
    , m_cb( cb )
    , m_probe( std::move( probe ) )
{
}

bool FsDiscoverer::discover( const std::string& entryPoint )
{
    LOG_INFO( "Running discover on: ", entryPoint );

    if ( m_fsFactory->isMrlSupported( entryPoint ) == false )
        return false;

    std::shared_ptr<fs::IDirectory> fsDir;
    try
    {
        fsDir = m_fsFactory->createDirectory( entryPoint );
    }
    catch ( std::system_error& ex )
    {
        LOG_WARN( entryPoint, " discovery aborted because of a filesystem error: ", ex.what() );
        return true;
    }
    auto fsDirMrl = fsDir->mrl(); // Saving MRL now since we might need it after fsDir is moved
    auto f = Folder::fromMrl( m_ml, fsDirMrl );
    // If the folder exists, we assume it will be handled by reload()
    if ( f != nullptr )
        return true;
    try
    {
        if ( m_probe->proceedOnDirectory( *fsDir ) == false || m_probe->isHidden( *fsDir ) == true )
            return true;
        // Fetch files explicitly
        fsDir->files();
        return addFolder( std::move( fsDir ), m_probe->getFolderParent().get() );
    }
    catch ( sqlite::errors::ConstraintViolation& ex )
    {
        LOG_WARN( fsDirMrl, " discovery aborted (assuming blacklisted folder): ", ex.what() );
    }
    catch ( DeviceRemovedException& )
    {
        // Simply ignore, the device has already been marked as removed and the DB updated accordingly
        LOG_INFO( "Discovery of ", fsDirMrl, " was stopped after the device was removed" );
    }
    return true;
}

void FsDiscoverer::reloadFolder( std::shared_ptr<Folder> f )
{
    assert( f->isPresent() );
    auto mrl = f->mrl();

    std::shared_ptr<fs::IDirectory> directory;
    try
    {
        directory = m_fsFactory->createDirectory( mrl );
        assert( directory->device() != nullptr );
        if ( directory->device() == nullptr )
            return;
    }
    catch ( const std::system_error& ex )
    {
        LOG_INFO( "Failed to instanciate a directory for ", mrl, ": ", ex.what(),
                  ". Can't reload the folder." );
    }
    if ( directory == nullptr )
    {
        auto device = m_fsFactory->createDeviceFromMrl( mrl );
        if ( device == nullptr || device->isRemovable() == false )
        {
            LOG_INFO( "Failed to find folder matching entrypoint ", mrl, ". "
                      "Removing that folder" );
            m_ml->deleteFolder( *f );
            return;
        }
    }
    try
    {
        checkFolder( std::move( directory ), std::move( f ), false );
    }
    catch ( DeviceRemovedException& )
    {
        LOG_INFO( "Reloading of ", mrl, " was stopped after the device was removed" );
    }

}

bool FsDiscoverer::reload()
{
    LOG_INFO( "Reloading all folders" );
    auto rootFolders = Folder::fetchRootFolders( m_ml );
    for ( const auto& f : rootFolders )
    {
        // fetchRootFolders only returns present folders
        assert( f->isPresent() == true );
        reloadFolder( f );
    }
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
    if ( folder->isPresent() == false )
    {
        LOG_INFO( "Folder ", entryPoint, " isn't present, and therefore won't "
                  "be reloaded" );
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
        if ( m_probe->isHidden( *currentFolderFs ) == true )
        {
            if ( newFolder == false )
                m_ml->deleteFolder( *currentFolder );
            return;
        }
        // Ensuring that the file fetching is done in this scope, to catch errors
        currentFolderFs->files();
    }
    // Only check once for a system_error. They are bound to happen when we list the files/folders
    // within, and IProbe::isHidden is the first place when this is done
    catch ( std::system_error& ex )
    {
        LOG_WARN( "Failed to browse ", currentFolderFs->mrl(), ": ", ex.what() );
        // Even when we're discovering a new folder, we want to rule out device removal as the cause of
        // an IO error. If this is the cause, simply abort the discovery. All the folder we have
        // discovered so far will be marked as non-present through sqlite hooks, and we'll resume the
        // discovery when the device gets plugged back in
        auto device = currentFolderFs->device();
        // The device might not be present at all, and therefor we might miss a
        // representation for it.
        if ( device == nullptr || device->isRemovable() )
        {
            // If the device is removable/missing, check if it was indeed removed.
            LOG_INFO( "The device containing ", currentFolderFs->mrl(), " is ",
                      device != nullptr ? "removable" : "not found",
                      ". Refreshing device cache..." );

            m_ml->refreshDevices( *m_fsFactory );
            // If the device was missing, refresh our list of devices in case
            // the device was plugged back and/or we missed a notification for it
            if ( device == nullptr )
                device = currentFolderFs->device();
            // The device presence flag will be changed in place, so simply retest it
            if ( device == nullptr || device->isPresent() == false )
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

    if ( m_cb != nullptr )
        m_cb->onDiscoveryProgress( currentFolderFs->mrl() );
    // Load the folders we already know of:
    LOG_INFO( "Checking for modifications in ", currentFolderFs->mrl() );
    // Don't try to fetch any potential sub folders if the folder was freshly added
    std::vector<std::shared_ptr<Folder>> subFoldersInDB;
    if ( newFolder == false )
        subFoldersInDB = currentFolder->folders();
    for ( const auto& subFolder : currentFolderFs->dirs() )
    {
        if ( subFolder->device() == nullptr )
            continue;
        if ( m_probe->stopFileDiscovery() == true )
            break;
        if ( m_probe->proceedOnDirectory( *subFolder ) == false )
            continue;
        auto it = std::find_if( begin( subFoldersInDB ), end( subFoldersInDB ), [&subFolder](const std::shared_ptr<Folder>& f) {
            return f->mrl() == subFolder->mrl();
        });
        // We don't know this folder, it's a new one
        if ( it == end( subFoldersInDB ) )
        {
            if ( m_probe->isHidden( *subFolder ) )
                continue;
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
    if ( m_probe->deleteUnseenFolders() == true )
    {
        // Now all folders we had in DB but haven't seen from the FS must have been deleted.
        for ( const auto& f : subFoldersInDB )
        {
            LOG_INFO( "Folder ", f->mrl(), " not found in FS, deleting it" );
            m_ml->deleteFolder( *f );
        }
    }
    checkFiles( currentFolderFs, currentFolder );
    LOG_INFO( "Done checking subfolders in ", currentFolderFs->mrl() );
}

void FsDiscoverer::checkFiles( std::shared_ptr<fs::IDirectory> parentFolderFs,
                               std::shared_ptr<Folder> parentFolder ) const
{
    LOG_INFO( "Checking file in ", parentFolderFs->mrl() );
    static const std::string req = "SELECT * FROM " + File::Table::Name
            + " WHERE folder_id = ?";
    auto files = File::fetchAll<File>( m_ml, req, parentFolder->id() );
    std::vector<std::shared_ptr<fs::IFile>> filesToAdd;
    std::vector<std::pair<std::shared_ptr<File>, std::shared_ptr<fs::IFile>>> filesToRefresh;
    for ( const auto& fileFs: parentFolderFs->files() )
    {
        if ( m_probe->stopFileDiscovery() == true )
            break;
        if ( m_probe->proceedOnFile( *fileFs ) == false )
            continue;
        auto it = std::find_if( begin( files ), end( files ), [fileFs](const std::shared_ptr<File>& f) {
            return f->mrl() == fileFs->mrl();
        });
        if ( it == end( files ) || m_probe->forceFileRefresh() == true )
        {
            if ( MediaLibrary::isExtensionSupported( fileFs->extension().c_str() ) == true )
                filesToAdd.push_back( fileFs );
            continue;
        }
        if ( fileFs->lastModificationDate() != (*it)->lastModificationDate() )
        {
            LOG_INFO( "Forcing file refresh ", fileFs->mrl() );
            filesToRefresh.emplace_back( std::move( *it ), fileFs );
        }
        files.erase( it );
    }
    if ( m_probe->deleteUnseenFiles() == false )
        files.clear();
    using FilesT = decltype( files );
    using FilesToRefreshT = decltype( filesToRefresh );
    using FilesToAddT = decltype( filesToAdd );
    sqlite::Tools::withRetries( 3, [this, &parentFolder, &parentFolderFs]
                            ( FilesT files, FilesToAddT filesToAdd, FilesToRefreshT filesToRefresh ) {
        auto t = m_ml->getConn()->newTransaction();
        for ( const auto& file : files )
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
        for ( auto& p: filesToRefresh )
            m_ml->onUpdatedFile( std::move( p.first ), std::move( p.second ) );
        // Insert all files at once to avoid SQL write contention
        for ( auto& p : filesToAdd )
            m_ml->onDiscoveredFile( p, parentFolder, parentFolderFs, m_probe->getPlaylistParent() );
        t->commit();
        LOG_INFO( "Done checking files in ", parentFolderFs->mrl() );
    }, std::move( files ), std::move( filesToAdd ), std::move( filesToRefresh ) );
}

bool FsDiscoverer::addFolder( std::shared_ptr<fs::IDirectory> folder,
                              Folder* parentFolder ) const
{
    auto deviceFs = folder->device();
    // We are creating a folder, there has to be a device containing it.
    assert( deviceFs != nullptr );
    // But gracefully handle failure in release mode
    if( deviceFs == nullptr )
        return false;
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
