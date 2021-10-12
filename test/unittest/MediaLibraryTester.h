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

#pragma once

#include "MediaLibrary.h"
#include "Folder.h"
#include "medialibrary/filesystem/IDirectory.h"

using namespace medialibrary;

namespace medialibrary
{
    class Playlist;
    class AlbumTrack;
}

class MediaLibraryTester : public MediaLibrary
{
public:
    MediaLibraryTester( const std::string& dbPath,
                        const std::string& mlFolderPath );
    virtual void startParser() override { return; }
    virtual void startDeletionNotifier() override {}
    virtual void onDbConnectionReady( sqlite::Connection* dbConn ) override;
    std::vector<MediaPtr> files();
    // Use the filename getter
    using MediaLibrary::media;
    // And override the ID getter to return a Media instead of IMedia
    std::shared_ptr<Media> media( int64_t id );
    FolderPtr folder( const std::string& path ) const override;
    virtual FolderPtr folder( int64_t id ) const override;
    void deleteAlbum( int64_t albumId );
    std::shared_ptr<Genre> createGenre( const std::string& name );
    void deleteGenre( int64_t genreId );
    void deleteArtist( int64_t artistId );
    void deleteShow( int64_t showId );
    std::shared_ptr<Device> addDevice( const std::string& uuid,
                                       const std::string& scheme, bool isRemovable );
    void setFsFactory( std::shared_ptr<fs::IFileSystemFactory> fsFactory );
    std::shared_ptr<medialibrary::Media> albumTrack( int64_t id );
    // Use to run tests that fiddles with file properties (modification dates, size...)
    std::shared_ptr<Media> addFile(std::shared_ptr<fs::IFile> file, IMedia::Type type);
    // Used when we need an actual file instead of an external media
    std::shared_ptr<Media> addFile(const std::string& path , IMedia::Type type);

    virtual void addLocalFsFactory() override;
    std::shared_ptr<Device> device( const std::string& uuid,
                                    const std::string& scheme);
    virtual void onDiscoveredFile(std::shared_ptr<fs::IFile> fileFs,
                                   std::shared_ptr<Folder> parentFolder,
                                   std::shared_ptr<fs::IDirectory> parentFolderFs,
                                   IFile::Type fileType) override;
    virtual void populateNetworkFsFactories() override;
    MediaPtr addMedia( const std::string& mrl, IMedia::Type type );
    void deleteMedia( int64_t mediaId );
    bool outdateAllDevices();
    bool setMediaInsertionDate( int64_t mediaId, time_t t );
    bool setMediaLastPlayedDate( int64_t mediaId, time_t lastPlayedDate );
    bool outdateAllExternalMedia();
    bool setMediaType( int64_t mediaId, IMedia::Type type );
    uint32_t countNbThumbnails();
    uint32_t countNbTasks();
    virtual bool setupDummyFolder();
    void deleteAllTables( sqlite::Connection* dbConn );

private:
    std::shared_ptr<Media> addFile( std::shared_ptr<fs::IFile> fileFs,
                                    std::shared_ptr<Folder> parentFolder,
                                    std::shared_ptr<fs::IDirectory> parentFolderFs, IFile::Type fileType,
                                    IMedia::Type type );

private:
    std::shared_ptr<fs::IDevice> dummyDevice;
    std::shared_ptr<fs::IDirectory> dummyDirectory;
    std::shared_ptr<fs::IFileSystemFactory> fsFactory;
    std::shared_ptr<Folder> dummyFolder;
};

class MediaLibraryWithDiscoverer : public MediaLibraryTester
{
    using MediaLibraryTester::MediaLibraryTester;

    virtual bool setupDummyFolder() override
    {
        return true;
    }

    virtual void startDiscoverer() override
    {
        // Fall back to the default variant which actually starts the discoverer
        MediaLibrary::startDiscoverer();
    }
};

class MediaLibraryWithNotifier : public MediaLibraryTester
{
    using MediaLibraryTester::MediaLibraryTester;
    virtual void startDeletionNotifier() override
    {
        // Fall back to the default variant which actually starts the notifier
        MediaLibrary::startDeletionNotifier();
    }
};
