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

#include "medialibrary/IMediaLibrary.h"
#include "File.h"
#include "Media.h"
#include "Artist.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"
#include "compat/Thread.h"

class Medias : public Tests
{
};


TEST_F( Medias, Init )
{
    // only test for correct test fixture behavior
}

TEST_F( Medias, Create )
{
    auto m = ml->addFile( "media.avi" );
    ASSERT_NE( m, nullptr );

    ASSERT_EQ( m->playCount(), 0 );
    ASSERT_EQ( m->albumTrack(), nullptr );
    ASSERT_EQ( m->showEpisode(), nullptr );
    ASSERT_EQ( m->duration(), -1 );
    ASSERT_NE( 0u, m->insertionDate() );
}

TEST_F( Medias, Fetch )
{
    auto f = ml->addFile( "media.avi" );
    auto f2 = ml->media( f->id() );
    ASSERT_EQ( f->id(), f2->id() );
    ASSERT_EQ( f, f2 );

    // Flush cache and fetch from DB
    Reload();

    f2 = std::static_pointer_cast<Media>( ml->media( f->id() ) );
    ASSERT_EQ( f->id(), f2->id() );
}

TEST_F( Medias, Duration )
{
    auto f = ml->addFile( "media.avi" );
    ASSERT_EQ( f->duration(), -1 );

    // Use a value that checks we're using a 64bits value
    int64_t d = int64_t(1) << 40;

    f->setDuration( d );
    f->save();
    ASSERT_EQ( f->duration(), d );

    Reload();

    auto f2 = ml->media( f->id() );
    ASSERT_EQ( f2->duration(), d );
}

TEST_F( Medias, Thumbnail )
{
    auto f = ml->addFile( "media.avi" );
    ASSERT_EQ( f->thumbnail(), "" );

    std::string newThumbnail( "/path/to/thumbnail" );

    f->setThumbnail( newThumbnail );
    f->save();
    ASSERT_EQ( f->thumbnail(), newThumbnail );

    Reload();

    auto f2 = ml->media( f->id() );
    ASSERT_EQ( f2->thumbnail(), newThumbnail );
}

TEST_F( Medias, PlayCount )
{
    auto f = ml->addFile( "media.avi" );
    ASSERT_EQ( 0, f->playCount() );
    f->increasePlayCount();
    ASSERT_EQ( 1, f->playCount() );

    Reload();

    f = std::static_pointer_cast<Media>( ml->media( f->id() ) );
    ASSERT_EQ( 1, f->playCount() );
}

TEST_F( Medias, Progress )
{
    auto f = ml->addFile( "media.avi" );
    ASSERT_EQ( .0f, f->progress() );
    f->setProgress( 123.0f );
    // Check that a non-sensical value is ignored
    ASSERT_EQ( .0f, f->progress() );
    f->setProgress( 0.666f );
    ASSERT_EQ( .666f, f->progress() );

    Reload();

    f = ml->media( f->id() );
    ASSERT_EQ( .666f, f->progress() );
}

TEST_F( Medias, Rating )
{
    auto f = ml->addFile( "media.avi" );
    ASSERT_EQ( -1, f->rating() );
    f->setRating( 12345 );
    ASSERT_EQ( 12345, f->rating() );

    Reload();

    f = ml->media( f->id() );
    ASSERT_EQ( 12345, f->rating() );
}

TEST_F( Medias, Search )
{
    for ( auto i = 1u; i <= 10u; ++i )
    {
        auto m = ml->addFile( "track " + std::to_string( i ) + ".mp3" );
    }
    auto media = ml->searchMedia( "tra" ).others;
    ASSERT_EQ( 10u, media.size() );

    media = ml->searchMedia( "track 1" ).others;
    ASSERT_EQ( 2u, media.size() );

    media = ml->searchMedia( "grouik" ).others;
    ASSERT_EQ( 0u, media.size() );

    media = ml->searchMedia( "rack" ).others;
    ASSERT_EQ( 0u, media.size() );
}

TEST_F( Medias, SearchAfterEdit )
{
    auto m = ml->addFile( "media.mp3" );

    auto media = ml->searchMedia( "media" ).others;
    ASSERT_EQ( 1u, media.size() );

    m->setTitle( "otters are awesome" );
    m->save();

    media = ml->searchMedia( "media" ).others;
    ASSERT_EQ( 0u, media.size() );

    media = ml->searchMedia( "otters" ).others;
    ASSERT_EQ( 1u, media.size() );
}

