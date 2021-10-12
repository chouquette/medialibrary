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

#include "UnitTests.h"

#include "Album.h"
#include "Device.h"
#include "File.h"
#include "Media.h"
#include "Artist.h"
#include "Show.h"
#include "MediaGroup.h"
#include "Genre.h"
#include "Playlist.h"

#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"

struct DeviceFsTests : public UnitTests<mock::WaitForDiscoveryComplete>
{
    static const std::string RemovableDeviceUuid;
    static const std::string RemovableDeviceMountpoint;
    static constexpr auto NbRemovableMedia = 6u;

    virtual void SetupMockFileSystem() override
    {
        fsMock.reset( new mock::FileSystemFactory );
        fsMock->addFolder( "file:///a/mnt/" );
        fsMock->addDevice( DeviceFsTests::RemovableDeviceMountpoint, DeviceFsTests::RemovableDeviceUuid, true );
        fsMock->addFile( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
        fsMock->addFile( DeviceFsTests::RemovableDeviceMountpoint + "removablefile2.mp3" );
        fsMock->addFile( DeviceFsTests::RemovableDeviceMountpoint + "removablefile3.mp3" );
        fsMock->addFile( DeviceFsTests::RemovableDeviceMountpoint + "removablefile4.mp3" );
        fsMock->addFile( DeviceFsTests::RemovableDeviceMountpoint + "removablevideo.mkv" );
        fsMock->addFile( DeviceFsTests::RemovableDeviceMountpoint + "removablevideo2.mkv" );
    }

    virtual void InstantiateMediaLibrary( const std::string& dbPath,
                                          const std::string& mlFolderDir ) override
    {
        ml.reset( new MediaLibraryWithDiscoverer( dbPath, mlFolderDir ) );
    }

    bool Reload()
    {
        ml->reload();
        auto res = cbMock->waitReload();
        ASSERT_TRUE( res );
        return true;
    }

    void enforceFakeMediaTypes()
    {
        auto media = std::static_pointer_cast<Media>( ml->media(
                                    mock::FileSystemFactory::Root + "video.avi" ) );
        media->setType( IMedia::Type::Video );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        mock::FileSystemFactory::Root + "audio.mp3" ) );
        media->setType( IMedia::Type::Audio );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        mock::FileSystemFactory::SubFolder + "subfile.mp4" ) );
        media->setType( IMedia::Type::Video );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" ) );
        media->setType( IMedia::Type::Audio );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        DeviceFsTests::RemovableDeviceMountpoint + "removablefile2.mp3" ) );
        media->setType( IMedia::Type::Audio );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        DeviceFsTests::RemovableDeviceMountpoint + "removablefile3.mp3" ) );
        media->setType( IMedia::Type::Audio );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        DeviceFsTests::RemovableDeviceMountpoint + "removablefile4.mp3" ) );
        media->setType( IMedia::Type::Audio );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        DeviceFsTests::RemovableDeviceMountpoint + "removablevideo.mkv" ) );
        media->setType( IMedia::Type::Video );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        DeviceFsTests::RemovableDeviceMountpoint + "removablevideo2.mkv" ) );
        media->setType( IMedia::Type::Video );
        media->save();
    }
};

const std::string DeviceFsTests::RemovableDeviceUuid = "{fake-removable-device}";
const std::string DeviceFsTests::RemovableDeviceMountpoint = "file:///a/mnt/fake-device/";

static void RemoveDisk( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = T->ml->files();
    ASSERT_EQ( 3u + DeviceFsTests::NbRemovableMedia, files.size() );

    auto media = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );

    T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );

    T->Reload();

    files = T->ml->files();
    ASSERT_EQ( 3u, files.size() );

    media = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, media );
}

static void UnmountDisk( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
    T->enforceFakeMediaTypes();

    auto files = T->ml->files();
    ASSERT_EQ( 3u + DeviceFsTests::NbRemovableMedia, files.size() );
    auto allVideos = T->ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 4u, allVideos.size() );

    auto media = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );
    auto mediaId = media->id();

    T->fsMock->unmountDevice( DeviceFsTests::RemovableDeviceUuid );

    T->Reload();

    files = T->ml->files();
    ASSERT_EQ( 3u, files.size() );

    media = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, media );

    media = T->ml->media( mediaId );
    ASSERT_NE( nullptr, media );
    ASSERT_FALSE( media->isPresent() );

    allVideos = T->ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 2u, allVideos.size() );
    QueryParameters params{};
    params.includeMissing = true;
    allVideos = T->ml->videoFiles( &params )->all();
    ASSERT_EQ( 4u, allVideos.size() );

    T->fsMock->remountDevice( DeviceFsTests::RemovableDeviceUuid );

    T->Reload();

    files = T->ml->files();
    ASSERT_EQ( 3u + DeviceFsTests::NbRemovableMedia, files.size() );

    media = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );

    media = T->ml->media( mediaId );
    ASSERT_NE( nullptr, media );
    ASSERT_TRUE( media->isPresent() );
}

