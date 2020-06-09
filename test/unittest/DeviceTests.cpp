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

#include "Tests.h"

#include "Album.h"
#include "Device.h"
#include "File.h"
#include "Media.h"
#include "Artist.h"
#include "Show.h"
#include "MediaGroup.h"

#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"

namespace
{

class MediaLibraryTesterDevices : public MediaLibraryTester
{
    virtual bool setupDummyFolder() override { return true; }
};

}

class DeviceEntity : public Tests
{
    virtual void InstantiateMediaLibrary() override
    {
        ml.reset( new MediaLibraryTesterDevices );
    }
};

class DeviceFs : public Tests
{
protected:
    static const std::string RemovableDeviceUuid;
    static const std::string RemovableDeviceMountpoint;
    std::shared_ptr<mock::FileSystemFactory> fsMock;
    std::unique_ptr<mock::WaitForDiscoveryComplete> cbMock;
    static constexpr auto NbRemovableMedia = 6u;

protected:
    virtual void SetUp() override
    {
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        fsMock->addFolder( "file:///a/mnt/" );
        auto device = fsMock->addDevice( RemovableDeviceMountpoint, RemovableDeviceUuid, true );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile.mp3" );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile2.mp3" );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile3.mp3" );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile4.mp3" );
        fsMock->addFile( RemovableDeviceMountpoint + "removablevideo.mkv" );
        fsMock->addFile( RemovableDeviceMountpoint + "removablevideo2.mkv" );
        fsFactory = fsMock;
        mlCb = cbMock.get();
        Tests::SetUp();
    }

    virtual void InstantiateMediaLibrary() override
    {
        ml.reset( new MediaLibraryWithDiscoverer );
    }

    virtual void Reload() override
    {
        Tests::Reload();
        ml->reload();
        auto res = cbMock->waitReload();
        ASSERT_TRUE( res );
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
                                        RemovableDeviceMountpoint + "removablefile.mp3" ) );
        media->setType( IMedia::Type::Audio );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        RemovableDeviceMountpoint + "removablefile2.mp3" ) );
        media->setType( IMedia::Type::Audio );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        RemovableDeviceMountpoint + "removablefile3.mp3" ) );
        media->setType( IMedia::Type::Audio );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        RemovableDeviceMountpoint + "removablefile4.mp3" ) );
        media->setType( IMedia::Type::Audio );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        RemovableDeviceMountpoint + "removablevideo.mkv" ) );
        media->setType( IMedia::Type::Video );
        media->save();

        media = std::static_pointer_cast<Media>( ml->media(
                                        RemovableDeviceMountpoint + "removablevideo2.mkv" ) );
        media->setType( IMedia::Type::Video );
        media->save();
    }
};

const std::string DeviceFs::RemovableDeviceUuid = "{fake-removable-device}";
const std::string DeviceFs::RemovableDeviceMountpoint = "file:///a/mnt/fake-device/";

// Database/Entity tests

TEST_F( DeviceEntity, Create )
{
    auto d = ml->addDevice( "dummy", "file://", true );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    ASSERT_TRUE( d->isPresent() );

    d = ml->device( "dummy", "file://" );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    // The device won't be marked absent until the discoverer starts, but it won't
    // happen in unit case configuration
}

TEST_F( DeviceEntity, SetPresent )
{
    auto d = ml->addDevice( "dummy", "file://", true );
    ASSERT_NE( nullptr, d );
    ASSERT_TRUE( d->isPresent() );

    d->setPresent( false );
    ASSERT_FALSE( d->isPresent() );

    d = ml->device( "dummy", "file://" );
    ASSERT_FALSE( d->isPresent() );
}

