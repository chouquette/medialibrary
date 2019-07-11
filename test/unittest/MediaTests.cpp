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
#include "Playlist.h"

class Medias : public Tests
{
};


TEST_F( Medias, Init )
{
    // only test for correct test fixture behavior
}

TEST_F( Medias, Create )
{
    auto m = ml->addFile( "media.avi", IMedia::Type::Video );
    ASSERT_NE( m, nullptr );

    ASSERT_EQ( 0u, m->playCount() );
    ASSERT_EQ( m->albumTrack(), nullptr );
    ASSERT_EQ( m->showEpisode(), nullptr );
    ASSERT_EQ( m->duration(), -1 );
    ASSERT_NE( 0u, m->insertionDate() );

    auto files = m->files();
    ASSERT_EQ( 1u, files.size() );
    auto f = files[0];
    ASSERT_FALSE( f->isExternal() );
    ASSERT_EQ( File::Type::Main, f->type() );
}

TEST_F( Medias, Fetch )
{
    auto f = ml->addMedia( "media.avi" );
    auto f2 = ml->media( f->id() );
    ASSERT_EQ( f->id(), f2->id() );

    // Flush cache and fetch from DB
    Reload();

    f2 = std::static_pointer_cast<Media>( ml->media( f->id() ) );
    ASSERT_EQ( f->id(), f2->id() );
}

TEST_F( Medias, Duration )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "media.avi" ) );
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
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "media.avi" ) );
    ASSERT_EQ( f->thumbnailMrl( ThumbnailSizeType::Thumbnail ), "" );

    std::string newThumbnail( "file:///path/to/thumbnail" );

    f->setThumbnail( newThumbnail, ThumbnailSizeType::Thumbnail );
    f->save();
    ASSERT_EQ( f->thumbnailMrl( ThumbnailSizeType::Thumbnail ), newThumbnail );

    Reload();

    auto f2 = ml->media( f->id() );
    ASSERT_EQ( f2->thumbnailMrl( ThumbnailSizeType::Thumbnail ), newThumbnail );
}

TEST_F( Medias, PlayCount )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "media.avi" ) );
    ASSERT_EQ( 0u, f->playCount() );
    f->increasePlayCount();
    ASSERT_EQ( 1u, f->playCount() );

    Reload();

    f = std::static_pointer_cast<Media>( ml->media( f->id() ) );
    ASSERT_EQ( 1u, f->playCount() );
}

TEST_F( Medias, Progress )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "media.avi" ) );
    ASSERT_EQ( 0, f->metadata( Media::MetadataType::Progress ).integer() );
    f->setMetadata( Media::MetadataType::Progress, 123 );
    ASSERT_EQ( 123, f->metadata( Media::MetadataType::Progress ).integer() );
    ASSERT_TRUE( f->metadata( Media::MetadataType::Progress ).isSet() );

    Reload();

    f = ml->media( f->id() );
    ASSERT_EQ( 123, f->metadata( Media::MetadataType::Progress ).integer() );
}

TEST_F( Medias, Rating )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "media.avi" ) );
    ASSERT_FALSE( f->metadata( Media::MetadataType::Rating ).isSet() );
    f->setMetadata( Media::MetadataType::Rating, 12345 );
    ASSERT_EQ( 12345, f->metadata( Media::MetadataType::Rating ).integer() );
    ASSERT_TRUE( f->metadata( Media::MetadataType::Rating ).isSet() );

    Reload();

    f = ml->media( f->id() );
    ASSERT_EQ( 12345, f->metadata( Media::MetadataType::Rating ).integer() );
}