TEST_F( Medias, SearchAfterDelete )
{
    auto m = ml->addFile( "media.mp3" );

    auto media = ml->searchMedia( "media" ).others;
    ASSERT_EQ( 1u, media.size() );

    auto f = m->files()[0];
    m->removeFile( static_cast<File&>( *f ) );

    media = ml->searchMedia( "media" ).others;
    ASSERT_EQ( 0u, media.size() );
}

TEST_F( Medias, SearchByLabel )
{
    auto m = ml->addFile( "media.mkv" );
    auto media = ml->searchMedia( "otter" ).others;
    ASSERT_EQ( 0u, media.size() );

    auto l = ml->createLabel( "otter" );
    m->addLabel( l );

    media = ml->searchMedia( "otter" ).others;
    ASSERT_EQ( 1u, media.size() );

    auto l2 = ml->createLabel( "pangolins" );
    m->addLabel( l2 );

    media = ml->searchMedia( "otter" ).others;
    ASSERT_EQ( 1u, media.size() );

    media = ml->searchMedia( "pangolin" ).others;
    ASSERT_EQ( 1u, media.size() );

    m->removeLabel( l );

    media = ml->searchMedia( "otter" ).others;
    ASSERT_EQ( 0u, media.size() );

    media = ml->searchMedia( "pangolin" ).others;
    ASSERT_EQ( 1u, media.size() );

    m->addLabel( l );

    media = ml->searchMedia( "otter" ).others;
    ASSERT_EQ( 1u, media.size() );

    media = ml->searchMedia( "pangolin" ).others;
    ASSERT_EQ( 1u, media.size() );

    ml->deleteLabel( l );

    media = ml->searchMedia( "otter" ).others;
    ASSERT_EQ( 0u, media.size() );

    media = ml->searchMedia( "pangolin" ).others;
    ASSERT_EQ( 1u, media.size() );
}

TEST_F( Medias, SearchTracks )
{
    auto a = ml->createAlbum( "album" );
    for ( auto i = 1u; i <= 10u; ++i )
    {
       auto m = ml->addFile( "track " + std::to_string( i ) + ".mp3" );
       a->addTrack( m, i, 1, 0, 0 );
    }
    auto tracks = ml->searchMedia( "tra" ).tracks;
    ASSERT_EQ( 10u, tracks.size() );

    tracks = ml->searchMedia( "track 1" ).tracks;
    ASSERT_EQ( 2u, tracks.size() );

    tracks = ml->searchMedia( "grouik" ).tracks;
    ASSERT_EQ( 0u, tracks.size() );

    tracks = ml->searchMedia( "rack" ).tracks;
    ASSERT_EQ( 0u, tracks.size() );
}

TEST_F( Medias, Favorite )
{
    auto m = ml->addFile( "media.mkv" );
    ASSERT_FALSE( m->isFavorite() );

    m->setFavorite( true );
    ASSERT_TRUE( m->isFavorite() );

    Reload();

    m = ml->media( m->id() );
    ASSERT_TRUE( m->isFavorite() );
}

TEST_F( Medias, History )
{
    auto m = ml->addFile( "media.mkv" );

    auto history = ml->lastMediaPlayed();
    ASSERT_EQ( 0u, history.size() );

    m->increasePlayCount();
    m->save();
    history = ml->lastMediaPlayed();
    ASSERT_EQ( 1u, history.size() );
    ASSERT_EQ( m->id(), history[0]->id() );

    compat::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    auto m2 = ml->addFile( "media.mkv" );
    m2->increasePlayCount();
    m2->save();

    history = ml->lastMediaPlayed();
    ASSERT_EQ( 2u, history.size() );
    ASSERT_EQ( m2->id(), history[0]->id() );
    ASSERT_EQ( m->id(), history[1]->id() );
}

TEST_F( Medias, ClearHistory )
{
    auto m = ml->addFile( "media.mkv" );

    auto history = ml->lastMediaPlayed();
    ASSERT_EQ( 0u, history.size() );

    m->increasePlayCount();
    m->save();
    history = ml->lastMediaPlayed();
    ASSERT_EQ( 1u, history.size() );

    ASSERT_TRUE( ml->clearHistory() );

    history = ml->lastMediaPlayed();
    ASSERT_EQ( 0u, history.size() );

    Reload();

    history = ml->lastMediaPlayed();
    ASSERT_EQ( 0u, history.size() );
}

