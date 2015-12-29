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

#include "Tests.h"

#include "Album.h"
#include "Device.h"
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
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        fsMock->addFolder( "/a/mnt/" );
        fsMock->addDevice( RemovableDeviceMountpoint, RemovableDeviceUuid );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile.mp3" );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile2.mp3" );
        Reload();
    }

    virtual void InstantiateMediaLibrary() override
    {
        ml.reset( new MediaLibraryWithoutParser );
    }

    virtual void Reload()
    {
        Tests::Reload( fsMock, cbMock.get() );
    }
};

const std::string DeviceFs::RemovableDeviceUuid = "{fake-removable-device}";
const std::string DeviceFs::RemovableDeviceMountpoint = "/a/mnt/fake-device/";

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
    ASSERT_TRUE( d->isPresent() );
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
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 5u, files.size() );

    auto file = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, file );

    fsMock->removeDevice( RemovableDeviceUuid );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    file = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, file );
}

TEST_F( DeviceFs, UnmountDisk )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 5u, files.size() );

    auto file = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, file );

    auto device = std::static_pointer_cast<mock::Device>( fsMock->createDevice( RemovableDeviceUuid ) );
    device->setPresent( false );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    file = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, file );
}

TEST_F( DeviceFs, ReplugDisk )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 5u, files.size() );

    auto file = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, file );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    file = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, file );

    fsMock->addDevice( device );
    cbMock->prepareForReload();
    Reload();
    reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 5u, files.size() );

    file = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, file );
}

TEST_F( DeviceFs, ReplugDiskWithExtraFiles )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 5u, files.size() );

    auto device = fsMock->removeDevice( RemovableDeviceUuid );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    fsMock->addDevice( device );
    fsMock->addFile( RemovableDeviceMountpoint + "newfile.mkv" );

    cbMock->prepareForReload();
    Reload();
    reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 6u, files.size() );
}

TEST_F( DeviceFs, RemoveAlbum )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    // Create an album on a non-removable device
    {
        auto album = std::static_pointer_cast<Album>( ml->createAlbum( "album" ) );
        auto file = ml->file( mock::FileSystemFactory::Root + "audio.mp3" );
        album->addTrack( std::static_pointer_cast<Media>( file ), 1, 1 );
        auto artist = ml->createArtist( "artist" );
        album->setAlbumArtist( artist.get() );
    }
    // And an album that will disappear, along with its artist
    {
        auto album = std::static_pointer_cast<Album>( ml->createAlbum( "album 2" ) );
        auto file = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
        auto file2 = ml->file( RemovableDeviceMountpoint + "removablefile2.mp3" );
        album->addTrack( std::static_pointer_cast<Media>( file ), 1, 1 );
        album->addTrack( std::static_pointer_cast<Media>( file ), 2, 1 );
        auto artist = ml->createArtist( "artist 2" );
        album->setAlbumArtist( artist.get() );
    }

    auto albums = ml->albums();
    ASSERT_EQ( 2u, albums.size() );
    auto artists = ml->artists();
    ASSERT_EQ( 2u, artists.size() );

    fsMock->removeDevice( RemovableDeviceUuid );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    albums = ml->albums();
    ASSERT_EQ( 1u, albums.size() );
    artists = ml->artists();
    ASSERT_EQ( 1u, artists.size() );
}

TEST_F( DeviceFs, PartialAlbumRemoval )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    {
        auto album = std::static_pointer_cast<Album>( ml->createAlbum( "album" ) );
        auto file = ml->file( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
        auto file2 = ml->file( RemovableDeviceMountpoint + "removablefile2.mp3" );
        album->addTrack( std::static_pointer_cast<Media>( file ), 1, 1 );
        album->addTrack( std::static_pointer_cast<Media>( file2 ), 2, 1 );
        auto newArtist = ml->createArtist( "artist" );
        album->setAlbumArtist( newArtist.get() );
        newArtist->addMedia( static_cast<Media*>( file.get() ) );
        newArtist->addMedia( static_cast<Media*>( file2.get() ) );
    }

    auto albums = ml->albums();
    ASSERT_EQ( 1u, albums.size() );
    auto artists = ml->artists();
    ASSERT_EQ( 1u, artists.size() );
    auto artist = artists[0];
    ASSERT_EQ( 2u, artist->media().size() );

    fsMock->removeDevice( RemovableDeviceUuid );
    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    albums = ml->albums();
    ASSERT_EQ( 1u, albums.size() );
    artists = ml->artists();
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( 1u, albums[0]->tracks().size() );
    ASSERT_EQ( 1u, artists[0]->media().size() );
}

TEST_F( DeviceFs, ChangeDevice )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    // Fetch a removable file's ID
    auto f = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( f, nullptr );
    auto firstRemovableFileId = f->id();
    auto firstRemovableFilePath = f->mrl();

    // Remove & store the device
    auto oldRemovableDevice = fsMock->removeDevice( RemovableDeviceUuid );

    // Add a new device on the same mountpoint
    fsMock->addDevice( RemovableDeviceMountpoint, "{another-removable-device}" );
    fsMock->addFile( RemovableDeviceMountpoint + "removablefile.mp3" );

    cbMock->prepareForReload();
    Reload();
    auto reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    // Check that new files with the same name have different IDs
    // but the same "full path"
    f = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, f );
    ASSERT_EQ( firstRemovableFilePath, f->mrl() );
    ASSERT_NE( firstRemovableFileId, f->id() );

    fsMock->removeDevice( "{another-removable-device}" );
    fsMock->addDevice( oldRemovableDevice );

    cbMock->prepareForReload();
    Reload();
    reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    f = ml->file( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, f );
    ASSERT_EQ( firstRemovableFileId, f->id() );
}
