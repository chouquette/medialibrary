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

class Playlist;
class AlbumTrack;

class MediaLibraryTester : public MediaLibrary
{
public:
    MediaLibraryTester();
    std::vector<MediaPtr> files();
    std::shared_ptr<Media> media( int64_t id );
    MediaPtr media( const std::string& path );
    std::shared_ptr<Folder> folder( const std::string& path );
    std::shared_ptr<Media> addFile( const std::string& path );
    std::shared_ptr<Playlist> playlist( int64_t playlistId );
    void deleteAlbum( int64_t albumId );
    std::shared_ptr<Genre> createGenre( const std::string& name );
    void deleteGenre( int64_t genreId );
    void deleteArtist( int64_t artistId );
    std::shared_ptr<Device> addDevice( const std::string& uuid, bool isRemovable );
    void setFsFactory( std::shared_ptr<factory::IFileSystem> fsFactory );
    void deleteTrack( int64_t trackId );
    std::shared_ptr<AlbumTrack> albumTrack( int64_t id );
    std::shared_ptr<Media> addFile(fs::IFile& file);

private:
    std::unique_ptr<fs::IDirectory> dummyDirectory;
    Folder dummyFolder;
};

class MediaLibraryWithoutParser : public MediaLibraryTester
{
    virtual void startParser() override {}
};

class MediaLibraryWithoutBackground : public MediaLibraryTester
{
    virtual void startDiscoverer() override {}
    virtual void startParser() override {}
};

class MediaLibraryWithNotifier : public MediaLibraryTester
{
    virtual void startDiscoverer() override {}
    virtual void startParser() override {}
};
