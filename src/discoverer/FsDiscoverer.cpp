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
#include <cstring>

#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/Errors.h"
#include "Media.h"
#include "File.h"
#include "Device.h"
#include "Folder.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "medialibrary/IInterruptProbe.h"

namespace medialibrary
{

FsDiscoverer::FsDiscoverer( MediaLibrary* ml, IMediaLibraryCb* cb )
    : m_ml( ml )
    , m_cb( cb )
{
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
            Folder::remove( m_ml, std::move( f ), Folder::RemovalBehavior::RemovedFromDisk );
            return false;
        }
    }
    try
    {
        checkFolder( std::move( directory ), std::move( f ), probe, fsFactory );
    }
    catch ( fs::errors::DeviceRemoved& )
    {
        LOG_INFO( "Reloading of ", mrl, " was stopped after the device was removed" );
        return false;
    }
    return true;
}

void FsDiscoverer::checkRemovedDevices( fs::IDirectory& fsFolder,
                                        std::shared_ptr<Folder> folder,
                                        fs::IFileSystemFactory& fsFactory,
                                        bool newFolder, bool rootFolder ) const
{
    assert( folder != nullptr );
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

        // When a network device gets detected, it may be unaccessible for a while
        // which may cause the discovery to fail, but we don't want to remove the
        // folder in this case. However if we are browsing a subfolder, it means
        // the device is readable, and a folder not being found means it has
        // been deleted, in which case we're fine with deleting the folder
        // from the database
        if ( rootFolder == true )
        {
            LOG_DEBUG( "The device is present but can be read from. "
                       "Assuming it isn't ready yet" );
            return;
        }
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
        Folder::remove( m_ml, std::move( folder ),
                        Folder::RemovalBehavior::RemovedFromDisk );
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
        /*
         * We only fetch present folders, but during the reload (or even between
         * the fetch from database & the time we actually use the results)
         * another thread may have set the containing device to missing
         */
        if ( f->isPresent() == false )
            continue;
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

        m_cb->onDiscoveryStarted( mrl );
        auto res = reloadFolder( f, interruptProbe, *fsFactory );
        m_cb->onDiscoveryCompleted( mrl, res );
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

bool FsDiscoverer::addEntryPoint( const std::string& entryPoint )
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
        LOG_ERROR( "Can't create IDirectory to represent ", entryPoint,
                   ": ", ex.what() );
        return false;
    }
     // Use the canonical path computed by IDirectory
    try
    {
        return addFolder( std::move( fsDir ), nullptr ) != nullptr;
    }
    catch ( const sqlite::errors::ConstraintViolation& ex )
    {
        LOG_WARN( "Can't add entrypoint ", entryPoint, ": ", ex.what() );
        return false;
    }
}