static void ReplugDisk( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = T->ml->files();
    ASSERT_EQ( 3u + DeviceFsTests::NbRemovableMedia, files.size() );

    auto media = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );

    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );

    T->Reload();

    files = T->ml->files();
    ASSERT_EQ( 3u, files.size() );

    media = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, media );

    T->fsMock->addDevice( device );
    T->Reload();

    files = T->ml->files();
    ASSERT_EQ( 3u + DeviceFsTests::NbRemovableMedia, files.size() );

    media = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );
}

static void ReplugDiskWithExtraFiles( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = T->ml->files();
    ASSERT_EQ( 3u + DeviceFsTests::NbRemovableMedia, files.size() );

    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );

    T->Reload();

    files = T->ml->files();
    ASSERT_EQ( 3u, files.size() );

    T->fsMock->addDevice( device );
    T->fsMock->addFile( DeviceFsTests::RemovableDeviceMountpoint + "newfile.mkv" );

    T->Reload();

    files = T->ml->files();
    ASSERT_EQ( 3u + DeviceFsTests::NbRemovableMedia + 1, files.size() );
}

static void RemoveAlbumAndArtist( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    // Create an album on a non-removable device
    {
        auto album = std::static_pointer_cast<Album>( T->ml->createAlbum( "album" ) );
        auto media = std::static_pointer_cast<Media>( T->ml->media( mock::FileSystemFactory::Root + "audio.mp3" ) );
        auto artist = T->ml->createArtist( "artist" );
        album->addTrack( std::static_pointer_cast<Media>( media ), 1, 1, artist->id(), nullptr );
        album->setAlbumArtist( artist );
        artist->addMedia( *media );
    }
    // And an album that will disappear, along with its artist
    int64_t removableAlbumId;
    int64_t removableArtistId;
    {
        auto album = std::static_pointer_cast<Album>( T->ml->createAlbum( "album 2" ) );
        removableAlbumId = album->id();
        auto album2 = std::static_pointer_cast<Album>( T->ml->createAlbum( "album 3" ) );
        auto media1 = std::static_pointer_cast<Media>( T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" ) );
        auto media2 = std::static_pointer_cast<Media>( T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile2.mp3" ) );
        auto media3 = std::static_pointer_cast<Media>( T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile3.mp3" ) );
        auto media4 = std::static_pointer_cast<Media>( T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile4.mp3" ) );
        auto artist = T->ml->createArtist( "artist 2" );
        removableArtistId = artist->id();
        album->addTrack( std::static_pointer_cast<Media>( media1 ), 1, 1, artist->id(), nullptr );
        album->addTrack( std::static_pointer_cast<Media>( media2 ), 2, 1, artist->id(), nullptr );
        album2->addTrack( std::static_pointer_cast<Media>( media3 ), 1, 1, artist->id(), nullptr );
        album2->addTrack( std::static_pointer_cast<Media>( media4 ), 2, 1, artist->id(), nullptr );
        album->setAlbumArtist( artist );
        album2->setAlbumArtist( artist );
        artist->addMedia( *media1 );
        artist->addMedia( *media2 );
        artist->addMedia( *media3 );
        artist->addMedia( *media4 );
        media1->save();
        media2->save();
        media3->save();
        media4->save();
    }

    auto res = Artist::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );

    auto albums = T->ml->albums( nullptr )->all();
    ASSERT_EQ( 3u, albums.size() );
    auto artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );
    auto removableAlbum = T->ml->album( removableAlbumId );
    ASSERT_NE( nullptr, removableAlbum );
    ASSERT_NE( 0u, removableAlbum->nbPresentTracks() );
    auto removableArtist = T->ml->artist( removableArtistId );
    ASSERT_NE( nullptr, removableArtist );
    ASSERT_NE( 0u, removableArtist->nbPresentTracks() );


    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );

    T->Reload();

    albums = T->ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );

    QueryParameters params{};
    params.includeMissing = true;
    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );

    artists = T->ml->artists( ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );

    removableAlbum = T->ml->album( removableAlbumId );
    ASSERT_NE( nullptr, removableAlbum );
    ASSERT_EQ( 0u, removableAlbum->nbPresentTracks() );
    removableArtist = T->ml->artist( removableArtistId );
    ASSERT_NE( nullptr, removableArtist );
    ASSERT_EQ( 0u, removableArtist->nbPresentTracks() );

    // Now check that everything appears again when we plug the device back in

    T->fsMock->addDevice( device );

    T->Reload();

    albums = T->ml->albums( nullptr )->all();
    ASSERT_EQ( 3u, albums.size() );
    artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );

    res = Artist::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );
}