TEST_F( DeviceEntity, CheckDbModel )
{
    auto res = Device::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( DeviceEntity, MultipleScheme )
{
    auto d1 = ml->addDevice( "dummy", "file://", false );
    auto d2 = ml->addDevice( "dummy", "smb://", true );
    ASSERT_NE( nullptr, d1 );
    ASSERT_NE( nullptr, d2 );
    ASSERT_NE( d1->id(), d2->id() );

    auto d = Device::fromUuid( ml.get(), d1->uuid(), d1->scheme() );
    ASSERT_EQ( d->id(), d1->id() );

    d = Device::fromUuid( ml.get(), d2->uuid(), d2->scheme() );
    ASSERT_EQ( d->id(), d2->id() );
}

TEST_F( DeviceEntity, IsKnown )
{
    const std::string uuid{ "{device-uuid}" };
    const std::string localMountpoint{ "file:///path/to/device" };
    const std::string smbMountpoint{ "smb:///1.3.1.2/" };

    auto res = ml->isDeviceKnown( uuid, localMountpoint, false );
    ASSERT_FALSE( res );
    res = ml->isDeviceKnown( uuid, localMountpoint, false );
    ASSERT_TRUE( res );

    /* Ensure the removable flag doesn't change anything regarding probing */
    res = ml->isDeviceKnown( uuid, localMountpoint, true );
    ASSERT_TRUE( res );

    /*
     * Now check that device are uniquely identified by their scheme & uuid, not
     * just their uuid
     */
    res = ml->isDeviceKnown( uuid, smbMountpoint, false );
    ASSERT_FALSE( res );
    res = ml->isDeviceKnown( uuid, smbMountpoint, false );
    ASSERT_TRUE( res );
}

TEST_F( DeviceEntity, DeleteRemovable )
{
    auto d = Device::create( ml.get(), "fake-device", "file://", false, false );
    ASSERT_NE( nullptr, d );
    d = Device::create( ml.get(), "fake-removable-device", "file://", true, false );
    ASSERT_NE( nullptr, d );
    d = Device::create( ml.get(), "another-removable-device", "file://", true, false );
    ASSERT_NE( nullptr, d );

    auto devices = Device::fetchAll( ml.get() );
    ASSERT_EQ( 3u, devices.size() );

    auto res = ml->deleteRemovableDevices();
    ASSERT_TRUE( res );

    devices = Device::fetchAll( ml.get() );
    ASSERT_EQ( 1u, devices.size() );
}

// Filesystem tests:

TEST_F( DeviceFs, RemoveDisk )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u + NbRemovableMedia, files.size() );

    auto media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );

    Reload();

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, media );
}

TEST_F( DeviceFs, UnmountDisk )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u + NbRemovableMedia, files.size() );

    auto media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );

    fsMock->unmountDevice( RemovableDeviceUuid );

    Reload();

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, media );

    fsMock->remountDevice( RemovableDeviceUuid );

    Reload();

    files = ml->files();
    ASSERT_EQ( 3u + NbRemovableMedia, files.size() );

    media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );
}

TEST_F( DeviceFs, ReplugDisk )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u + NbRemovableMedia, files.size() );

    auto media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );

    Reload();

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, media );

    fsMock->addDevice( device );
    Reload();

    files = ml->files();
    ASSERT_EQ( 3u + NbRemovableMedia, files.size() );

    media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );
}

TEST_F( DeviceFs, ReplugDiskWithExtraFiles )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u + NbRemovableMedia, files.size() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );

    Reload();

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    fsMock->addDevice( device );
    fsMock->addFile( RemovableDeviceMountpoint + "newfile.mkv" );

    Reload();

    files = ml->files();
    ASSERT_EQ( 3u + NbRemovableMedia + 1, files.size() );
}

