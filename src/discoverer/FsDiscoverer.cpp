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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "FsDiscoverer.h"

#include <algorithm>
#include <utility>

#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/Errors.h"
#include "Media.h"
#include "File.h"
#include "Device.h"
#include "Folder.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "discoverer/probe/IProbe.h"

namespace medialibrary
{

FsDiscoverer::FsDiscoverer( MediaLibrary* ml, IMediaLibraryCb* cb,
                            std::unique_ptr<prober::IProbe> probe )
    : m_ml( ml )
    , m_cb( cb )
    , m_probe( std::move( probe ) )
{
}

bool FsDiscoverer::discover( const std::string& entryPoint,
                             const IInterruptProbe& interruptProbe )
{
    auto fsFactory = m_ml->fsFactoryForMrl( entryPoint );
    if ( fsFactory == nullptr )
        return false;

    std::shared_ptr<fs::IDirectory> fsDir;
    try
    {
        fsDir = fsFactory->createDirectory( entryPoint );
    }
    catch ( const fs::errors::System& ex )
    {
        LOG_WARN( entryPoint, " discovery aborted because of a filesystem error: ", ex.what() );
        return true;
    }
    auto fsDirMrl = fsDir->mrl(); // Saving MRL now since we might need it after fsDir is moved
    auto f = Folder::fromMrl( m_ml, fsDirMrl );
    // If the folder exists, we assume it will be handled by reload()
    if ( f != nullptr )
        return true;
    /*
     * Ensure we have a device containing the folder before trying to probe it
     * IFileSystemFactory::createDirectory doesn't check for the actual mrl
     * existence, so we might be trying to index something that doesn't exist.
     */
    auto deviceFs = fsDir->device();
    if ( deviceFs == nullptr )
    {
        LOG_INFO( "Can't discover ", entryPoint, ": no associated device is "
                  "present" );
        return false;
    }
    try
    {
        if ( m_probe->proceedOnDirectory( *fsDir ) == false || m_probe->isHidden( *fsDir ) == true )
            return true;
        // Fetch files explicitly
        fsDir->files();
        auto res = addFolder( std::move( fsDir ), m_probe->getFolderParent().get(),
                              interruptProbe, *fsFactory );
        m_ml->getCb()->onEntryPointAdded( entryPoint, res );
        return res;
    }
    catch ( sqlite::errors::ConstraintUnique& ex )
    {
        LOG_DEBUG( fsDirMrl, " discovery aborted (assuming banned folder): ", ex.what() );
    }
    catch ( fs::errors::DeviceRemoved& )
    {
        // Simply ignore, the device has already been marked as removed and the DB updated accordingly
        LOG_DEBUG( "Discovery of ", fsDirMrl, " was stopped after the device was removed" );
    }
    return true;
}

bool FsDiscoverer::reloadFolder( std::shared_ptr<Folder> f,
                                 const IInterruptProbe& probe,
                                 fs::IFileSystemFactory& fsFactory )
{
    assert( f->isPresent() );
    auto mrl = f->mrl();

    std::shared_ptr<fs::IDirectory> directory;
    try
    {
        directory = fsFactory.createDirectory( mrl );
        assert( directory->device() != nullptr );
        if ( directory->device() == nullptr )
            return false;
    }
    catch ( const fs::errors::System& ex )
    {
        LOG_INFO( "Failed to instanciate a directory for ", mrl, ": ", ex.what(),
                  ". Can't reload the folder." );
    }
    if ( directory == nullptr )
    {
        auto device = fsFactory.createDeviceFromMrl( mrl );
        if ( device == nullptr || device->isRemovable() == false )
        {
            LOG_DEBUG( "Failed to find folder matching entrypoint ", mrl, ". "
                      "Removing that folder" );
            m_ml->deleteFolder( *f );
            return false;
        }
    }
    try
    {
        checkFolder( std::move( directory ), std::move( f ), probe, fsFactory,
                     false );
    }
    catch ( fs::errors::DeviceRemoved& )
    {
        LOG_INFO( "Reloading of ", mrl, " was stopped after the device was removed" );
        return false;
    }
    return true;
}

void FsDiscoverer::checkRemovedDevices( fs::IDirectory& fsFolder, Folder& folder,
                                        fs::IFileSystemFactory& fsFactory,
                                        bool newFolder ) const
{
    // Even when we're discovering a new folder, we want to rule out device removal as the cause of
    // an IO error. If this is the cause, simply abort the discovery. All the folder we have
    // discovered so far will be marked as non-present through sqlite hooks, and we'll resume the
    // discovery when the device gets plugged back in
    auto device = fsFolder.device();
    // The device might not be present at all, and therefor we might miss a
    // representation for it.
    if ( device == nullptr || device->isRemovable() )
    {
        // If the device is removable/missing, check if it was indeed removed.
        LOG_INFO( "The device containing ", fsFolder.mrl(), " is ",
                  device != nullptr ? "removable" : "not found",
                  ". Refreshing device cache..." );

        fsFactory.refreshDevices();
        m_ml->refreshDevices( fsFactory );
        // If the device was missing, refresh our list of devices in case
        // the device was plugged back and/or we missed a notification for it
        if ( device == nullptr )
            device = fsFolder.device();
        // The device presence flag will be changed in place, so simply retest it
        if ( device == nullptr || device->isPresent() == false )
            throw fs::errors::DeviceRemoved{};
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
        m_ml->deleteFolder( folder );
    }
}

bool FsDiscoverer::reload( const IInterruptProbe& interruptProbe )
{
    LOG_INFO( "Reloading all folders" );
    auto rootFolders = Folder::fetchRootFolders( m_ml );
    for ( const auto& f : rootFolders )
    {
        if ( interruptProbe.isInterrupted() == true )
            break;
        // fetchRootFolders only returns present folders
        assert( f->isPresent() == true );
        std::string mrl;
        try
        {
            mrl = f->mrl();
        }
        catch ( const fs::errors::DeviceRemoved& ex )
        {
            // If the device was removed, let's wait until it comes back, but
            // don't abort all entry points reloading
            LOG_INFO( "Can't reload folder on a removed device" );
            continue;
        }
        catch ( const fs::errors::UnknownScheme& ex )
        {
            // We might have added a folder before, but the FS factory required to
            // handle it has not been inserted yet, or maybe some type of discoveries
            // have been disabled (most likely network discovery)
            // In this case, we must not fail hard, but simply ignore that folder.
            LOG_INFO( "No file system factory was able to handle scheme ",
                      ex.scheme() );
            continue;
        }
        auto fsFactory = m_ml->fsFactoryForMrl( mrl );
        if ( fsFactory == nullptr)
            continue;

        m_cb->onReloadStarted( mrl );
        auto res = reloadFolder( f, interruptProbe, *fsFactory );
        m_cb->onReloadCompleted( mrl, res );
    }
    return true;
}

bool FsDiscoverer::reload( const std::string& entryPoint,
                           const IInterruptProbe& interruptProbe )
{
    auto fsFactory = m_ml->fsFactoryForMrl( entryPoint );
    if ( fsFactory == nullptr )
        return false;

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
    reloadFolder( std::move( folder ), interruptProbe, *fsFactory );
    return true;
}

void FsDiscoverer::checkFolder( std::shared_ptr<fs::IDirectory> currentFolderFs,
                                std::shared_ptr<Folder> currentFolder,
                                const IInterruptProbe& interruptProbe,
                                fs::IFileSystemFactory& fsFactory,
                                bool newFolder ) const
{
    try
    {
        LOG_DEBUG( "Checking for modifications in ", currentFolderFs->mrl() );
        // We already know of this folder, though it may now contain a .nomedia file.
        // In this case, simply delete the folder.
        if ( m_probe->isHidden( *currentFolderFs ) == true )
        {
            if ( newFolder == false )
            {
                LOG_INFO( "A .nomedia file was added into a known folder, "
                          "removing it." );
                m_ml->deleteFolder( *currentFolder );
            }
            return;
        }

        if ( m_cb != nullptr )
            m_cb->onDiscoveryProgress( currentFolderFs->mrl() );
        // Load the folders we already know of:
        // Don't try to fetch any potential sub folders if the folder was freshly added
        std::vector<std::shared_ptr<Folder>> subFoldersInDB;
        if ( newFolder == false )
            subFoldersInDB = currentFolder->folders();
        for ( const auto& subFolder : currentFolderFs->dirs() )
        {
            if ( interruptProbe.isInterrupted() == true )
                break;
            if ( m_probe->stopFileDiscovery() == true )
                break;
            if ( m_probe->proceedOnDirectory( *subFolder ) == false )
                continue;
            auto it = std::find_if( begin( subFoldersInDB ), end( subFoldersInDB ),
                                    [&subFolder](const std::shared_ptr<Folder>& f) {
                auto subFolderName = utils::file::directoryName( subFolder->mrl() );
                return f->name() == utils::url::decode( subFolderName );
            });
            // We don't know this folder, it's a new one
            if ( it == end( subFoldersInDB ) )
            {
                LOG_DEBUG( "New folder detected: ", subFolder->mrl() );
                try
                {
                    if ( m_probe->isHidden( *subFolder ) )
                        continue;
                    addFolder( subFolder, currentFolder.get(), interruptProbe,
                               fsFactory );
                    continue;
                }
                catch ( const sqlite::errors::ConstraintForeignKey& ex )
                {
                    // A foreign key constraint violation indicates that the
                    // parent folders have been deleted due to being banned
                    LOG_WARN( "Creation of a folder failed because the parent is"
                              " non existing: ", ex.what(),
                              ". Assuming it was deleted due to being banned" );
                    return;
                }
                catch ( sqlite::errors::ConstraintViolation& ex )
                {
                    LOG_WARN( "Creation of a folder failed: ", ex.what(), ". Assuming it was banned" );
                    continue;
                }
            }
            auto folderInDb = *it;
            // In any case, check for modifications, as a change related to a mountpoint might
            // not update the folder modification date.
            // Also, relying on the modification date probably isn't portable
            checkFolder( subFolder, folderInDb, interruptProbe, fsFactory, false );
            subFoldersInDB.erase( it );
        }
        if ( m_probe->deleteUnseenFolders() == true )
        {
            // Now all folders we had in DB but haven't seen from the FS must have been deleted.
            for ( const auto& f : subFoldersInDB )
            {
                LOG_DEBUG( "Folder ", f->mrl(), " not found in FS, deleting it" );
                m_ml->deleteFolder( *f );
            }
        }
        checkFiles( currentFolderFs, currentFolder, interruptProbe );
    }
    // Only check once for a fs system error. They are bound to happen when we list the files/folders
    // within, and IProbe::isHidden is the first place when this is done
    catch ( const fs::errors::System& ex )
    {
        LOG_WARN( "Failed to browse ", currentFolderFs->mrl(), ": ", ex.what() );
        checkRemovedDevices( *currentFolderFs, *currentFolder, fsFactory, newFolder );
        // If the device has indeed been removed, fs::errors::DeviceRemoved will
        // be thrown, otherwise, we just failed to browse that folder and will
        // have to try again later.
        return;
    }
    LOG_DEBUG( "Done checking subfolders in ", currentFolderFs->mrl() );
}

void FsDiscoverer::checkFiles( std::shared_ptr<fs::IDirectory> parentFolderFs,
                               std::shared_ptr<Folder> parentFolder,
                               const IInterruptProbe& interruptProbe) const
{
    LOG_DEBUG( "Checking file in ", parentFolderFs->mrl() );

    struct FilesToAdd
    {
        FilesToAdd( std::shared_ptr<fs::IFile> f, IFile::Type t )
            : file( std::move( f ) ), type( t )
        {
        }
        std::shared_ptr<fs::IFile> file;
        IFile::Type type;
    };

    auto files = File::fromParentFolder( m_ml, parentFolder->id() );
    std::vector<FilesToAdd> filesToAdd;
    std::vector<FilesToAdd> linkedFilesToAdd;
    std::vector<std::pair<std::shared_ptr<File>, std::shared_ptr<fs::IFile>>> filesToRefresh;
    for ( const auto& fileFs: parentFolderFs->files() )
    {
        if ( interruptProbe.isInterrupted() == true )
            return;
        if ( m_probe->stopFileDiscovery() == true )
            break;
        if ( m_probe->proceedOnFile( *fileFs ) == false )
            continue;
        auto it = std::find_if( begin( files ), end( files ),
                                [fileFs](const std::shared_ptr<File>& f) {
            auto fileName = utils::file::fileName( f->mrl() );
            return fileName == fileFs->name();
        });
        if ( it == end( files ) || m_probe->forceFileRefresh() == true )
        {
            if ( fileFs->linkedType() == fs::IFile::LinkedFileType::None )
            {
                const auto ext = fileFs->extension();
                IFile::Type fileType = IFile::Type::Unknown;
                if ( m_ml->isMediaExtensionSupported( ext.c_str() ) == true )
                     fileType = IFile::Type::Main;
                else if ( m_ml->isPlaylistExtensionSupported( ext.c_str() ) == true )
                    fileType = IFile::Type::Playlist;
                if ( fileType != IFile::Type::Unknown )
                    filesToAdd.emplace_back( std::move( fileFs ), fileType );
            }
            else
            {
                auto fileType = IFile::Type::Unknown;
                switch ( fileFs->linkedType() )
                {
                    case fs::IFile::LinkedFileType::None:
                        assert( !"The linked file type can't be none" );
                        break;
                    case fs::IFile::LinkedFileType::Subtitles:
                        fileType = IFile::Type::Subtitles;
                        break;
                    case fs::IFile::LinkedFileType::SoundTrack:
                        fileType = IFile::Type::Soundtrack;
                        break;
                }
                if ( fileType != IFile::Type::Unknown )
                    linkedFilesToAdd.emplace_back( std::move( fileFs ), fileType );
            }
            continue;
        }
        if ( fileFs->lastModificationDate() != (*it)->lastModificationDate() )
        {
            LOG_DEBUG( "Forcing file refresh ", fileFs->mrl() );
            filesToRefresh.emplace_back( std::move( *it ), fileFs );
        }
        /* File sizes were stored as uint32_t, this is a best attempt to detect
         * those which were truncated
         */
        else if ( fileFs->size() != (*it)->size() && fileFs->size() > 0xFFFFFFFF )
        {
            (*it)->updateFsInfo( fileFs->lastModificationDate(), fileFs->size() );
        }
        files.erase( it );
    }
    if ( m_probe->deleteUnseenFiles() == false )
        files.clear();

    for ( const auto& file : files )
    {
        if ( interruptProbe.isInterrupted() == true )
            break;
        LOG_DEBUG( "File ", file->mrl(), " not found on filesystem, deleting it" );
        if ( file->type() == IFile::Type::Playlist )
        {
            file->destroy();
        }
        else
        {
            auto media = file->media();
            if ( media != nullptr )
                media->removeFile( *file );
            else
            {
                // This is unexpected, as the file should have been deleted when the media was
                // removed.
                LOG_WARN( "Deleting a file without an associated media." );
                file->destroy();
            }
        }
    }
    for ( auto& p: filesToRefresh )
    {
        if ( interruptProbe.isInterrupted() == true )
            break;
        m_ml->onUpdatedFile( std::move( p.first ), std::move( p.second ),
                             parentFolder, parentFolderFs );
    }
    for ( auto& p : filesToAdd )
    {
        if ( interruptProbe.isInterrupted() == true )
            break;
        m_ml->onDiscoveredFile( p.file, parentFolder, parentFolderFs,
                                p.type, m_probe->getPlaylistParent() );
    }
    for ( auto p : linkedFilesToAdd )
    {
        if ( interruptProbe.isInterrupted() == true )
            break;
        m_ml->onDiscoveredLinkedFile( std::move( p.file ), p.type );
    }
    LOG_DEBUG( "Done checking files in ", parentFolderFs->mrl() );
}

bool FsDiscoverer::addFolder( std::shared_ptr<fs::IDirectory> folder,
                              Folder* parentFolder,
                              const IInterruptProbe& interruptProbe,
                              fs::IFileSystemFactory& fsFactory ) const
{
    auto deviceFs = folder->device();
    // We are creating a folder, there has to be a device containing it.
    assert( deviceFs != nullptr );
    // But gracefully handle failure in release mode
    if( deviceFs == nullptr )
        return false;
    auto device = Device::fromUuid( m_ml, deviceFs->uuid(), fsFactory.scheme() );
    if ( device == nullptr )
    {
        LOG_INFO( "Creating new device in DB ", deviceFs->uuid() );
        device = Device::create( m_ml, deviceFs->uuid(),
                                 utils::url::scheme( folder->mrl() ),
                                 deviceFs->isRemovable(), deviceFs->isNetwork() );
        if ( device == nullptr )
            return false;
        if ( deviceFs->isNetwork() == true )
        {
            auto mountpoints = deviceFs->mountpoints();
            auto seenDate = time( nullptr );
            for ( const auto& m : mountpoints )
                device->addMountpoint( m, seenDate );
        }
    }

    auto f = Folder::create( m_ml, folder->mrl(),
                             parentFolder != nullptr ? parentFolder->id() : 0,
                             *device, *deviceFs );
    if ( f == nullptr )
        return false;
    checkFolder( std::move( folder ), std::move( f ), interruptProbe, fsFactory, true );
    return true;
}

}