TEST_F( Medias, Search )
{
    for ( auto i = 1u; i <= 10u; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    ml->addMedia( "track " + std::to_string( i ) + ".mp3", IMedia::Type::Video ) );
        m->save();
    }
    auto media = ml->searchMedia( "tra", nullptr )->all();
    ASSERT_EQ( 10u, media.size() );

    media = ml->searchMedia( "track 1", nullptr )->all();
    ASSERT_EQ( 2u, media.size() );

    media = ml->searchMedia( "grouik", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    media = ml->searchMedia( "rack", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
}

TEST_F( Medias, SearchAndSort )
{
    for ( auto i = 1u; i <= 3u; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    ml->addMedia( "track " + std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        m->setDuration( 3 - i );
        m->save();
    }
    auto m = std::static_pointer_cast<Media>( ml->addMedia(
                    "this pattern doesn't match.mp3", IMedia::Type::Audio ) );

    auto media = ml->searchMedia( "tra", nullptr )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( media[0]->title(), "track 1.mp3" );
    ASSERT_EQ( media[1]->title(), "track 2.mp3" );
    ASSERT_EQ( media[2]->title(), "track 3.mp3" );

    QueryParameters params { SortingCriteria::Duration, false };
    media = ml->searchMedia( "tra", &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( media[0]->title(), "track 3.mp3" );
    ASSERT_EQ( media[1]->title(), "track 2.mp3" );
    ASSERT_EQ( media[2]->title(), "track 1.mp3" );
}

TEST_F( Medias, SearchAfterEdit )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );

    auto media = ml->searchMedia( "media", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    m->setTitleBuffered( "otters are awesome" );
    m->save();

    media = ml->searchMedia( "media", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    media = ml->searchMedia( "otters", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
}

TEST_F( Medias, SearchAfterDelete )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );

    auto media = ml->searchMedia( "media", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    auto f = m->files()[0];
    m->removeFile( static_cast<File&>( *f ) );

    media = ml->searchMedia( "media", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
}

TEST_F( Medias, SearchByLabel )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    auto media = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    auto l = ml->createLabel( "otter" );
    m->addLabel( l );

    media = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    auto l2 = ml->createLabel( "pangolins" );
    m->addLabel( l2 );

    media = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    media = ml->searchMedia( "pangolin", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    m->removeLabel( l );

    media = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    media = ml->searchMedia( "pangolin", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    m->addLabel( l );

    media = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    media = ml->searchMedia( "pangolin", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    ml->deleteLabel( l );

    media = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    media = ml->searchMedia( "pangolin", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
}

TEST_F( Medias, SearchTracks )
{
    auto a = ml->createAlbum( "album" );
    for ( auto i = 1u; i <= 10u; ++i )
    {
       auto m = std::static_pointer_cast<Media>( ml->addMedia( "track " +
                        std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
       a->addTrack( m, i, 1, 0, 0 );
       m->save();
    }
    auto tracks = ml->searchMedia( "tra", nullptr )->all();
    ASSERT_EQ( 10u, tracks.size() );

    tracks = ml->searchMedia( "track 1", nullptr )->all();
    ASSERT_EQ( 2u, tracks.size() );

    tracks = ml->searchMedia( "grouik", nullptr )->all();
    ASSERT_EQ( 0u, tracks.size() );

    tracks = ml->searchMedia( "rack", nullptr )->all();
    ASSERT_EQ( 0u, tracks.size() );
}

TEST_F( Medias, Favorite )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    ASSERT_FALSE( m->isFavorite() );

    m->setFavorite( true );
    ASSERT_TRUE( m->isFavorite() );

    Reload();

    m = ml->media( m->id() );
    ASSERT_TRUE( m->isFavorite() );
}

TEST_F( Medias, History )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv" ) );

    auto history = ml->history()->all();
    ASSERT_EQ( 0u, history.size() );

    m->increasePlayCount();
    m->save();
    history = ml->history()->all();
    ASSERT_EQ( 1u, history.size() );
    ASSERT_EQ( m->id(), history[0]->id() );

    compat::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mkv" ) );
    m2->increasePlayCount();

    history = ml->history()->all();
    ASSERT_EQ( 2u, history.size() );
    ASSERT_EQ( m2->id(), history[0]->id() );
    ASSERT_EQ( m->id(), history[1]->id() );
}

TEST_F( Medias, StreamHistory )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addStream( "http://media.org/sample.mkv" ) );
    auto m2 = std::static_pointer_cast<Media>( ml->addStream( "http://media.org/sample2.mkv" ) );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "localfile.mkv" ) );

    m1->increasePlayCount();
    m2->increasePlayCount();
    m3->increasePlayCount();

    auto history = ml->streamHistory()->all();
    ASSERT_EQ( 2u, history.size() );

    history = ml->history()->all();
    ASSERT_EQ( 1u, history.size() );
}

TEST_F( Medias, HistoryByType )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "video.mkv" ) );
    m1->setType( IMedia::Type::Video );
    m1->increasePlayCount();
    m1->save();

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "audio.mp3" ) );
    m2->setType( IMedia::Type::Audio);
    m2->save();
    m2->increasePlayCount();

    auto h = ml->history( IMedia::Type::Audio )->all();
    ASSERT_EQ( 1u, h.size() );

    h = ml->history( IMedia::Type::Video)->all();
    ASSERT_EQ( 1u, h.size() );

    h = ml->history()->all();
    ASSERT_EQ( 2u, h.size() );
}