TEST_F( DeviceFs, RemoveAlbumAndArtist )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    // Create an album on a non-removable device
    {
        auto album = std::static_pointer_cast<Album>( ml->createAlbum( "album" ) );
        auto media = std::static_pointer_cast<Media>( ml->media( mock::FileSystemFactory::Root + "audio.mp3" ) );
        auto artist = ml->createArtist( "artist" );
        album->addTrack( std::static_pointer_cast<Media>( media ), 1, 1, artist->id(), 0 );
        album->setAlbumArtist( artist );
        artist->addMedia( *media );
    }
    // And an album that will disappear, along with its artist
    {
        auto album = std::static_pointer_cast<Album>( ml->createAlbum( "album 2" ) );
        auto album2 = std::static_pointer_cast<Album>( ml->createAlbum( "album 3" ) );
        auto media1 = std::static_pointer_cast<Media>( ml->media( RemovableDeviceMountpoint + "removablefile.mp3" ) );
        auto media2 = std::static_pointer_cast<Media>( ml->media( RemovableDeviceMountpoint + "removablefile2.mp3" ) );
        auto media3 = std::static_pointer_cast<Media>( ml->media( RemovableDeviceMountpoint + "removablefile3.mp3" ) );
        auto media4 = std::static_pointer_cast<Media>( ml->media( RemovableDeviceMountpoint + "removablefile4.mp3" ) );
        auto artist = ml->createArtist( "artist 2" );
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

    auto res = Artist::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );

    auto albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 3u, albums.size() );
    auto artists = ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );

    Reload();

    albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );

    // Now check that everything appears again when we plug the device back in

    fsMock->addDevice( device );

    Reload();

    albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 3u, albums.size() );
    artists = ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );

    res = Artist::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( DeviceFs, RemoveArtist )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    // Check that an artist with a track remaining but album is still present
    // Album artist disappearance is already tested by RemoveAlbumAndArtist test
    auto artist = ml->createArtist( "removable artist" );

    auto album = std::static_pointer_cast<Album>( ml->createAlbum( "removable album" ) );
    auto media1 = std::static_pointer_cast<Media>( ml->media( RemovableDeviceMountpoint + "removablefile.mp3" ) );
    auto media2 = std::static_pointer_cast<Media>( ml->media( RemovableDeviceMountpoint + "removablefile2.mp3" ) );
    auto media3 = std::static_pointer_cast<Media>( ml->media( mock::FileSystemFactory::Root + "audio.mp3" ) );

    album->addTrack( std::static_pointer_cast<Media>( media1 ), 1, 1, artist->id(), 0 );
    album->addTrack( std::static_pointer_cast<Media>( media2 ), 2, 1, artist->id(), 0 );
    album->addTrack( std::static_pointer_cast<Media>( media3 ), 3, 1, artist->id(), 0 );
    artist->addMedia( *media1 );
    artist->addMedia( *media2 );
    artist->addMedia( *media3 );

    auto res = Artist::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );

    auto albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    auto artists = ml->artists( ArtistIncluded::All, nullptr )->all();

    ASSERT_EQ( 1u, artists.size() );

    QueryParameters params;
    params.sort = SortingCriteria::Alpha;
    params.desc = false;
    auto tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 3u, tracks.size() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );

    Reload();

    // nothing should have changed as far as the artist count goes

    albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );

    // But we expect the tracks count to be down
    artist = std::static_pointer_cast<Artist>( ml->artist( artist->id() ) );
    tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 1u, tracks.size() );

    // Now check that everything appears again when we plug the device back in

    fsMock->addDevice( device );

    Reload();

    albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
    artist = std::static_pointer_cast<Artist>( ml->artist( artist->id() ) );
    tracks = artist->tracks( nullptr )->all();
    ASSERT_EQ( 3u, tracks.size() );

    res = Artist::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );
}