void FsDiscoverer::checkFolder( std::shared_ptr<fs::IDirectory> folderFs,
                                std::shared_ptr<Folder> folder,
                                const IInterruptProbe& interruptProbe,
                                fs::IFileSystemFactory& fsFactory ) const
{
    struct CurrentFolder
    {
        std::shared_ptr<fs::IDirectory> fs;
        std::shared_ptr<Folder> entity;
        std::shared_ptr<Folder> parent;
    };
    auto rootFolder = true;

    std::stack<CurrentFolder> directories;
    directories.push( { std::move( folderFs ), std::move( folder ), nullptr } );
    while ( directories.empty() == false )
    {
        auto& dirToCheck = directories.top();
        auto currentDirFs = std::move( dirToCheck.fs );
        auto currentDir = std::move( dirToCheck.entity );
        auto parentDir = std::move( dirToCheck.parent );
        auto newFolder = currentDir == nullptr;
        directories.pop();
        LOG_DEBUG( "Checking for modifications in ", currentDirFs->mrl() );
        bool hasNoMedia;
        try
        {
            hasNoMedia = currentDirFs->contains( ".nomedia" );
        }
        catch ( const fs::errors::System& ex )
        {
            LOG_WARN( "Failed to browse ", currentDirFs->mrl(), ": ", ex.what() );
            /*
             * If we were supposed to add this folder but can't read from it
             * just ignore it
             */
            if ( currentDir == nullptr )
                continue;
            checkRemovedDevices( *currentDirFs, std::move( currentDir ),
                                 fsFactory, newFolder, rootFolder );
            // If the device has indeed been removed, fs::errors::DeviceRemoved will
            // be thrown, otherwise, we just failed to browse that folder and will
            // have to try again later.
            continue;
        }
        /*
         * We managed to read from the directory we're refreshing, so we can
         * flag the device as readable for future potential checkRemovedDevice
         * calls
         */
        rootFolder = false;

        if ( hasNoMedia == true )
        {
            if ( newFolder == false )
            {
                LOG_INFO( "A .nomedia file was added into a known folder, "
                          "removing it." );
                Folder::remove( m_ml, std::move( currentDir ),
                                 Folder::RemovalBehavior::RemovedFromDisk );
            }
            continue;
        }

        if ( currentDir == nullptr )
        {
            try
            {
                currentDir = addFolder( currentDirFs, parentDir.get() );
                if ( currentDir == nullptr )
                    continue;
                LOG_DEBUG( "New folder detected: ", currentDirFs->mrl() );
            }
            catch ( const sqlite::errors::ConstraintForeignKey& ex )
            {
                // A foreign key constraint violation indicates that the
                // parent folders have been deleted due to being banned
                LOG_WARN( "Creation of a folder failed because the parent is"
                          " non existing: ", ex.what(),
                          ". Assuming it was deleted due to being banned" );
                continue;
            }
            catch ( sqlite::errors::ConstraintViolation& ex )
            {
                LOG_WARN( "Creation of a folder failed: ", ex.what(), ". Assuming it was banned" );
                continue;
            }
        }
        if ( m_cb != nullptr )
            m_cb->onDiscoveryProgress( currentDirFs->mrl() );
        auto subFoldersInDB = currentDir->folders();
        for ( const auto& subFolder : currentDirFs->dirs() )
        {
            if ( interruptProbe.isInterrupted() == true )
                break;
            auto it = std::find_if( begin( subFoldersInDB ), end( subFoldersInDB ),
                                    [&subFolder](const std::shared_ptr<Folder>& f) {
                auto subFolderName = utils::file::directoryName( subFolder->mrl() );
                return f->name() == utils::url::decode( subFolderName );
            });
            // We don't know this folder, it's a new one
            if ( it == end( subFoldersInDB ) )
            {
                directories.push( { subFolder, nullptr, currentDir } );
                continue;
            }
            auto folderInDb = *it;
            // In any case, check for modifications, as a change related to a mountpoint might
            // not update the folder modification date.
            // Also, relying on the modification date probably isn't portable
            directories.push( { subFolder, std::move( folderInDb ),
                                currentDir } );
            subFoldersInDB.erase( it );
        }
        // Now all folders we had in DB but haven't seen from the FS must have been deleted.
        for ( const auto& f : subFoldersInDB )
        {
            LOG_DEBUG( "Folder ", f->mrl(), " not found in FS, deleting it" );
            Folder::remove( m_ml, f, Folder::RemovalBehavior::RemovedFromDisk );
        }
        checkFiles( currentDirFs, currentDir, interruptProbe );
        LOG_DEBUG( "Done checking subfolders in ", currentDir->mrl() );
    }
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
        auto it = std::find_if( begin( files ), end( files ),
                                [fileFs](const std::shared_ptr<File>& f) {
            auto fileName = utils::file::fileName( f->mrl() );
            return fileName == fileFs->name();
        });
        if ( it == end( files ) )
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
                                p.type );
    }
    for ( auto p : linkedFilesToAdd )
    {
        if ( interruptProbe.isInterrupted() == true )
            break;
        m_ml->onDiscoveredLinkedFile( std::move( p.file ), p.type );
    }
    LOG_DEBUG( "Done checking files in ", parentFolderFs->mrl() );
}

std::shared_ptr<Folder>
FsDiscoverer::addFolder( std::shared_ptr<fs::IDirectory> folder,
                         Folder* parentFolder ) const
{
    auto deviceFs = folder->device();
    // We are creating a folder, there has to be a device containing it.
    assert( deviceFs != nullptr );
    // But gracefully handle failure in release mode
    if( deviceFs == nullptr )
        return nullptr;
    auto t = m_ml->getConn()->newTransaction();
    auto device = Device::fromUuid( m_ml, deviceFs->uuid(), deviceFs->scheme() );
    if ( device == nullptr )
    {
        LOG_INFO( "Creating new device in DB ", deviceFs->uuid() );
        device = Device::create( m_ml, deviceFs->uuid(),
                                 utils::url::scheme( folder->mrl() ),
                                 deviceFs->isRemovable(), deviceFs->isNetwork() );
        if ( device == nullptr )
            return nullptr;
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
        return nullptr;
    t->commit();
    return f;
}

}