TEST_F( Medias, ClearHistory )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv" ) );

    auto history = ml->history()->all();
    ASSERT_EQ( 0u, history.size() );

    m->increasePlayCount();
    m->save();
    history = ml->history()->all();
    ASSERT_EQ( 1u, history.size() );

    ASSERT_TRUE( ml->clearHistory() );

    history = ml->history()->all();
    ASSERT_EQ( 0u, history.size() );

    Reload();

    history = ml->history()->all();
    ASSERT_EQ( 0u, history.size() );
}

TEST_F( Medias, RemoveFromHistory )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv" ) );

    auto history = ml->history()->all();
    ASSERT_EQ( 0u, history.size() );

    m->increasePlayCount();
    m->save();
    m->setMetadata( IMedia::MetadataType::Progress, "50" );
    history = ml->history()->all();
    ASSERT_EQ( 1u, history.size() );
    ASSERT_EQ( m->id(), history[0]->id() );
    ASSERT_EQ( 1u, m->playCount() );
    ASSERT_TRUE( m->metadata( IMedia::MetadataType::Progress ).isSet() );
    ASSERT_EQ( m->metadata( IMedia::MetadataType::Progress ).str(), "50" );

    m->removeFromHistory();

    history = ml->history()->all();
    ASSERT_EQ( 0u, history.size() );
    ASSERT_EQ( 0u, m->playCount() );
    ASSERT_FALSE( m->metadata( IMedia::MetadataType::Progress ).isSet() );
}

TEST_F( Medias, SetReleaseDate )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "movie.mkv" ) );

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
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "media1.mp3", Media::Type::Audio ) );
    m1->setTitleBuffered( "Abcd" );
    m1->save();

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mp3", Media::Type::Audio ) );
    m2->setTitleBuffered( "Zyxw" );
    m2->save();

    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "media3.mp3", Media::Type::Audio) );
    m3->setTitleBuffered( "afterA-beforeZ" );
    m3->save();

    QueryParameters params { SortingCriteria::Alpha, false };
    auto media = ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    ASSERT_EQ( m2->id(), media[2]->id() );

    params.desc = true;
    media = ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[2]->id() );
}

TEST_F( Medias, SortByLastModifDate )
{
    auto file1 = std::make_shared<mock::NoopFile>( "media.mkv" );
    file1->setLastModificationDate( 666 );
    auto m1 = ml->addFile( file1, Media::Type::Video );

    auto file2 = std::make_shared<mock::NoopFile>( "media2.mkv" );
    file2->setLastModificationDate( 111 );
    auto m2 = ml->addFile( file2, Media::Type::Video );

    QueryParameters params { SortingCriteria::LastModificationDate, false };
    auto media = ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );

    params.desc = true;
    media = ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

TEST_F( Medias, SortByFileSize )
{
    auto file1 = std::make_shared<mock::NoopFile>( "media.mkv" );
    file1->setSize( 666 );
    auto m1 = ml->addFile( file1, Media::Type::Video );

    auto file2 = std::make_shared<mock::NoopFile>( "media2.mkv" );
    file2->setSize( 111 );
    auto m2 = ml->addFile( file2, Media::Type::Video );

    QueryParameters params { SortingCriteria::FileSize, false };
    auto media = ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );

    params.desc = true;
    media = ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

TEST_F( Medias, SortByFilename )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "zzzzz.mp3", Media::Type::Video ) );
    m1->setTitle( "aaaaa" );

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "aaaaa.mp3", Media::Type::Video ) );
    m2->setTitle( "zzzzz" );

    QueryParameters params { SortingCriteria::Filename, false };
    auto media = ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );

    params.desc = true;
    media = ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

TEST_F( Medias, SetType )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "media1.mp3" ) );
    ASSERT_EQ( IMedia::Type::External, m1->type() );

    m1->setType( IMedia::Type::Video );
    m1->save();

    ASSERT_EQ( IMedia::Type::Video, m1->type() );

    Reload();

    auto m2 = ml->media( m1->id() );
    ASSERT_EQ( IMedia::Type::Video, m2->type() );
}

