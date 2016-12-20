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

#include "MediaLibraryTester.h"

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "Device.h"
#include "Playlist.h"
#include "Genre.h"
#include "Media.h"
#include "Folder.h"
#include "mocks/FileSystem.h"


MediaLibraryTester::MediaLibraryTester()
    : dummyDirectory( new mock::NoopDirectory )
    , dummyFolder( nullptr, "./", 0, 0, false )
{
}

std::shared_ptr<Media> MediaLibraryTester::media( int64_t id )
{
    return std::static_pointer_cast<Media>( MediaLibrary::media( id ) );
}

std::shared_ptr<Folder> MediaLibraryTester::folder( const std::string& path )
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name +
            " WHERE is_blacklisted = 0 AND is_present = 1";
    auto folders = Folder::DatabaseHelpers::fetchAll<Folder>( this, req );
    for ( auto &f : folders )
    {
        if ( f->path() == path )
            return f;
    }
    return nullptr;
}

std::shared_ptr<Media> MediaLibraryTester::addFile( const std::string& path )
{
    mock::NoopFile file( path );
    return MediaLibrary::addFile( file, dummyFolder, *dummyDirectory );
}

std::shared_ptr<Media> MediaLibraryTester::addFile( fs::IFile& file )
{
    return MediaLibrary::addFile( file, dummyFolder, *dummyDirectory );
}

void MediaLibraryTester::addLocalFsFactory()
{
    if ( fsFactory != nullptr )
    {
        m_fsFactories.clear();
        m_fsFactories.push_back( fsFactory );
    }
    else
    {
        MediaLibrary::addLocalFsFactory();
    }
}

std::shared_ptr<Playlist> MediaLibraryTester::playlist( int64_t playlistId )
{
    return Playlist::fetch( this, playlistId );
}

void MediaLibraryTester::deleteAlbum( int64_t albumId )
{
    Album::destroy( this, albumId );
}

std::shared_ptr<Album> MediaLibraryTester::createAlbum( const std::string& title )
{
    return MediaLibrary::createAlbum( title, "dummy artwork" );
}

std::shared_ptr<Genre> MediaLibraryTester::createGenre( const std::string& name )
{
    return Genre::create( this, name );
}

void MediaLibraryTester::deleteGenre( int64_t genreId )
{
    Genre::destroy( this, genreId );
}

void MediaLibraryTester::deleteArtist( int64_t artistId )
{
    Artist::destroy( this, artistId );
}

std::shared_ptr<Device> MediaLibraryTester::addDevice( const std::string& uuid, bool isRemovable )
{
    return Device::create( this, uuid, isRemovable );
}

void MediaLibraryTester::setFsFactory( std::shared_ptr<factory::IFileSystem> fsf )
{
    fsFactory = fsf;
}

void MediaLibraryTester::deleteTrack( int64_t trackId )
{
    AlbumTrack::destroy( this, trackId );
}

std::shared_ptr<AlbumTrack> MediaLibraryTester::albumTrack( int64_t id )
{
    return AlbumTrack::fetch( this, id );
}

std::vector<MediaPtr> MediaLibraryTester::files()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE is_present = 1";
    return Media::fetchAll<IMedia>( this, req );
}

std::shared_ptr<Device> MediaLibraryTester::device( const std::string& uuid )
{
    return Device::fromUuid( this, uuid );
}