static void RemoveArtist( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    // Check that an artist with a track remaining but album is still present
    // Album artist disappearance is already tested by RemoveAlbumAndArtist test
    auto artist = T->ml->createArtist( "removable artist" );

    auto album = std::static_pointer_cast<Album>( T->ml->createAlbum( "removable album" ) );
    auto media1 = std::static_pointer_cast<Media>( T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" ) );
    auto media2 = std::static_pointer_cast<Media>( T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile2.mp3" ) );
    auto media3 = std::static_pointer_cast<Media>( T->ml->media( mock::FileSystemFactory::Root + "audio.mp3" ) );

    album->addTrack( std::static_pointer_cast<Media>( media1 ), 1, 1, artist->id(), nullptr );
    media1->save();
    album->addTrack( std::static_pointer_cast<Media>( media2 ), 2, 1, artist->id(), nullptr );
    media2->save();
    album->addTrack( std::static_pointer_cast<Media>( media3 ), 3, 1, artist->id(), nullptr );
    media3->save();
    artist->addMedia( *media1 );
    artist->addMedia( *media2 );
    artist->addMedia( *media3 );

    auto res = Artist::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );

    auto albums = T->ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    auto artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();

    ASSERT_EQ( 1u, artists.size() );

    QueryParameters params;
    params.sort = SortingCriteria::Alpha;
    params.desc = false;
    auto tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 3u, tracks.size() );

    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );

    T->Reload();

    // nothing should have changed as far as the artist count goes

    albums = T->ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );

    // But we expect the tracks count to be down
    artist = std::static_pointer_cast<Artist>( T->ml->artist( artist->id() ) );
    tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 1u, tracks.size() );

    // Now check that everything appears again when we plug the device back in

    T->fsMock->addDevice( device );

    T->Reload();

    albums = T->ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
    artist = std::static_pointer_cast<Artist>( T->ml->artist( artist->id() ) );
    tracks = artist->tracks( nullptr )->all();
    ASSERT_EQ( 3u, tracks.size() );

    res = Artist::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );
}

static void PartialAlbumRemoval( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    {
        auto album = T->ml->createAlbum( "album" );
        auto media = std::static_pointer_cast<Media>(
                    T->ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" ) );
        auto media2 = std::static_pointer_cast<Media>(
                    T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile2.mp3" ) );
        auto newArtist = T->ml->createArtist( "artist" );
        album->addTrack( media, 1, 1, newArtist->id(), nullptr );
        media->save();
        album->addTrack( media2, 2, 1, newArtist->id(), nullptr );
        media2->save();
        album->setAlbumArtist( newArtist );
        newArtist->addMedia( static_cast<Media&>( *media ) );
        newArtist->addMedia( static_cast<Media&>( *media2 ) );
    }

    auto res = Artist::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );

    auto albums = T->ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    auto artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
    auto artist = artists[0];
    ASSERT_EQ( 2u, artist->tracks( nullptr )->count() );

    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );
    T->Reload();

    albums = T->ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( 1u, albums[0]->tracks( nullptr )->count() );
    ASSERT_EQ( 1u, artists[0]->tracks( nullptr )->count() );

    T->fsMock->addDevice( device );

    T->Reload();

    res = Artist::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( T->ml.get() );
    ASSERT_TRUE( res );
}

static void ChangeDevice( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    // Fetch a removable media's ID
    auto f = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( f, nullptr );
    auto firstRemovableFileId = f->id();
    auto files = f->files();
    ASSERT_EQ( 1u, files.size() );
    auto firstRemovableFilePath = files[0]->mrl();

    // Remove & store the device
    auto oldRemovableDevice = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );

    // Add a new device on the same mountpoint
    T->fsMock->addDevice( DeviceFsTests::RemovableDeviceMountpoint, "{another-removable-device}", true );
    T->fsMock->addFile( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );

    T->Reload();

    // Check that new files with the same name have different IDs
    // but the same "full path"
    f = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, f );
    files = f->files();
    ASSERT_EQ( 1u, files.size() );
    ASSERT_EQ( firstRemovableFilePath, files[0]->mrl() );
    ASSERT_NE( firstRemovableFileId, f->id() );

    T->fsMock->removeDevice( "{another-removable-device}" );
    T->fsMock->addDevice( oldRemovableDevice );

    T->Reload();

    f = T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, f );
    ASSERT_EQ( firstRemovableFileId, f->id() );
}