TEST_F( DeviceFs, PartialAlbumRemoval )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    {
        auto album = ml->createAlbum( "album" );
        auto media = ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
        auto media2 = ml->media( RemovableDeviceMountpoint + "removablefile2.mp3" );
        auto newArtist = ml->createArtist( "artist" );
        album->addTrack( std::static_pointer_cast<Media>( media ), 1, 1, newArtist->id(), nullptr );
        album->addTrack( std::static_pointer_cast<Media>( media2 ), 2, 1, newArtist->id(), nullptr );
        album->setAlbumArtist( newArtist );
        newArtist->addMedia( static_cast<Media&>( *media ) );
        newArtist->addMedia( static_cast<Media&>( *media2 ) );
    }

    auto res = Artist::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );

    auto albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    auto artists = ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
    auto artist = artists[0];
    ASSERT_EQ( 2u, artist->tracks( nullptr )->count() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );
    Reload();

    albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( 1u, albums[0]->tracks( nullptr )->count() );
    ASSERT_EQ( 1u, artists[0]->tracks( nullptr )->count() );

    fsMock->addDevice( device );

    Reload();

    res = Artist::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );
    res = Album::checkDBConsistency( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( DeviceFs, ChangeDevice )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    // Fetch a removable media's ID
    auto f = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( f, nullptr );
    auto firstRemovableFileId = f->id();
    auto files = f->files();
    ASSERT_EQ( 1u, files.size() );
    auto firstRemovableFilePath = files[0]->mrl();

    // Remove & store the device
    auto oldRemovableDevice = fsMock->removeDevice( RemovableDeviceUuid );

    // Add a new device on the same mountpoint
    fsMock->addDevice( RemovableDeviceMountpoint, "{another-removable-device}", true );
    fsMock->addFile( RemovableDeviceMountpoint + "removablefile.mp3" );

    Reload();

    // Check that new files with the same name have different IDs
    // but the same "full path"
    f = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, f );
    files = f->files();
    ASSERT_EQ( 1u, files.size() );
    ASSERT_EQ( firstRemovableFilePath, files[0]->mrl() );
    ASSERT_NE( firstRemovableFileId, f->id() );

    auto device = fsMock->removeDevice( "{another-removable-device}" );
    fsMock->addDevice( oldRemovableDevice );

    Reload();

    f = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, f );
    ASSERT_EQ( firstRemovableFileId, f->id() );
}

TEST_F( DeviceFs, UnknownMountpoint )
{
    // The mock filesystem starts at /a/
    // Simply check that we don't crash
    ml->discover( "file:///" );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
}

TEST_F( DeviceFs, OutdatedDevices )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u + NbRemovableMedia, ml->files().size() );
    auto oldMediaCount = ml->files().size();

    ml->outdateAllDevices();
    fsMock->removeDevice( RemovableDeviceUuid );

    Reload();

    ASSERT_NE( oldMediaCount, ml->files().size() );
}

TEST_F( DeviceFs, RemovableMountPointName )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( RemovableDeviceMountpoint );
    ASSERT_NE( nullptr, f );
    ASSERT_NE( 0u, f->name().size() );
}