TEST_F( Medias, SetSubType )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "media1.mp3" ) );
    ASSERT_EQ( IMedia::SubType::Unknown, m1->subType() );

    m1->setSubType( IMedia::SubType::Movie );
    m1->save();

    ASSERT_EQ( IMedia::SubType::Movie, m1->subType() );

    Reload();

    auto m2 = ml->media( m1->id() );
    ASSERT_EQ( IMedia::SubType::Movie, m1->subType() );
}

TEST_F( Medias, Metadata )
{
    auto m = ml->addMedia( "media.mp3" );

    {
        const auto& md = m->metadata( Media::MetadataType::Speed );
        ASSERT_FALSE( md.isSet() );
    }

    auto res = m->setMetadata( Media::MetadataType::Speed, "foo" );
    ASSERT_TRUE( res );

    {
        const auto& md = m->metadata( Media::MetadataType::Speed );
        ASSERT_EQ( "foo", md.str() );
    }

    Reload();

    m = ml->media( m->id() );
    const auto& md = m->metadata( Media::MetadataType::Speed );
    ASSERT_EQ( "foo", md.str() );
}

TEST_F( Medias, MetadataOverride )
{
    auto m = ml->addMedia( "media.mp3" );
    auto res = m->setMetadata( Media::MetadataType::Speed, "foo" );
    ASSERT_TRUE( res );

    m->setMetadata( Media::MetadataType::Speed, "otter" );
    {
        const auto& md = m->metadata( Media::MetadataType::Speed );
        ASSERT_EQ( "otter", md.str() );
    }

    Reload();

    m = ml->media( m->id() );
    const auto& md = m->metadata( Media::MetadataType::Speed );
    ASSERT_EQ( "otter", md.str() );
}

TEST_F( Medias, MetadataUnset )
{
    auto m = ml->addMedia( "media.mp3" );
    auto res = m->unsetMetadata( Media::MetadataType::ApplicationSpecific );
    ASSERT_TRUE( res );

    res = m->setMetadata( Media::MetadataType::ApplicationSpecific, "otters" );
    ASSERT_TRUE( res );

    auto& md = m->metadata( Media::MetadataType::ApplicationSpecific );
    ASSERT_TRUE( md.isSet() );
    ASSERT_EQ( "otters", md.str() );

    res = m->unsetMetadata( Media::MetadataType::ApplicationSpecific );
    ASSERT_TRUE( res );

    ASSERT_FALSE( md.isSet() );

    Reload();

    m = ml->media( m->id() );
    auto& md2 = m->metadata( Media::MetadataType::ApplicationSpecific );
    ASSERT_FALSE( md2.isSet() );
}

TEST_F( Medias, ExternalMrl )
{
    auto m = ml->addExternalMedia( "https://foo.bar/sea-otters.mkv" );
    ASSERT_NE( nullptr, m );

    ASSERT_EQ( m->title(), "sea-otters.mkv" );

    // External files shouldn't appear in listings
    auto videos = ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 0u, videos.size() );

    auto audios = ml->audioFiles( nullptr )->all();
    ASSERT_EQ( 0u, audios.size() );

    Reload();

    auto m2 = ml->media( "https://foo.bar/sea-otters.mkv" );
    ASSERT_NE( nullptr, m2 );
    ASSERT_EQ( m->id(), m2->id() );

    auto files = m2->files();
    ASSERT_EQ( 1u, files.size() );
    auto f = files[0];
    ASSERT_TRUE( f->isExternal() );
    ASSERT_EQ( File::Type::Main, f->type() );
}

TEST_F( Medias, DuplicatedExternalMrl )
{
    auto m = ml->addExternalMedia( "http://foo.bar" );
    auto m2 = ml->addExternalMedia( "http://foo.bar" );
    ASSERT_NE( nullptr, m );
    ASSERT_EQ( nullptr, m2 );
}

TEST_F( Medias, SetTitle )
{
    auto m = ml->addMedia( "media" );
    ASSERT_EQ( "media", m->title() );
    auto res = m->setTitle( "sea otters" );
    ASSERT_TRUE( res );
    ASSERT_EQ( "sea otters", m->title() );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( "sea otters", m->title() );
}