static void UnknownMountpoint( DeviceFsTests* T )
{
    // The mock filesystem starts at /a/
    // Simply check that we don't crash
    T->ml->discover( "file:///" );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
}

static void OutdatedDevices( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u + DeviceFsTests::NbRemovableMedia, T->ml->files().size() );
    auto oldMediaCount = T->ml->files().size();

    T->ml->outdateAllDevices();
    T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );

    T->Reload();

    ASSERT_NE( oldMediaCount, T->ml->files().size() );
}

static void RemovableMountPointName( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto f = T->ml->folder( DeviceFsTests::RemovableDeviceMountpoint );
    ASSERT_NE( nullptr, f );
    ASSERT_NE( 0u, f->name().size() );
}

static void RemoveShowEpisodes( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto show1 = T->ml->createShow( "Show1" );
    auto media1 = std::static_pointer_cast<Media>(
                T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablevideo.mkv" ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    media1->save();
    auto media2 = std::static_pointer_cast<Media>(
                T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablevideo2.mkv" ) );
    show1->addEpisode( *media2, 1, 2, "episode title" );
    media2->save();

    auto showsQuery = T->ml->shows( nullptr );
    ASSERT_EQ( 1u, showsQuery->count() );
    ASSERT_EQ( 1u, showsQuery->all().size() );

    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );
    T->Reload();

    showsQuery = T->ml->shows( nullptr );
    ASSERT_EQ( 0u, showsQuery->count() );
    ASSERT_EQ( 0u, showsQuery->all().size() );

    T->fsMock->addDevice( device );
    T->Reload();

    showsQuery = T->ml->shows( nullptr );
    ASSERT_EQ( 1u, showsQuery->count() );
    ASSERT_EQ( 1u, showsQuery->all().size() );
}

static void PartialRemoveShowEpisodes( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto show1 = T->ml->createShow( "Show1" );
    auto media1 = std::static_pointer_cast<Media>(
                T->ml->media( mock::FileSystemFactory::Root + "video.avi" ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    media1->save();

    auto media2 = std::static_pointer_cast<Media>(
                T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablevideo.mkv" ) );
    show1->addEpisode( *media2, 1, 2, "episode title" );
    media2->save();

    auto shows = T->ml->shows( nullptr )->all();
    ASSERT_EQ( 1u, shows.size() );

    auto episodeQuery = shows[0]->episodes( nullptr );
    ASSERT_EQ( 2u, episodeQuery->count() );
    ASSERT_EQ( 2u, episodeQuery->all().size() );

    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );
    T->Reload();

    shows = T->ml->shows( nullptr )->all();
    ASSERT_EQ( 1u, shows.size() );

    episodeQuery = shows[0]->episodes( nullptr );
    ASSERT_EQ( 1u, episodeQuery->count() );
    ASSERT_EQ( 1u, episodeQuery->all().size() );

    T->fsMock->addDevice( device );
    T->Reload();

    shows = T->ml->shows( nullptr )->all();
    ASSERT_EQ( 1u, shows.size() );

    episodeQuery = shows[0]->episodes( nullptr );
    ASSERT_EQ( 2u, episodeQuery->count() );
    ASSERT_EQ( 2u, episodeQuery->all().size() );
}

static void MediaGroupPresence( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    T->enforceFakeMediaTypes();

    auto addToGroup = [T]( IMediaGroup& mg, const std::string& mrl ) {
        auto m = T->ml->media( mrl );
        auto res = m->setPlayCount( 1 );
        ASSERT_TRUE( res );
        ASSERT_NE( nullptr, m );
        res = mg.add( *m );
        ASSERT_TRUE( res );
    };

    auto rmg = T->ml->createMediaGroup( "removable group" );

    addToGroup( *rmg, DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    addToGroup( *rmg, DeviceFsTests::RemovableDeviceMountpoint + "removablefile2.mp3" );
    addToGroup( *rmg, DeviceFsTests::RemovableDeviceMountpoint + "removablefile3.mp3" );
    addToGroup( *rmg, DeviceFsTests::RemovableDeviceMountpoint + "removablefile4.mp3" );
    addToGroup( *rmg, DeviceFsTests::RemovableDeviceMountpoint + "removablevideo.mkv" );
    addToGroup( *rmg, DeviceFsTests::RemovableDeviceMountpoint + "removablevideo2.mkv" );

    ASSERT_EQ( 4u, rmg->nbPresentAudio() );
    ASSERT_EQ( 2u, rmg->nbPresentVideo() );
    ASSERT_EQ( 0u, rmg->nbPresentUnknown() );
    ASSERT_EQ( 6u, rmg->nbPresentMedia() );
    ASSERT_EQ( 6u, rmg->nbPresentSeen() );

    ASSERT_EQ( 4u, rmg->nbAudio() );
    ASSERT_EQ( 2u, rmg->nbVideo() );
    ASSERT_EQ( 0u, rmg->nbUnknown() );
    ASSERT_EQ( 6u, rmg->nbSeen() );
    ASSERT_EQ( 6u, rmg->nbTotalMedia() );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    rmg = T->ml->mediaGroup( rmg->id() );

    ASSERT_EQ( 4u, rmg->nbPresentAudio() );
    ASSERT_EQ( 2u, rmg->nbPresentVideo() );
    ASSERT_EQ( 0u, rmg->nbPresentUnknown() );
    ASSERT_EQ( 6u, rmg->nbPresentMedia() );
    ASSERT_EQ( 6u, rmg->nbTotalMedia() );
    ASSERT_EQ( 6u, rmg->nbPresentSeen() );

    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );
    T->Reload();

    rmg = T->ml->mediaGroup( rmg->id() );

    ASSERT_EQ( 0u, rmg->nbPresentAudio() );
    ASSERT_EQ( 0u, rmg->nbPresentVideo() );
    ASSERT_EQ( 0u, rmg->nbPresentUnknown() );
    ASSERT_EQ( 0u, rmg->nbPresentMedia() );
    ASSERT_EQ( 6u, rmg->nbTotalMedia() );
    ASSERT_EQ( 0u, rmg->nbPresentSeen() );

    ASSERT_EQ( 4u, rmg->nbAudio() );
    ASSERT_EQ( 2u, rmg->nbVideo() );
    ASSERT_EQ( 0u, rmg->nbUnknown() );
    ASSERT_EQ( 6u, rmg->nbSeen() );

    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );

    QueryParameters params{};
    params.includeMissing = true;
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 1u, groups.size() );
    auto media = groups[0]->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
    media = groups[0]->media( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 6u, media.size() );
    media = groups[0]->media( IMedia::Type::Video, &params )->all();
    ASSERT_EQ( 2u, media.size() );
    media = groups[0]->media( IMedia::Type::Audio, &params )->all();
    ASSERT_EQ( 4u, media.size() );

    T->fsMock->addDevice( device );
    T->Reload();

    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    rmg = T->ml->mediaGroup( rmg->id() );

    ASSERT_EQ( 4u, rmg->nbPresentAudio() );
    ASSERT_EQ( 2u, rmg->nbPresentVideo() );
    ASSERT_EQ( 0u, rmg->nbPresentUnknown() );
    ASSERT_EQ( 6u, rmg->nbPresentMedia() );
    ASSERT_EQ( 6u, rmg->nbPresentSeen() );
    ASSERT_EQ( 6u, rmg->nbTotalMedia() );
    ASSERT_EQ( 4u, rmg->nbAudio() );
    ASSERT_EQ( 2u, rmg->nbVideo() );
    ASSERT_EQ( 0u, rmg->nbUnknown() );
    ASSERT_EQ( 6u, rmg->nbTotalMedia() );
    ASSERT_EQ( 6u, rmg->nbSeen() );

    auto videos = rmg->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 2u, videos.size() );
    for ( const auto& m : videos )
        rmg->remove( *m );

    groups = T->ml->mediaGroups( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    for ( const auto& g : groups )
    {
        ASSERT_TRUE( static_cast<MediaGroup*>( g.get() )->isForcedSingleton() );
    }
    groups = T->ml->mediaGroups( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
}

static void GenrePresence( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto genre = T->ml->createGenre( "test genre" );
    // Create an album with removable and non removable tracks
    auto album = std::static_pointer_cast<Album>( T->ml->createAlbum( "album" ) );
    auto media1 = std::static_pointer_cast<Media>( T->ml->media( mock::FileSystemFactory::Root + "audio.mp3" ) );
    auto media2 = std::static_pointer_cast<Media>(
                T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" ) );
    auto media3 = std::static_pointer_cast<Media>(
                T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile2.mp3" ) );
    album->addTrack( std::static_pointer_cast<Media>( media1 ), 1, 1, 0, genre.get() );
    album->addTrack( std::static_pointer_cast<Media>( media2 ), 2, 1, 0, genre.get() );
    album->addTrack( std::static_pointer_cast<Media>( media3 ), 3, 1, 0, genre.get() );
    media1->save();
    media2->save();
    media3->save();

    /* We should now have 3 tracks in the genre. 1 non-removable and 2 removable */
    genre = std::static_pointer_cast<Genre>( T->ml->genre( genre->id() ) );
    ASSERT_EQ( 3u, genre->nbTracks() );
    ASSERT_EQ( 3u, genre->nbPresentTracks() );

    /*
     * Remove the device, the genre is still present and nb_tracks isn't changed
     * since the tracks still exist
     */
    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );
    T->Reload();

    genre = std::static_pointer_cast<Genre>( T->ml->genre( genre->id() ) );
    ASSERT_EQ( 3u, genre->nbTracks() );
    ASSERT_EQ( 1u, genre->nbPresentTracks() );

    /* Remove the present track */
    T->ml->deleteMedia( media1->id() );

    /* The genre shouldn't have any remaining media and should now be missing */
    genre = std::static_pointer_cast<Genre>( T->ml->genre( genre->id() ) );
    ASSERT_EQ( 2u, genre->nbTracks() );
    ASSERT_EQ( 0u, genre->nbPresentTracks() );

    T->fsMock->addDevice( device );
    T->Reload();

    genre = std::static_pointer_cast<Genre>( T->ml->genre( genre->id() ) );
    ASSERT_EQ( 2u, genre->nbTracks() );
    ASSERT_EQ( 2u, genre->nbPresentTracks() );
}

static void PlaylistPresence( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    T->enforceFakeMediaTypes();

    auto pl = T->ml->createPlaylist( "test playlist" );
    auto pl2 = T->ml->createPlaylist( "test playlist 2" );
    auto pl3 = T->ml->createPlaylist( "video playlist" );
    auto media1 = std::static_pointer_cast<Media>( T->ml->media( mock::FileSystemFactory::Root + "audio.mp3" ) );
    auto media2 = std::static_pointer_cast<Media>( T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablefile.mp3" ) );
    auto media3 = std::static_pointer_cast<Media>( T->ml->media( DeviceFsTests::RemovableDeviceMountpoint + "removablevideo.mkv" ) );

    auto res = pl->append( *media1 );
    ASSERT_TRUE( res );
    res = pl->append( *media2 );
    ASSERT_TRUE( res );

    res = pl2->append( *media1 );
    ASSERT_TRUE( res );

    res = pl3->append( *media3 );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, pl->nbMedia() );
    ASSERT_EQ( 2u, pl->nbPresentMedia() );
    ASSERT_EQ( 2u, pl->nbAudio() );
    ASSERT_EQ( 2u, pl->nbPresentAudio() );
    ASSERT_EQ( 1u, pl2->nbMedia() );
    ASSERT_EQ( 1u, pl2->nbPresentMedia() );
    ASSERT_EQ( 1u, pl2->nbAudio() );
    ASSERT_EQ( 1u, pl2->nbPresentAudio() );
    ASSERT_EQ( 1u, pl3->nbMedia() );
    ASSERT_EQ( 1u, pl3->nbVideo() );
    ASSERT_EQ( 1u, pl3->nbPresentMedia() );
    ASSERT_EQ( 1u, pl3->nbPresentVideo() );

    pl = std::static_pointer_cast<Playlist>( T->ml->playlist( pl->id() ) );
    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    pl3 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl3->id() ) );

    ASSERT_EQ( 2u, pl->nbMedia() );
    ASSERT_EQ( 2u, pl->nbPresentMedia() );
    ASSERT_EQ( 2u, pl->nbAudio() );
    ASSERT_EQ( 2u, pl->nbPresentAudio() );
    ASSERT_EQ( 1u, pl2->nbMedia() );
    ASSERT_EQ( 1u, pl2->nbPresentMedia() );
    ASSERT_EQ( 1u, pl2->nbAudio() );
    ASSERT_EQ( 1u, pl2->nbPresentAudio() );
    ASSERT_EQ( 1u, pl3->nbMedia() );
    ASSERT_EQ( 1u, pl3->nbVideo() );
    ASSERT_EQ( 1u, pl3->nbPresentMedia() );
    ASSERT_EQ( 1u, pl3->nbPresentVideo() );

    auto videoPlaylists = T->ml->playlists( PlaylistType::VideoOnly, nullptr )->all();
    ASSERT_EQ( 1u, videoPlaylists.size() );
    ASSERT_EQ( pl3->id(), videoPlaylists[0]->id() );

    auto device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );
    T->Reload();

    pl = std::static_pointer_cast<Playlist>( T->ml->playlist( pl->id() ) );
    ASSERT_EQ( 2u, pl->nbMedia() );
    ASSERT_EQ( 2u, pl->nbAudio() );
    ASSERT_EQ( 1u, pl->nbPresentMedia() );
    ASSERT_EQ( 1u, pl->nbPresentAudio() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbMedia() );
    ASSERT_EQ( 1u, pl2->nbPresentMedia() );
    ASSERT_EQ( 1u, pl2->nbAudio() );
    ASSERT_EQ( 1u, pl2->nbPresentAudio() );

    QueryParameters params{};
    auto plMedia = pl->media( &params )->all();
    ASSERT_EQ( 1u, plMedia.size() );
    auto nbMedia = pl->media( &params )->count();
    ASSERT_EQ( 1u, nbMedia );

    plMedia = pl2->media( &params )->all();
    ASSERT_EQ( 1u, plMedia.size() );
    nbMedia = pl2->media( &params )->count();
    ASSERT_EQ( 1u, nbMedia );

    videoPlaylists = T->ml->playlists( PlaylistType::VideoOnly, &params )->all();
    ASSERT_EQ( 0u, videoPlaylists.size() );

    params.includeMissing = true;
    plMedia = pl->media( &params )->all();
    ASSERT_EQ( 2u, plMedia.size() );
    nbMedia = pl->media( &params )->count();
    ASSERT_EQ( 2u, nbMedia );

    plMedia = pl2->media( &params )->all();
    ASSERT_EQ( 1u, plMedia.size() );
    nbMedia = pl2->media( &params )->count();
    ASSERT_EQ( 1u, nbMedia );

    videoPlaylists = T->ml->playlists( PlaylistType::VideoOnly, &params )->all();
    ASSERT_EQ( 1u, videoPlaylists.size() );
    ASSERT_EQ( pl3->id(), videoPlaylists[0]->id() );

    T->fsMock->addDevice( device );
    T->Reload();

    plMedia = pl->media( &params )->all();
    ASSERT_EQ( 2u, plMedia.size() );
    nbMedia = pl->media( &params )->count();
    ASSERT_EQ( 2u, nbMedia );

    plMedia = pl2->media( &params )->all();
    ASSERT_EQ( 1u, plMedia.size() );
    nbMedia = pl2->media( &params )->count();
    ASSERT_EQ( 1u, nbMedia );

    /*
     * Moving a playlist item is implemented by removing/adding, so check that
     * it doesn't wreck the counters
     */
    res = pl->move( 1, 999 );
    ASSERT_TRUE( res );

    pl = std::static_pointer_cast<Playlist>( T->ml->playlist( pl->id() ) );
    ASSERT_EQ( 2u, pl->nbMedia() );
    ASSERT_EQ( 2u, pl->nbPresentMedia() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbMedia() );
    ASSERT_EQ( 1u, pl2->nbPresentMedia() );

    pl = std::static_pointer_cast<Playlist>( T->ml->playlist( pl->id() ) );
    ASSERT_EQ( 2u, pl->nbMedia() );
    ASSERT_EQ( 2u, pl->nbPresentMedia() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbMedia() );
    ASSERT_EQ( 1u, pl2->nbPresentMedia() );

    /*
     * Now try to update the type of media which is handled by the same trigger
     * as the presence
     */
    res = media1->setType( IMedia::Type::Video );
    ASSERT_TRUE( res );

    pl = std::static_pointer_cast<Playlist>( T->ml->playlist( pl->id() ) );
    ASSERT_EQ( 2u, pl->nbMedia() );
    ASSERT_EQ( 2u, pl->nbPresentMedia() );
    ASSERT_EQ( 1u, pl->nbPresentAudio() );
    ASSERT_EQ( 1u, pl->nbPresentVideo() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbMedia() );
    ASSERT_EQ( 1u, pl2->nbVideo() );
    ASSERT_EQ( 1u, pl2->nbPresentVideo() );
    ASSERT_EQ( 1u, pl2->nbPresentMedia() );

    res = media2->setType( IMedia::Type::Video );
    ASSERT_TRUE( res );

    pl = std::static_pointer_cast<Playlist>( T->ml->playlist( pl->id() ) );
    ASSERT_EQ( 2u, pl->nbMedia() );
    ASSERT_EQ( 2u, pl->nbPresentMedia() );
    ASSERT_EQ( 0u, pl->nbPresentAudio() );
    ASSERT_EQ( 2u, pl->nbPresentVideo() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbMedia() );
    ASSERT_EQ( 0u, pl2->nbPresentAudio() );
    ASSERT_EQ( 1u, pl2->nbPresentVideo() );
    ASSERT_EQ( 1u, pl2->nbPresentMedia() );

    device = T->fsMock->removeDevice( DeviceFsTests::RemovableDeviceUuid );
    T->Reload();

    pl = std::static_pointer_cast<Playlist>( T->ml->playlist( pl->id() ) );
    ASSERT_EQ( 2u, pl->nbMedia() );
    ASSERT_EQ( 1u, pl->nbPresentMedia() );
    ASSERT_EQ( 1u, pl->nbPresentVideo() );
    ASSERT_EQ( 2u, pl->nbVideo() );

    T->ml->deleteMedia( media2->id() );

    pl = std::static_pointer_cast<Playlist>( T->ml->playlist( pl->id() ) );
    ASSERT_EQ( 1u, pl->nbMedia() );
    ASSERT_EQ( 1u, pl->nbPresentMedia() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbMedia() );
    ASSERT_EQ( 1u, pl2->nbPresentMedia() );

    T->ml->deleteMedia( media1->id() );

    pl = std::static_pointer_cast<Playlist>( T->ml->playlist( pl->id() ) );
    ASSERT_EQ( 0u, pl->nbMedia() );
    ASSERT_EQ( 0u, pl->nbPresentMedia() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 0u, pl2->nbMedia() );
    ASSERT_EQ( 0u, pl2->nbPresentMedia() );
}