TEST_F( Medias, SetReleaseDate )
{
    auto m = ml->addFile( "movie.mkv" );

    ASSERT_EQ( m->releaseDate(), 0u );
    m->setReleaseDate( 1234 );
    m->save();
    ASSERT_EQ( m->releaseDate(), 1234u );

    Reload();

    auto m2 = ml->media( m->id() );
    ASSERT_EQ( m2->releaseDate(), 1234u );
}

TEST_F( Medias, SortByAlpha )
{
    auto m1 = ml->addFile( "media1.mp3" );
    m1->setTitle( "Abcd" );
    m1->setType( Media::Type::Audio );
    m1->save();

    auto m2 = ml->addFile( "media2.mp3" );
    m2->setTitle( "Zyxw" );
    m2->setType( Media::Type::Audio );
    m2->save();

    auto m3 = ml->addFile( "media3.mp3" );
    m3->setTitle( "afterA-beforeZ" );
    m3->setType( Media::Type::Audio );
    m3->save();

    auto media = ml->audioFiles( SortingCriteria::Alpha, false );
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    ASSERT_EQ( m2->id(), media[2]->id() );

    media = ml->audioFiles( SortingCriteria::Alpha, true );
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[2]->id() );
}

TEST_F( Medias, SortByLastModifDate )
{
    auto file1 = std::make_shared<mock::NoopFile>( "media.mkv" );
    file1->setLastModificationDate( 666 );
    auto m1 = ml->addFile( *file1 );
    m1->setType( Media::Type::Video );
    m1->save();

    auto file2 = std::make_shared<mock::NoopFile>( "media2.mkv" );
    file2->setLastModificationDate( 111 );
    auto m2 = ml->addFile( *file2 );
    m2->setType( Media::Type::Video );
    m2->save();

    auto media = ml->videoFiles( SortingCriteria::LastModificationDate, false );
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );

    media = ml->videoFiles( SortingCriteria::LastModificationDate, true );
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

TEST_F( Medias, SortByFileSize )
{
    auto file1 = std::make_shared<mock::NoopFile>( "media.mkv" );
    file1->setSize( 666 );
    auto m1 = ml->addFile( *file1 );
    m1->setType( Media::Type::Video );
    m1->save();

    auto file2 = std::make_shared<mock::NoopFile>( "media2.mkv" );
    file2->setSize( 111 );
    auto m2 = ml->addFile( *file2 );
    m2->setType( Media::Type::Video );
    m2->save();

    auto media = ml->videoFiles( SortingCriteria::FileSize, false );
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );

    media = ml->videoFiles( SortingCriteria::FileSize, true );
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

TEST_F( Medias, SetType )
{
    auto m1 = ml->addFile( "media1.mp3" );
    ASSERT_EQ( IMedia::Type::Unknown, m1->type() );

    m1->setType( IMedia::Type::Video );
    m1->save();

    ASSERT_EQ( IMedia::Type::Video, m1->type() );

    Reload();

    auto m2 = ml->media( m1->id() );
    ASSERT_EQ( IMedia::Type::Video, m2->type() );
}

class FetchMedia : public Tests
{
protected:
    static const std::string RemovableDeviceUuid;
    static const std::string RemovableDeviceMountpoint;
    std::shared_ptr<mock::FileSystemFactory> fsMock;
    std::unique_ptr<mock::WaitForDiscoveryComplete> cbMock;

    virtual void SetUp() override
    {
        unlink( "test.db" );
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        fsMock->addFolder( "/a/mnt/" );
        auto device = fsMock->addDevice( RemovableDeviceMountpoint, RemovableDeviceUuid );
        device->setRemovable( true );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile.mp3" );
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

const std::string FetchMedia::RemovableDeviceUuid = "{fake-removable-device}";
const std::string FetchMedia::RemovableDeviceMountpoint = "/a/mnt/fake-device/";

TEST_F( FetchMedia, FetchNonRemovable )
{
    cbMock->prepareForWait();
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto m = ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    ASSERT_NE( nullptr, m );
}

TEST_F( FetchMedia, FetchRemovable )
{
    cbMock->prepareForWait();
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto m = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, m );
}

TEST_F( FetchMedia, FetchRemovableUnplugged )
{
    cbMock->prepareForWait();
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    fsMock->unmountDevice( RemovableDeviceUuid );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    auto m = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, m );
}