TEST_F( Medias, Pagination )
{
    for ( auto i = 1u; i <= 9u; ++i )
    {
       auto m = std::static_pointer_cast<Media>( ml->addMedia( "track " +
                        std::to_string( i ) + ".mp3", IMedia::Type::Video ) );
    }

    auto allMedia = ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 9u, allMedia.size() );

    auto paginator = ml->videoFiles( nullptr );
    auto media = paginator->items( 1, 0 );
    int i = 0u;
    while( media.size() > 0 )
    {
        // Offset starts from 0; ids from 1
        ASSERT_EQ( 1u, media.size() );
        ASSERT_EQ( i + 1, media[0]->id() );
        ++i;
        media = paginator->items( 1, i );
    }
}

TEST_F( Medias, SortFilename )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "AAAAB.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "aaaaa.mp3", IMedia::Type::Audio ) );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "BbBbB.mp3", IMedia::Type::Audio ) );

    QueryParameters params { SortingCriteria::Filename, false };
    auto media = ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );

    params.desc = true;
    media = ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m2->id(), media[2]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[0]->id() );
}

TEST_F( Medias, CreateStream )
{
    auto m1 = ml->addStream( "http://foo.bar/media.mkv" );
    ASSERT_EQ( IMedia::Type::Stream, m1->type() );
}

TEST_F( Medias, SearchExternal )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addExternalMedia( "localfile.mkv" ) );
    m1->setTitle( "local otter" );
    auto m2 = std::static_pointer_cast<Media>( ml->addStream( "http://remote.file/media.asf" ) );
    m2->setTitle( "remote otter" );

    auto media = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    ml->setMediaType( m1->id(), IMedia::Type::Video );
    ml->setMediaType( m2->id(), IMedia::Type::Video );

    media = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 2u, media.size() );
}

TEST_F( Medias, VacuumOldExternal )
{
    auto m1 = ml->addExternalMedia( "foo.avi" );
    auto m2 = ml->addExternalMedia( "bar.mp3" );
    auto s1 = ml->addStream( "http://baz.mkv" );

    ASSERT_NE( nullptr, m1 );
    ASSERT_NE( nullptr, m2 );
    ASSERT_NE( nullptr, s1 );

    // Check that they will not be vacuumed even if they haven't been played yet.

    Reload();

    m1 = ml->media( m1->id() );
    m2 = ml->media( m2->id() );
    s1 = ml->media( s1->id() );

    ASSERT_NE( nullptr, m1 );
    ASSERT_NE( nullptr, m2 );
    ASSERT_NE( nullptr, s1 );

    auto playlist = ml->createPlaylist( "playlist" );
    playlist->append( *m1 );

    ml->outdateAllExternalMedia();

    Reload();

    m1 = ml->media( m1->id() );
    m2 = ml->media( m2->id() );
    s1 = ml->media( s1->id() );

    ASSERT_NE( nullptr, m1 );
    ASSERT_EQ( nullptr, m2 );
    ASSERT_EQ( nullptr, s1 );
}

TEST_F( Medias, VacuumNeverPlayedMedia )
{
    auto m1 = ml->addExternalMedia( "foo.avi" );
    auto m2 = ml->addExternalMedia( "bar.mp3" );
    auto s1 = ml->addStream( "http://baz.mkv" );

    ASSERT_NE( nullptr, m1 );
    ASSERT_NE( nullptr, m2 );
    ASSERT_NE( nullptr, s1 );

    ml->setMediaInsertionDate( m1->id(), 1 );

    Reload();

    m1 = ml->media( m1->id() );
    m2 = ml->media( m2->id() );
    s1 = ml->media( s1->id() );

    ASSERT_EQ( nullptr, m1 );
    ASSERT_NE( nullptr, m2 );
    ASSERT_NE( nullptr, s1 );
}

TEST_F( Medias, RemoveExternal )
{
    auto m = ml->addExternalMedia( "http://extern.al/media.mkv" );
    ASSERT_NE( nullptr, m );

    auto res = ml->removeExternalMedia( m );
    ASSERT_TRUE( res );

    m = ml->media( m->id() );
    ASSERT_EQ( nullptr, m );
    m = ml->media( "http://extern.al/media.mkv" );
    ASSERT_EQ( nullptr, m );
}