static void FolderPresence( DeviceFsTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
    T->enforceFakeMediaTypes();

    QueryParameters params{};
    auto folders = T->ml->folders( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 3u, folders.size() );
    params.includeMissing = true;
    folders = T->ml->folders( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 3u, folders.size() );

    T->fsMock->removeDevice( T->RemovableDeviceUuid );
    T->Reload();

    folders = T->ml->folders( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 3u, folders.size() );

    params.includeMissing = false;
    folders = T->ml->folders( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, folders.size() );
}

static void CompareMountpoints( DeviceFsTests* T )
{
    auto d = T->fsMock->addDevice( "smb://1.2.3.4:445", "{fake-uuid}", true );
    auto doCheck = [&d]( const std::string& mrl, bool exp ) {
        auto res = d->matchesMountpoint( mrl );
        ASSERT_EQ( std::get<0>( res ), exp );
    };
    doCheck( "SMB://1.2.3.4:445", true );
    doCheck( "SMB://1.2.3.4", true );
    doCheck( "SMB://1.2.3.4:666", false );
    doCheck( "SMB://1.2.3.4:445/foo/bar", true );
    doCheck( "SMB://1.2.3.4/foo/bar", true );
    doCheck( "sMb://1.3.1.2", false );
    doCheck( "upnp://1.2.3.4:445", false );
    doCheck( "upnp://1.2.3.4", false );
    doCheck( "smb://1.2.3.4/////////", true );
    doCheck( "smb://1.2.3.4:445/", true );
}

