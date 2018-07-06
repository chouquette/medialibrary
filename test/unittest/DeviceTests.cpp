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

#include "Tests.h"

#include "Album.h"
#include "Device.h"
#include "File.h"
#include "Media.h"
#include "Artist.h"
#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"

class DeviceEntity : public Tests
{
};

class DeviceFs : public Tests
{
protected:
    static const std::string RemovableDeviceUuid;
    static const std::string RemovableDeviceMountpoint;
    std::shared_ptr<mock::FileSystemFactory> fsMock;
    std::unique_ptr<mock::WaitForDiscoveryComplete> cbMock;

protected:
    virtual void SetUp() override
    {
        unlink( "test.db" );
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        fsMock->addFolder( "file:///a/mnt/" );
        auto device = fsMock->addDevice( RemovableDeviceMountpoint, RemovableDeviceUuid );
        device->setRemovable( true );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile.mp3" );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile2.mp3" );
        Reload();
    }

    virtual void InstantiateMediaLibrary() override
    {
        ml.reset( new MediaLibraryWithDiscoverer );
    }

    virtual void Reload()
    {
        Tests::Reload( fsMock, cbMock.get() );
        auto res = cbMock->waitReload();
        ASSERT_TRUE( res );
    }
};

const std::string DeviceFs::RemovableDeviceUuid = "{fake-removable-device}";
const std::string DeviceFs::RemovableDeviceMountpoint = "file:///a/mnt/fake-device/";

// Database/Entity tests

TEST_F( DeviceEntity, Create )
{
    auto d = ml->addDevice( "dummy", true );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    ASSERT_TRUE( d->isPresent() );

    Reload();

    d = ml->device( "dummy" );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    // Since there is no FsFactory to testify that this device is still plugged, the reload
    // will mark it as "absent"
    ASSERT_FALSE( d->isPresent() );
}

TEST_F( DeviceEntity, SetPresent )
{
    auto d = ml->addDevice( "dummy", true );
    ASSERT_NE( nullptr, d );
    ASSERT_TRUE( d->isPresent() );

    d->setPresent( false );
    ASSERT_FALSE( d->isPresent() );

    Reload();

    d = ml->device( "dummy" );
    ASSERT_FALSE( d->isPresent() );
}

// Filesystem tests:

TEST_F( DeviceFs, RemoveDisk )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 5u, files.size() );

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
    ASSERT_EQ( 5u, files.size() );

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
    ASSERT_EQ( 5u, files.size() );

    media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );
}

TEST_F( DeviceFs, ReplugDisk )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 5u, files.size() );

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
    ASSERT_EQ( 5u, files.size() );

    media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, media );
}

TEST_F( DeviceFs, ReplugDiskWithExtraFiles )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 5u, files.size() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );

    Reload();

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    fsMock->addDevice( device );
    fsMock->addFile( RemovableDeviceMountpoint + "newfile.mkv" );

    Reload();

    files = ml->files();
    ASSERT_EQ( 6u, files.size() );
}

TEST_F( DeviceFs, RemoveAlbum )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    // Create an album on a non-removable device
    {
        auto album = std::static_pointer_cast<Album>( ml->createAlbum( "album" ) );
        auto media = ml->media( mock::FileSystemFactory::Root + "audio.mp3" );
        auto artist = ml->createArtist( "artist" );
        album->addTrack( std::static_pointer_cast<Media>( media ), 1, 1, artist->id(), 0 );
        album->setAlbumArtist( artist );
    }
    // And an album that will disappear, along with its artist
    {
        auto album = std::static_pointer_cast<Album>( ml->createAlbum( "album 2" ) );
        auto album2 = std::static_pointer_cast<Album>( ml->createAlbum( "album 3" ) );
        auto media = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
        auto artist = ml->createArtist( "artist 2" );
        album->addTrack( std::static_pointer_cast<Media>( media ), 1, 1, artist->id(), nullptr );
        album->addTrack( std::static_pointer_cast<Media>( media ), 2, 1, artist->id(), nullptr );
        album2->addTrack( std::static_pointer_cast<Media>( media ), 1, 1, artist->id(), nullptr );
        album2->addTrack( std::static_pointer_cast<Media>( media ), 2, 1, artist->id(), nullptr );
        album->setAlbumArtist( artist );
        album2->setAlbumArtist( artist );
    }

    auto albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 3u, albums.size() );
    auto artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );

    Reload();

    albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );

    // Now check that everything appears again when we plug the device back in

    fsMock->addDevice( device );

    Reload();

    albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 3u, albums.size() );
    artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );
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

    auto albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    auto artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
    auto artist = artists[0];
    ASSERT_EQ( 2u, artist->tracks( nullptr )->count() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );
    Reload();

    albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
    artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( 1u, albums[0]->tracks( nullptr )->count() );
    ASSERT_EQ( 1u, artists[0]->tracks( nullptr )->count() );
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
    fsMock->addDevice( RemovableDeviceMountpoint, "{another-removable-device}" );
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