TEST_F( Medias, NbPlaylists )
{
    auto m = std::static_pointer_cast<Media>( ml->addExternalMedia( "media.mkv" ) );
    ASSERT_EQ( 0u, m->nbPlaylists() );

    auto playlist = ml->createPlaylist( "playlisẗ" );
    auto res = playlist->append( *m );
    ASSERT_TRUE( res );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( 1u, m->nbPlaylists() );

    playlist = ml->playlist( playlist->id() );
    playlist->remove( 0 );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( 0u, m->nbPlaylists() );

    playlist = ml->playlist( playlist->id() );
    playlist->append( *m );
    playlist->append( *m );

    Reload();

    m = ml->media( m->id() );
    playlist = ml->playlist( playlist->id() );

    ASSERT_EQ( 2u, m->nbPlaylists() );

    playlist->remove( 0 );

    Reload();

    m = ml->media( m->id() );
    // The media was inserted twice in the playlist and should therefor still have
    // one entry there, causing the number of playlist to be 1
    ASSERT_EQ( 1u, m->nbPlaylists() );

    ml->deletePlaylist( playlist->id() );

    Reload();
    m = ml->media( m->id() );
    ASSERT_EQ( 0u, m->nbPlaylists() );
}

TEST_F( Medias, SortByAlbum )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );

    // Create the albums in reversed alphabetical order to ensure id & alpha orders
    // are different
    auto album1 = ml->createAlbum( "Ziltoid ");
    auto album2 = ml->createAlbum( "Addicted" );

    album1->addTrack( m2, 1, 0, 0, nullptr );
    album1->addTrack( m1, 2, 0, 0, nullptr );
    album2->addTrack( m3, 1, 0, 0, nullptr );

    // Album1: [m2; m1]
    // Album2: [m3]

    m1->save();
    m2->save();
    m3->save();

    auto queryParams = QueryParameters{};
    queryParams.desc = false;
    queryParams.sort = SortingCriteria::Album;
    auto tracks = ml->audioFiles( &queryParams )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( m3->id(), tracks[0]->id() );
    ASSERT_EQ( m2->id(), tracks[1]->id() );
    ASSERT_EQ( m1->id(), tracks[2]->id() );

    queryParams.desc = true;
    tracks = ml->audioFiles( &queryParams )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( m2->id(), tracks[0]->id() );
    ASSERT_EQ( m1->id(), tracks[1]->id() );
    ASSERT_EQ( m3->id(), tracks[2]->id() );
}

TEST_F( Medias, SetFilename )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv" ) );
    ASSERT_EQ( "media.mkv", m->fileName() );

    m->setFileName( "sea_otter.asf" );
    ASSERT_EQ( "sea_otter.asf", m->fileName() );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( "sea_otter.asf", m->fileName() );
}

TEST_F( Medias, SetPlayCount )
{
    auto m = ml->addMedia( "media.avi" );
    ASSERT_EQ( 0u, m->playCount() );
    auto res = m->setPlayCount( 123 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 123u, m->playCount() );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( 123u, m->playCount() );
}

TEST_F( Medias, SetDeviceId )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv" ) );
    ASSERT_EQ( 0u, m->deviceId() );

    m->setDeviceId( 123u );
    ASSERT_EQ( 123u, m->deviceId() );
    m->save();

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( 123u, m->deviceId() );
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
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        fsMock->addFolder( "file:///a/mnt/" );
        auto device = fsMock->addDevice( RemovableDeviceMountpoint, RemovableDeviceUuid );
        device->setRemovable( true );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile.mp3" );
        fsFactory = fsMock;
        mlCb = cbMock.get();
        Tests::SetUp();
    }

    virtual void InstantiateMediaLibrary() override
    {
        ml.reset( new MediaLibraryWithDiscoverer );
    }
};

const std::string FetchMedia::RemovableDeviceUuid = "{fake-removable-device}";
const std::string FetchMedia::RemovableDeviceMountpoint = "file:///a/mnt/fake-device/";

TEST_F( FetchMedia, FetchNonRemovable )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto m = ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    ASSERT_NE( nullptr, m );
}

TEST_F( FetchMedia, FetchRemovable )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto m = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, m );
}

TEST_F( FetchMedia, FetchRemovableUnplugged )
{
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    fsMock->unmountDevice( RemovableDeviceUuid );

    Reload();
    bool reloaded = cbMock->waitReload();
    ASSERT_TRUE( reloaded );

    auto m = ml->media( RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, m );
}
