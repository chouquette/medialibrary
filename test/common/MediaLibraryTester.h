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

#pragma once

#include "MediaLibrary.h"
#include "Folder.h"
#include "filesystem/IDirectory.h"

using namespace medialibrary;

namespace medialibrary
{
    class Playlist;
    class AlbumTrack;
}

class MediaLibraryTester : public MediaLibrary
{
public:
    MediaLibraryTester();
    virtual void startParser() override {}
    virtual void startDiscoverer() override {}
    virtual void startDeletionNotifier() override {}
    std::vector<MediaPtr> files();
    // Use the filename getter
    using MediaLibrary::media;
    // And override the ID getter to return a Media instead of IMedia
    std::shared_ptr<Media> media( int64_t id );
    std::shared_ptr<Folder> folder( const std::string& path );
    std::shared_ptr<Playlist> playlist( int64_t playlistId );
    void deleteAlbum( int64_t albumId );
    std::shared_ptr<Album> createAlbum( const std::string& title );
    std::shared_ptr<Genre> createGenre( const std::string& name );
    void deleteGenre( int64_t genreId );
    void deleteArtist( int64_t artistId );
    std::shared_ptr<Device> addDevice( const std::string& uuid, bool isRemovable );
    void setFsFactory( std::shared_ptr<factory::IFileSystem> fsFactory );
    void deleteTrack( int64_t trackId );
    std::shared_ptr<AlbumTrack> albumTrack( int64_t id );
    // Use to run tests that fiddles with file properties (modification dates, size...)
    std::shared_ptr<Media> addFile( std::shared_ptr<fs::IFile> file);
    // Used when we need an actual file instead of an external media
    std::shared_ptr<Media> addFile( const std::string& path );
    std::shared_ptr<Media> addFile( std::shared_ptr<fs::IFile> fileFs,
                                    std::shared_ptr<Folder> parentFolder,
                                    std::shared_ptr<fs::IDirectory> parentFolderFs );
    virtual void addLocalFsFactory() override;
    std::shared_ptr<Device> device( const std::string& uuid );
    std::vector<const char*> getSupportedExtensions() const;
    virtual void addDiscoveredFile( std::shared_ptr<fs::IFile> fileFs,
                                    std::shared_ptr<Folder> parentFolder,
                                    std::shared_ptr<fs::IDirectory> parentFolderFs,
                                    std::pair<std::shared_ptr<Playlist>, unsigned int> parentPlaylist ) override;
    sqlite::Connection* getDbConn();

private:
    std::shared_ptr<fs::IDirectory> dummyDirectory;
    std::shared_ptr<factory::IFileSystem> fsFactory;
    std::shared_ptr<Folder> dummyFolder;
};

class MediaLibraryWithDiscoverer : public MediaLibraryTester
{
    virtual void startDiscoverer() override
    {
        // Fall back to the default variant which actually starts the discoverer
        MediaLibrary::startDiscoverer();
    }
};

class MediaLibraryWithNotifier : public MediaLibraryTester
{
    virtual void startDeletionNotifier() override
    {
        // Fall back to the default variant which actually starts the notifier
        MediaLibrary::startDeletionNotifier();
    }
};