static void RelativeMrl( DeviceFsTests* T )
{
    auto d = T->fsMock->addDevice( "smb://1.2.3.4:445", "{fake-uuid}", true );
    d->addMountpoint( "smb://SHARENAME/" );
    auto doCheck = [&d]( const std::string& mrl, const std::string& exp ) {
        auto relativeMrl = d->relativeMrl( mrl );
        ASSERT_EQ( relativeMrl, exp );
    };

    doCheck( "smb://1.2.3.4:445/a/b/c", "a/b/c" );
    doCheck( "smb://1.2.3.4/a/b/c", "a/b/c" );
    doCheck( "smb://SHARENAME/a/b/c", "a/b/c" );
    doCheck( "smb://SHARENAME:445/a/b/c", "a/b/c" );
    doCheck( "smb://SHARENAME:445", "" );
    /*
     * relative mrl doesn't remove useless path separators, but manage to return
     * something sensible
     */
    doCheck( "smb://SHARENAME:445/////", "////" );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( DeviceFsTests )
    ADD_TEST( RemoveDisk );
    ADD_TEST( UnmountDisk );
    ADD_TEST( ReplugDisk );
    ADD_TEST( ReplugDiskWithExtraFiles );
    ADD_TEST( RemoveAlbumAndArtist );
    ADD_TEST( RemoveArtist );
    ADD_TEST( PartialAlbumRemoval );
    ADD_TEST( ChangeDevice );
    ADD_TEST( UnknownMountpoint );
    ADD_TEST( OutdatedDevices );
    ADD_TEST( RemovableMountPointName );
    ADD_TEST( RemoveShowEpisodes );
    ADD_TEST( PartialRemoveShowEpisodes );
    ADD_TEST( MediaGroupPresence );
    ADD_TEST( GenrePresence );
    ADD_TEST( PlaylistPresence );
    ADD_TEST( FolderPresence );
    ADD_TEST( CompareMountpoints );
    ADD_TEST( RelativeMrl );

    END_TESTS
}