TEST_F( DeviceFs, RemoveShowEpisodes )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto show1 = ml->createShow( "Show1" );
    auto media1 = std::static_pointer_cast<Media>(
                ml->media( RemovableDeviceMountpoint + "removablevideo.mkv" ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    media1->save();
    auto media2 = std::static_pointer_cast<Media>(
                ml->media( RemovableDeviceMountpoint + "removablevideo2.mkv" ) );
    show1->addEpisode( *media2, 1, 2, "episode title" );
    media2->save();

    auto showsQuery = ml->shows( nullptr );
    ASSERT_EQ( 1u, showsQuery->count() );
    ASSERT_EQ( 1u, showsQuery->all().size() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );
    Reload();

    showsQuery = ml->shows( nullptr );
    ASSERT_EQ( 0u, showsQuery->count() );
    ASSERT_EQ( 0u, showsQuery->all().size() );

    fsMock->addDevice( device );
    Reload();

    showsQuery = ml->shows( nullptr );
    ASSERT_EQ( 1u, showsQuery->count() );
    ASSERT_EQ( 1u, showsQuery->all().size() );
}

TEST_F( DeviceFs, PartialRemoveShowEpisodes )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto show1 = ml->createShow( "Show1" );
    auto media1 = std::static_pointer_cast<Media>(
                ml->media( mock::FileSystemFactory::Root + "video.avi" ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    media1->save();

    auto media2 = std::static_pointer_cast<Media>(
                ml->media( RemovableDeviceMountpoint + "removablevideo.mkv" ) );
    show1->addEpisode( *media2, 1, 2, "episode title" );
    media2->save();

    auto shows = ml->shows( nullptr )->all();
    ASSERT_EQ( 1u, shows.size() );

    auto episodeQuery = shows[0]->episodes( nullptr );
    ASSERT_EQ( 2u, episodeQuery->count() );
    ASSERT_EQ( 2u, episodeQuery->all().size() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );
    Reload();

    shows = ml->shows( nullptr )->all();
    ASSERT_EQ( 1u, shows.size() );

    episodeQuery = shows[0]->episodes( nullptr );
    ASSERT_EQ( 1u, episodeQuery->count() );
    ASSERT_EQ( 1u, episodeQuery->all().size() );

    fsMock->addDevice( device );
    Reload();

    shows = ml->shows( nullptr )->all();
    ASSERT_EQ( 1u, shows.size() );

    episodeQuery = shows[0]->episodes( nullptr );
    ASSERT_EQ( 2u, episodeQuery->count() );
    ASSERT_EQ( 2u, episodeQuery->all().size() );
}

TEST_F( DeviceFs, MediaGroupPresence )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes();

    auto addToGroup = [this]( IMediaGroup& mg, const std::string& mrl ) {
        auto m = ml->media( mrl );
        ASSERT_NE( nullptr, m );
        auto res = mg.add( *m );
        ASSERT_TRUE( res );
    };

    auto rmg = ml->createMediaGroup( "removable group" );

    addToGroup( *rmg, RemovableDeviceMountpoint + "removablefile.mp3" );
    addToGroup( *rmg, RemovableDeviceMountpoint + "removablefile2.mp3" );
    addToGroup( *rmg, RemovableDeviceMountpoint + "removablefile3.mp3" );
    addToGroup( *rmg, RemovableDeviceMountpoint + "removablefile4.mp3" );
    addToGroup( *rmg, RemovableDeviceMountpoint + "removablevideo.mkv" );
    addToGroup( *rmg, RemovableDeviceMountpoint + "removablevideo2.mkv" );

    ASSERT_EQ( 4u, rmg->nbAudio() );
    ASSERT_EQ( 2u, rmg->nbVideo() );
    ASSERT_EQ( 0u, rmg->nbUnknown() );
    ASSERT_EQ( 6u, rmg->nbMedia() );

    auto groups = ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    rmg = ml->mediaGroup( rmg->id() );

    ASSERT_EQ( 4u, rmg->nbAudio() );
    ASSERT_EQ( 2u, rmg->nbVideo() );
    ASSERT_EQ( 0u, rmg->nbUnknown() );
    ASSERT_EQ( 6u, rmg->nbMedia() );
    ASSERT_EQ( 6u, rmg->nbTotalMedia() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );
    Reload();

    rmg = ml->mediaGroup( rmg->id() );

    ASSERT_EQ( 0u, rmg->nbAudio() );
    ASSERT_EQ( 0u, rmg->nbVideo() );
    ASSERT_EQ( 0u, rmg->nbUnknown() );
    ASSERT_EQ( 0u, rmg->nbMedia() );
    ASSERT_EQ( 6u, rmg->nbTotalMedia() );

    groups = ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );

    fsMock->addDevice( device );
    Reload();

    groups = ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    rmg = ml->mediaGroup( rmg->id() );

    ASSERT_EQ( 4u, rmg->nbAudio() );
    ASSERT_EQ( 2u, rmg->nbVideo() );
    ASSERT_EQ( 0u, rmg->nbUnknown() );
    ASSERT_EQ( 6u, rmg->nbMedia() );

    auto videos = rmg->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 2u, videos.size() );
    for ( const auto& m : videos )
        rmg->remove( *m );

    groups = ml->mediaGroups( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    for ( const auto& g : groups )
    {
        ASSERT_TRUE( static_cast<MediaGroup*>( g.get() )->isForcedSingleton() );
    }
    groups = ml->mediaGroups( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
}
