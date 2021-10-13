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

#include "mocks/FileSystem.h"

#include "medialibrary/IMediaLibrary.h"
#include "File.h"
#include "Media.h"
#include "Artist.h"
#include "Album.h"

#include "compat/Thread.h"
#include "Playlist.h"
#include "Device.h"

static void Create( Tests* T )
{
    auto m = T->ml->addFile( "media.avi", IMedia::Type::Video );
    ASSERT_NE( m, nullptr );

    ASSERT_EQ( 0u, m->playCount() );
    ASSERT_EQ( m->showEpisode(), nullptr );
    ASSERT_EQ( m->duration(), -1 );
    ASSERT_EQ( -1.f, m->lastPosition() );
    ASSERT_EQ( -1, m->lastTime() );
    ASSERT_NE( 0u, m->insertionDate() );
    ASSERT_TRUE( m->isDiscoveredMedia() );
    ASSERT_TRUE( m->isPresent() );

    auto files = m->files();
    ASSERT_EQ( 1u, files.size() );
    auto f = files[0];
    ASSERT_FALSE( f->isExternal() );
    ASSERT_EQ( File::Type::Main, f->type() );
}

static void Fetch( Tests* T )
{
    auto f = T->ml->addMedia( "media.avi", IMedia::Type::Video );
    auto f2 = T->ml->media( f->id() );
    ASSERT_EQ( f->id(), f2->id() );

    f2 = std::static_pointer_cast<Media>( T->ml->media( f->id() ) );
    ASSERT_EQ( f->id(), f2->id() );
}

static void Duration( Tests* T )
{
    auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "media.avi", IMedia::Type::Video ) );
    ASSERT_EQ( f->duration(), -1 );

    // Use a value that checks we're using a 64bits value
    int64_t d = int64_t(1) << 40;

    f->setDuration( d );
    ASSERT_EQ( f->duration(), d );

    auto f2 = T->ml->media( f->id() );
    ASSERT_EQ( f2->duration(), d );
}

static void GetThumbnail( Tests* T )
{
    auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "media.avi", IMedia::Type::Video ) );
    ASSERT_EQ( f->thumbnailMrl( ThumbnailSizeType::Thumbnail ), "" );

    std::string newThumbnail( "file:///path/to/thumbnail" );

    f->setThumbnail( newThumbnail, ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( f->thumbnailMrl( ThumbnailSizeType::Thumbnail ), newThumbnail );

    auto f2 = T->ml->media( f->id() );
    ASSERT_EQ( f2->thumbnailMrl( ThumbnailSizeType::Thumbnail ), newThumbnail );
}

static void SetProgress( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    m1->setDuration( 60 * 30 * 1000 ); // 30min
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    m2->setDuration( 5 * 3600 * 1000 ); // 5h

    ASSERT_EQ( -1.f, m1->lastPosition() );
    ASSERT_EQ( -1.f, m2->lastPosition() );
    ASSERT_EQ( -1, m1->lastTime() );
    ASSERT_EQ( -1, m2->lastTime() );

    /*
     * Test a duration in the middle of the media. This should just bump the
     * duration as such
     */
    auto expectedPosition = 0.5f;
    auto res = m1->setLastPosition( expectedPosition );
    ASSERT_EQ( IMedia::ProgressResult::AsIs, res );
    ASSERT_EQ( expectedPosition, m1->lastPosition() );
    ASSERT_EQ( m1->duration() / 2, m1->lastTime() );
    ASSERT_EQ( 0u, m1->playCount() );
    m1 = T->ml->media( m1->id() );
    ASSERT_EQ( expectedPosition, m1->lastPosition() );
    ASSERT_EQ( m1->duration() / 2, m1->lastTime() );
    ASSERT_EQ( 0u, m1->playCount() );

    /* Then update to a progress to 3%. This should reset it to -1 */

    res = m1->setLastTime( 54000 ); //3% of 30 minutes in milliseconds
    ASSERT_EQ( IMedia::ProgressResult::Begin, res );
    ASSERT_EQ( -1, m1->lastTime() );
    ASSERT_EQ( -1.f, m1->lastPosition() );
    ASSERT_EQ( 0u, m1->playCount() );
    m1 = T->ml->media( m1->id() );
    ASSERT_EQ( -1, m1->lastTime() );
    ASSERT_EQ( -1.f, m1->lastPosition() );
    ASSERT_EQ( 0u, m1->playCount() );

    /* Then again at 4% and ensure the progress is still -1 */
    res = m1->setLastPosition( 0.04 );
    ASSERT_EQ( IMedia::ProgressResult::Begin, res );
    ASSERT_EQ( -1, m1->lastTime() );
    ASSERT_EQ( -1.f, m1->lastPosition() );
    m1 = T->ml->media( m1->id() );
    ASSERT_EQ( -1, m1->lastTime() );
    ASSERT_EQ( -1.f, m1->lastPosition() );

    /*
     * Now set a progress of 99% and check the playcount was bumped, and
     * progress reset
     */
    res = m1->setLastTime( 1782000 );
    ASSERT_EQ( IMedia::ProgressResult::End, res );
    ASSERT_EQ( -1, m1->lastTime() );
    ASSERT_EQ( -1.f, m1->lastPosition() );
    ASSERT_EQ( 1u, m1->playCount() );
    m1 = T->ml->media( m1->id() );
    ASSERT_EQ( -1, m1->lastTime() );
    ASSERT_EQ( -1.f, m1->lastPosition() );
    ASSERT_EQ( 1u, m1->playCount() );

    /* Now do the same with a longer media to ensure the "margin" are updated */
    expectedPosition = 0.5f;
    res = m2->setLastPosition( expectedPosition );
    ASSERT_EQ( IMedia::ProgressResult::AsIs, res );
    ASSERT_EQ( expectedPosition, m2->lastPosition() );
    ASSERT_EQ( m2->duration() / 2, m2->lastTime() );
    ASSERT_EQ( 0u, m2->playCount() );
    m2 = T->ml->media( m2->id() );
    ASSERT_EQ( expectedPosition, m2->lastPosition() );
    ASSERT_EQ( m2->duration() / 2, m2->lastTime() );
    ASSERT_EQ( 0u, m2->playCount() );

    /* This media should only ignore the first & last percent */
    res = m2->setLastPosition( 0.009f );
    ASSERT_EQ( IMedia::ProgressResult::Begin, res );
    ASSERT_EQ( -1.f, m2->lastPosition() );
    ASSERT_EQ( -1, m2->lastTime() );
    ASSERT_EQ( 0u, m2->playCount() );
    m2 = T->ml->media( m2->id() );
    ASSERT_EQ( -1.f, m2->lastPosition() );
    ASSERT_EQ( -1, m2->lastTime() );
    ASSERT_EQ( 0u, m2->playCount() );

    /* So check 0.01 is not ignored */
    expectedPosition = 0.01f;
    res = m2->setLastPosition( expectedPosition );
    ASSERT_EQ( IMedia::ProgressResult::AsIs, res );
    ASSERT_EQ( expectedPosition, m2->lastPosition() );
    /* Don't fail the tests because of a rounding error */
    ASSERT_TRUE( m2->lastTime() >= 180000 - 1 &&
                 m2->lastTime() <= 180000 );
    ASSERT_EQ( 0u, m2->playCount() );
    m2 = T->ml->media( m2->id() );
    ASSERT_EQ( expectedPosition, m2->lastPosition() );
    ASSERT_TRUE( m2->lastTime() >= 180000 - 1 &&
                 m2->lastTime() <= 180000 );
    ASSERT_EQ( 0u, m2->playCount() );

    /*
     * And finally check that 0.98 is just a regular progress and not the end of
     * the media since it's 5hours long
     */
    expectedPosition = 0.98;
    res = m2->setLastPosition( expectedPosition );
    ASSERT_EQ( IMedia::ProgressResult::AsIs, res );
    ASSERT_EQ( expectedPosition, m2->lastPosition() );
    ASSERT_TRUE( m2->lastTime() >= 17640000 - 1 &&
                 m2->lastTime() <= 17640000 );
    ASSERT_EQ( 0u, m2->playCount() );
    m2 = T->ml->media( m2->id() );
    ASSERT_EQ( expectedPosition, m2->lastPosition() );
    ASSERT_TRUE( m2->lastTime() >= 17640000 - 1 &&
                 m2->lastTime() <= 17640000 );
    ASSERT_EQ( 0u, m2->playCount() );
}

static void SetLastPositionNoDuration( Tests* T )
{
    auto m = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    /* Check that we accept any value when no duration is provided */
    auto res = m->setLastPosition( 123.456f );
    ASSERT_EQ( IMedia::ProgressResult::AsIs, res );
    ASSERT_EQ( 123.456f, m->lastPosition() );
    /* And ensure we didn't infer the last_time value since we can't compute it */
    ASSERT_EQ( -1, m->lastTime() );
    m = T->ml->media( m->id() );
    ASSERT_EQ( 123.456f, m->lastPosition() );
    ASSERT_EQ( -1, m->lastTime() );

    /*
     * And check the other way around: if we set a time we don't get a
     * position back
     */
    res = m->setLastTime( 123456 );
    ASSERT_EQ( IMedia::ProgressResult::AsIs, res );
    ASSERT_EQ( 123456, m->lastTime() );
    ASSERT_EQ( -1.f, m->lastPosition() );
    m = T->ml->media( m->id() );
    ASSERT_EQ( 123456, m->lastTime() );
    ASSERT_EQ( -1.f, m->lastPosition() );
}

static void Rating( Tests* T )
{
    auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "media.avi", IMedia::Type::Video ) );
    ASSERT_FALSE( f->metadata( Media::MetadataType::Rating ).isSet() );
    f->setMetadata( Media::MetadataType::Rating, 12345 );
    ASSERT_EQ( 12345, f->metadata( Media::MetadataType::Rating ).asInt() );
    ASSERT_TRUE( f->metadata( Media::MetadataType::Rating ).isSet() );

    f = T->ml->media( f->id() );
    ASSERT_EQ( 12345, f->metadata( Media::MetadataType::Rating ).asInt() );
}

static void Search( Tests* T )
{
    for ( auto i = 1u; i <= 10u; ++i )
    {
        T->ml->addMedia( "track " + std::to_string( i ) + ".mp3",
                         IMedia::Type::Audio );
    }

    auto media = T->ml->searchMedia( "tra", nullptr )->all();
    ASSERT_EQ( 10u, media.size() );

    media = T->ml->searchMedia( "track 1", nullptr )->all();
    ASSERT_EQ( 2u, media.size() );

    media = T->ml->searchMedia( "grouik", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    media = T->ml->searchMedia( "rack", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
}

static void SearchAndSort( Tests* T )
{
    for ( auto i = 1u; i <= 3u; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "track " + std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        m->setDuration( 3 - i );
    }
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia(
                    "this pattern doesn't match.mp3", IMedia::Type::Audio ) );
    ASSERT_NON_NULL( m );

    auto media = T->ml->searchMedia( "tra", nullptr )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( media[0]->title(), "track 1.mp3" );
    ASSERT_EQ( media[1]->title(), "track 2.mp3" );
    ASSERT_EQ( media[2]->title(), "track 3.mp3" );

    QueryParameters params { SortingCriteria::Duration, false };
    media = T->ml->searchMedia( "tra", &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( media[0]->title(), "track 3.mp3" );
    ASSERT_EQ( media[1]->title(), "track 2.mp3" );
    ASSERT_EQ( media[2]->title(), "track 1.mp3" );
}

static void SearchAfterEdit( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );

    auto media = T->ml->searchMedia( "media", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    m->setTitle( "otters are awesome", true );

    media = T->ml->searchMedia( "media", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    media = T->ml->searchMedia( "otters", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
}

static void SearchAfterDelete( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );

    auto media = T->ml->searchMedia( "media", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    auto f = m->files()[0];
    m->removeFile( static_cast<File&>( *f ) );

    media = T->ml->searchMedia( "media", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
}

static void SearchByLabel( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    auto media = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    auto l = T->ml->createLabel( "otter" );
    m->addLabel( l );

    media = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    auto l2 = T->ml->createLabel( "pangolins" );
    m->addLabel( l2 );

    media = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    media = T->ml->searchMedia( "pangolin", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    m->removeLabel( l );

    media = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    media = T->ml->searchMedia( "pangolin", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    m->addLabel( l );

    media = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    media = T->ml->searchMedia( "pangolin", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    T->ml->deleteLabel( l );

    media = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    media = T->ml->searchMedia( "pangolin", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
}

static void SearchTracks( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );
    for ( auto i = 1u; i <= 10u; ++i )
    {
       auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track " +
                        std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
       a->addTrack( m, i, 1, 0, nullptr );
    }
    auto tracks = T->ml->searchMedia( "tra", nullptr )->all();
    ASSERT_EQ( 10u, tracks.size() );

    tracks = T->ml->searchMedia( "track 1", nullptr )->all();
    ASSERT_EQ( 2u, tracks.size() );

    tracks = T->ml->searchMedia( "grouik", nullptr )->all();
    ASSERT_EQ( 0u, tracks.size() );

    tracks = T->ml->searchMedia( "rack", nullptr )->all();
    ASSERT_EQ( 0u, tracks.size() );
}

static void SearchWeirdPatterns( Tests* T )
{
    auto m = std::static_pointer_cast<Media>(
                T->ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    // All we care about is this not crashing
    // https://code.videolan.org/videolan/medialibrary/issues/116
    auto media = T->ml->searchMedia( "@*\"", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
    media = T->ml->searchMedia( "'''''% % \"'", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
    media = T->ml->searchMedia( "Robert'); DROP TABLE STUDENTS;--", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
}

static void Favorite( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    ASSERT_FALSE( m->isFavorite() );

    m->setFavorite( true );
    ASSERT_TRUE( m->isFavorite() );

    m = T->ml->media( m->id() );
    ASSERT_TRUE( m->isFavorite() );
}

static void History( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );

    auto history = T->ml->history()->all();
    ASSERT_EQ( 0u, history.size() );

    m->setLastPosition( 0.5f );
    history = T->ml->history()->all();
    ASSERT_EQ( 1u, history.size() );
    ASSERT_EQ( m->id(), history[0]->id() );

    compat::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    m2->setLastTime( 50 );

    history = T->ml->history()->all();
    ASSERT_EQ( 2u, history.size() );
    ASSERT_EQ( m2->id(), history[0]->id() );
    ASSERT_EQ( m->id(), history[1]->id() );
}

static void StreamHistory( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addStream( "http://media.org/sample.mkv" ) );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addStream( "http://media.org/sample2.mkv" ) );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "localfile.mkv", IMedia::Type::Video ) );

    m1->setLastPosition( 0.5 );
    m2->setLastPosition( 0.5 );
    m3->setLastPosition( 0.5 );

    auto history = T->ml->streamHistory()->all();
    ASSERT_EQ( 2u, history.size() );

    history = T->ml->history()->all();
    ASSERT_EQ( 1u, history.size() );
}

static void HistoryByType( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "video.mkv", IMedia::Type::Video ) );
    m1->setLastPosition( 0.5f );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "audio.mp3", IMedia::Type::Audio ) );
    m2->setLastTime( 50 );

    auto h = T->ml->history( IMedia::Type::Audio )->all();
    ASSERT_EQ( 1u, h.size() );

    h = T->ml->history( IMedia::Type::Video)->all();
    ASSERT_EQ( 1u, h.size() );

    h = T->ml->history()->all();
    ASSERT_EQ( 2u, h.size() );
}

static void ClearHistory( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );

    auto history = T->ml->history()->all();
    ASSERT_EQ( 0u, history.size() );

    m->setLastPosition( 0.5f );
    history = T->ml->history()->all();
    ASSERT_EQ( 1u, history.size() );

    ASSERT_TRUE( T->ml->clearHistory() );

    history = T->ml->history()->all();
    ASSERT_EQ( 0u, history.size() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( -1.f, m->lastPosition() );
    ASSERT_EQ( -1, m->lastTime() );
}

static void RemoveFromHistory( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    m->setDuration( 100 );

    auto history = T->ml->history()->all();
    ASSERT_EQ( 0u, history.size() );

    m->setLastPosition( 1.f );
    history = T->ml->history()->all();
    ASSERT_EQ( 1u, history.size() );
    ASSERT_EQ( m->id(), history[0]->id() );
    ASSERT_EQ( 1u, m->playCount() );

    m->removeFromHistory();

    history = T->ml->history()->all();
    ASSERT_EQ( 0u, history.size() );
    ASSERT_EQ( 0u, m->playCount() );
    ASSERT_EQ( -1.f, m->lastPosition() );
    ASSERT_EQ( -1, m->lastTime() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( 0u, m->playCount() );
    ASSERT_EQ( -1.f, m->lastPosition() );
    ASSERT_EQ( -1, m->lastTime() );
}

static void SetReleaseDate( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "movie.mkv", IMedia::Type::Video ) );

    ASSERT_EQ( m->releaseDate(), 0u );
    m->setReleaseDate( 1234 );
    ASSERT_EQ( m->releaseDate(), 1234u );

    auto m2 = T->ml->media( m->id() );
    ASSERT_EQ( m2->releaseDate(), 1234u );
}

static void SortByAlpha( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", Media::Type::Audio ) );
    m1->setTitle( "Abcd", true );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", Media::Type::Audio ) );
    m2->setTitle( "Zyxw", true );

    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", Media::Type::Audio) );
    m3->setTitle( "afterA-beforeZ", true );

    QueryParameters params { SortingCriteria::Alpha, false };
    auto media = T->ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    ASSERT_EQ( m2->id(), media[2]->id() );

    params.desc = true;
    media = T->ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[2]->id() );
}

static void SortByReleaseDate( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", Media::Type::Audio ) );
    m1->setReleaseDate( 1111 );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", Media::Type::Audio ) );
    m2->setReleaseDate( 3333 );

    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", Media::Type::Audio ) );
    m3->setReleaseDate( 2222 );

    QueryParameters params { SortingCriteria::ReleaseDate, false };
    auto media = T->ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    ASSERT_EQ( m2->id(), media[2]->id() );

    params.desc = true;
    media = T->ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[2]->id() );
}

static void SortByLastModifDate( Tests* T )
{
    auto file1 = std::make_shared<mock::NoopFile>( "media.mkv" );
    file1->setLastModificationDate( 666 );
    auto m1 = T->ml->addFile( file1, Media::Type::Video );

    auto file2 = std::make_shared<mock::NoopFile>( "media2.mkv" );
    file2->setLastModificationDate( 111 );
    auto m2 = T->ml->addFile( file2, Media::Type::Video );

    QueryParameters params { SortingCriteria::LastModificationDate, false };
    auto media = T->ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );

    params.desc = true;
    media = T->ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

static void SortByFileSize( Tests* T )
{
    auto file1 = std::make_shared<mock::NoopFile>( "media.mkv" );
    file1->setSize( 666 );
    auto m1 = T->ml->addFile( file1, Media::Type::Video );

    auto file2 = std::make_shared<mock::NoopFile>( "media2.mkv" );
    file2->setSize( 111 );
    auto m2 = T->ml->addFile( file2, Media::Type::Video );

    QueryParameters params { SortingCriteria::FileSize, false };
    auto media = T->ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );

    params.desc = true;
    media = T->ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

static void SortByFilename( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "zzzzz.mp3", Media::Type::Video ) );
    m1->setTitle( "aaaaa", false );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "aaaaa.mp3", Media::Type::Video ) );
    m2->setTitle( "zzzzz", false );

    QueryParameters params { SortingCriteria::Filename, false };
    auto media = T->ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );

    params.desc = true;
    media = T->ml->videoFiles( &params )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

static void SetType( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addExternalMedia( "media1.mp3", -1 ) );
    ASSERT_TRUE( m1->isExternalMedia() );

    auto res = m1->setType( IMedia::Type::Video );
    ASSERT_TRUE( res );

    ASSERT_EQ( IMedia::Type::Video, m1->type() );

    auto m2 = T->ml->media( m1->id() );
    ASSERT_EQ( IMedia::Type::Video, m2->type() );

    // For safety check, just set the type to the current value and ensure it
    // still returns true
    res = m1->setType( IMedia::Type::Video );
    ASSERT_TRUE( res );
}

static void GetMetadata( Tests* T )
{
    auto m = T->ml->addMedia( "media.mp3", IMedia::Type::Audio );

    {
        const auto& md = m->metadata( Media::MetadataType::Speed );
        ASSERT_FALSE( md.isSet() );
    }

    auto res = m->setMetadata( Media::MetadataType::Speed, "foo" );
    ASSERT_TRUE( res );

    {
        const auto& md = m->metadata( Media::MetadataType::Speed );
        ASSERT_EQ( "foo", md.asStr() );
    }

    m = T->ml->media( m->id() );
    const auto& md = m->metadata( Media::MetadataType::Speed );
    ASSERT_EQ( "foo", md.asStr() );

    res = m->setMetadata( Media::MetadataType::Speed, "12.34" );
    ASSERT_TRUE( res );

    const auto& mdDouble = m->metadata( Media::MetadataType::Speed );
    ASSERT_EQ( 12.34, mdDouble.asDouble() );

}

static void MetadataOverride( Tests* T )
{
    auto m = T->ml->addMedia( "media.mp3", IMedia::Type::Audio );
    auto res = m->setMetadata( Media::MetadataType::Speed, "foo" );
    ASSERT_TRUE( res );

    m->setMetadata( Media::MetadataType::Speed, "otter" );
    {
        const auto& md = m->metadata( Media::MetadataType::Speed );
        ASSERT_EQ( "otter", md.asStr() );
    }

    m = T->ml->media( m->id() );
    const auto& md = m->metadata( Media::MetadataType::Speed );
    ASSERT_EQ( "otter", md.asStr() );
}

static void MetadataUnset( Tests* T )
{
    auto m = T->ml->addMedia( "media.mp3", IMedia::Type::Audio );
    auto res = m->unsetMetadata( Media::MetadataType::ApplicationSpecific );
    ASSERT_TRUE( res );

    res = m->setMetadata( Media::MetadataType::ApplicationSpecific, "otters" );
    ASSERT_TRUE( res );

    auto& md = m->metadata( Media::MetadataType::ApplicationSpecific );
    ASSERT_TRUE( md.isSet() );
    ASSERT_EQ( "otters", md.asStr() );

    res = m->unsetMetadata( Media::MetadataType::ApplicationSpecific );
    ASSERT_TRUE( res );

    ASSERT_FALSE( md.isSet() );

    m = T->ml->media( m->id() );
    auto& md2 = m->metadata( Media::MetadataType::ApplicationSpecific );
    ASSERT_FALSE( md2.isSet() );
}

static void MetadataGetBatch( Tests* T )
{
    auto m = T->ml->addMedia( "media.mp3", IMedia::Type::Audio );
    auto metas = m->metadata();
    ASSERT_EQ( 0u, metas.size() );

    m->setMetadata( IMedia::MetadataType::Crop, "crop" );
    m->setMetadata( IMedia::MetadataType::Gain, "gain" );
    m->setMetadata( IMedia::MetadataType::Rating, "five stars" );

    metas = m->metadata();
    ASSERT_EQ( 3u, metas.size() );
    ASSERT_EQ( metas[IMedia::MetadataType::Crop], "crop" );
    ASSERT_EQ( metas[IMedia::MetadataType::Gain], "gain" );
    ASSERT_EQ( metas[IMedia::MetadataType::Rating], "five stars" );

    m = T->ml->media( m->id() );
    metas = m->metadata();
    ASSERT_EQ( 3u, metas.size() );
    ASSERT_EQ( metas[IMedia::MetadataType::Crop], "crop" );
    ASSERT_EQ( metas[IMedia::MetadataType::Gain], "gain" );
    ASSERT_EQ( metas[IMedia::MetadataType::Rating], "five stars" );

    m->unsetMetadata( IMedia::MetadataType::Gain );
    metas = m->metadata();
    ASSERT_EQ( 2u, metas.size() );
    ASSERT_EQ( metas[IMedia::MetadataType::Crop], "crop" );
    ASSERT_EQ( metas[IMedia::MetadataType::Rating], "five stars" );
}

static void SetBatch( Tests* T )
{
    auto m = T->ml->addMedia( "media.mp3", IMedia::Type::Audio );
    auto metas = m->metadata();
    ASSERT_EQ( 0u, metas.size() );

    auto res = m->setMetadata( {
        { IMedia::MetadataType::Crop, "crop" },
        { IMedia::MetadataType::Gain, "gain" },
        { IMedia::MetadataType::Rating, "five stars" },
    });
    ASSERT_TRUE( res );

    metas = m->metadata();
    ASSERT_EQ( 3u, metas.size() );
    ASSERT_EQ( metas[IMedia::MetadataType::Crop], "crop" );
    ASSERT_EQ( metas[IMedia::MetadataType::Gain], "gain" );
    ASSERT_EQ( metas[IMedia::MetadataType::Rating], "five stars" );

    m = T->ml->media( m->id() );
    metas = m->metadata();
    ASSERT_EQ( 3u, metas.size() );
    ASSERT_EQ( metas[IMedia::MetadataType::Crop], "crop" );
    ASSERT_EQ( metas[IMedia::MetadataType::Gain], "gain" );
    ASSERT_EQ( metas[IMedia::MetadataType::Rating], "five stars" );

    // Partial override
    m->setMetadata( {
        { IMedia::MetadataType::Rating, "une étoile" },
        { IMedia::MetadataType::Zoom, "zoom"  }
    });
    metas = m->metadata();
    ASSERT_EQ( 4u, metas.size() );
    ASSERT_EQ( metas[IMedia::MetadataType::Crop], "crop" );
    ASSERT_EQ( metas[IMedia::MetadataType::Gain], "gain" );
    ASSERT_EQ( metas[IMedia::MetadataType::Rating], "une étoile" );
    ASSERT_EQ( metas[IMedia::MetadataType::Zoom], "zoom" );
}

static void MetadataCheckDbModel( Tests* T )
{
    auto res = Metadata::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void ExternalMrl( Tests* T )
{
    auto m = T->ml->addExternalMedia( "https://foo.bar/sea-otters.mkv", 1234 );
    ASSERT_NE( nullptr, m );

    ASSERT_EQ( m->title(), "sea-otters.mkv" );
    ASSERT_EQ( 1234, m->duration() );
    ASSERT_TRUE( m->isExternalMedia() );

    // External files shouldn't appear in listings
    auto videos = T->ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 0u, videos.size() );

    auto audios = T->ml->audioFiles( nullptr )->all();
    ASSERT_EQ( 0u, audios.size() );

    auto m2 = T->ml->media( "https://foo.bar/sea-otters.mkv" );
    ASSERT_NE( nullptr, m2 );
    ASSERT_EQ( m->id(), m2->id() );
    ASSERT_TRUE( m2->isExternalMedia() );

    auto files = m2->files();
    ASSERT_EQ( 1u, files.size() );
    auto f = files[0];
    ASSERT_TRUE( f->isExternal() );
    ASSERT_TRUE( f->isNetwork() );
    ASSERT_EQ( File::Type::Main, f->type() );

    auto f2 = m2->addExternalMrl( "file:///path/to/subtitles.srt", IFile::Type::Subtitles );
    ASSERT_NE( nullptr, f2 );
    ASSERT_FALSE( f2->isNetwork() );

    files = m2->files();
    ASSERT_EQ( 2u, files.size() );
    ASSERT_TRUE( files[0]->isExternal() );
    ASSERT_TRUE( files[0]->isNetwork() );
    ASSERT_EQ( File::Type::Main, files[0]->type() );

    ASSERT_TRUE( files[1]->isExternal() );
    ASSERT_FALSE( files[1]->isNetwork() );
    ASSERT_EQ( File::Type::Subtitles, files[1]->type() );

    auto m3 = T->ml->addExternalMedia( "https://foo.bar/media.mkv", -1234 );
    ASSERT_NE( nullptr, m3 );
    ASSERT_EQ( -1, m3->duration() );
}

static void AddStream( Tests* T )
{
    auto m = T->ml->addStream( "https://foo.bar/stream.mkv" );
    ASSERT_EQ( m->title(), "stream.mkv" );
    ASSERT_TRUE( m->isStream() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( m->title(), "stream.mkv" );
    ASSERT_TRUE( m->isStream() );
}

static void DuplicatedExternalMrl( Tests* T )
{
    auto m = T->ml->addExternalMedia( "http://foo.bar", 1 );
    auto m2 = T->ml->addExternalMedia( "http://foo.bar", 2 );
    ASSERT_NE( nullptr, m );
    ASSERT_EQ( nullptr, m2 );
}

static void SetTitle( Tests* T )
{
    auto m = T->ml->addMedia( "media", IMedia::Type::Video );
    ASSERT_EQ( "media", m->title() );
    auto res = m->setTitle( "sea otters" );
    ASSERT_TRUE( res );
    ASSERT_EQ( "sea otters", m->title() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( "sea otters", m->title() );
}

static void Pagination( Tests* T )
{
    for ( auto i = 1u; i <= 9u; ++i )
    {
       auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track " +
                        std::to_string( i ) + ".mp3", IMedia::Type::Video ) );
    }

    auto allMedia = T->ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 9u, allMedia.size() );

    auto paginator = T->ml->videoFiles( nullptr );
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

static void SortFilename( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "AAAAB.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "aaaaa.mp3", IMedia::Type::Audio ) );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "BbBbB.mp3", IMedia::Type::Audio ) );

    QueryParameters params { SortingCriteria::Filename, false };
    auto media = T->ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );

    params.desc = true;
    media = T->ml->audioFiles( &params )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m2->id(), media[2]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[0]->id() );
}

static void CreateStream( Tests* T )
{
    auto m1 = T->ml->addStream( "http://foo.bar/media.mkv" );
    ASSERT_TRUE( m1->isStream() );
}

static void SearchExternal( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addExternalMedia( "localfile.mkv", -1 ) );
    m1->setTitle( "local otter", false );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addStream( "http://remote.file/media.asf" ) );
    m2->setTitle( "remote otter", false );

    auto media = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    // The type is no longer bound to the internal/external distinction, so
    // updating the type will not yield different results anymore.
    T->ml->setMediaType( m1->id(), IMedia::Type::Video );
    T->ml->setMediaType( m2->id(), IMedia::Type::Video );

    media = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    media = T->ml->searchVideo( "otter", nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
}

static void VacuumOldExternal( Tests* T )
{
    auto m1 = T->ml->addExternalMedia( "foo.avi", -1 );
    auto m2 = T->ml->addExternalMedia( "bar.mp3", -1 );
    auto s1 = T->ml->addStream( "http://baz.mkv" );

    ASSERT_NE( nullptr, m1 );
    ASSERT_NE( nullptr, m2 );
    ASSERT_NE( nullptr, s1 );

    // Check that they will not be vacuumed even if they haven't been played yet.

    m1 = T->ml->media( m1->id() );
    m2 = T->ml->media( m2->id() );
    s1 = T->ml->media( s1->id() );

    ASSERT_NE( nullptr, m1 );
    ASSERT_NE( nullptr, m2 );
    ASSERT_NE( nullptr, s1 );

    auto playlist = T->ml->createPlaylist( "playlist" );
    playlist->append( *m1 );

    T->ml->outdateAllExternalMedia();

    MediaLibrary::removeOldEntities( T->ml.get() );

    m1 = T->ml->media( m1->id() );
    m2 = T->ml->media( m2->id() );
    s1 = T->ml->media( s1->id() );

    ASSERT_NE( nullptr, m1 );
    ASSERT_EQ( nullptr, m2 );
    ASSERT_EQ( nullptr, s1 );
}

static void VacuumNeverPlayedMedia( Tests* T )
{
    auto m1 = T->ml->addExternalMedia( "foo.avi", -1 );
    auto m2 = T->ml->addExternalMedia( "bar.mp3", -1 );
    auto s1 = T->ml->addStream( "http://baz.mkv" );

    ASSERT_NE( nullptr, m1 );
    ASSERT_NE( nullptr, m2 );
    ASSERT_NE( nullptr, s1 );

    T->ml->setMediaInsertionDate( m1->id(), 1 );

    MediaLibrary::removeOldEntities( T->ml.get() );

    m1 = T->ml->media( m1->id() );
    m2 = T->ml->media( m2->id() );
    s1 = T->ml->media( s1->id() );

    ASSERT_EQ( nullptr, m1 );
    ASSERT_NE( nullptr, m2 );
    ASSERT_NE( nullptr, s1 );
}

static void RemoveExternal( Tests* T )
{
    auto m = T->ml->addExternalMedia( "http://extern.al/media.mkv", -1 );
    ASSERT_NE( nullptr, m );

    auto res = T->ml->removeExternalMedia( m );
    ASSERT_TRUE( res );

    m = T->ml->media( m->id() );
    ASSERT_EQ( nullptr, m );
    m = T->ml->media( "http://extern.al/media.mkv" );
    ASSERT_EQ( nullptr, m );
}

static void NbPlaylists( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addExternalMedia( "media.mkv", -1 ) );
    ASSERT_EQ( 0u, m->nbPlaylists() );

    auto playlist = T->ml->createPlaylist( "playlisẗ" );
    auto res = playlist->append( *m );
    ASSERT_TRUE( res );

    m = T->ml->media( m->id() );
    ASSERT_EQ( 1u, m->nbPlaylists() );

    playlist = T->ml->playlist( playlist->id() );
    playlist->remove( 0 );

    m = T->ml->media( m->id() );
    ASSERT_EQ( 0u, m->nbPlaylists() );

    playlist = T->ml->playlist( playlist->id() );
    playlist->append( *m );
    playlist->append( *m );

    m = T->ml->media( m->id() );
    playlist = T->ml->playlist( playlist->id() );

    ASSERT_EQ( 2u, m->nbPlaylists() );

    playlist->remove( 0 );

    m = T->ml->media( m->id() );
    // The media was inserted twice in the playlist and should therefor still have
    // one entry there, causing the number of playlist to be 1
    ASSERT_EQ( 1u, m->nbPlaylists() );

    T->ml->deletePlaylist( playlist->id() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( 0u, m->nbPlaylists() );
}

static void SortByAlbum( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );

    // Create the albums in reversed alphabetical order to ensure id & alpha orders
    // are different
    auto album1 = T->ml->createAlbum( "Ziltoid ");
    auto album2 = T->ml->createAlbum( "Addicted" );

    album1->addTrack( m2, 1, 0, 0, nullptr );
    album1->addTrack( m1, 2, 0, 0, nullptr );
    album2->addTrack( m3, 1, 0, 0, nullptr );

    // Album1: [m2; m1]
    // Album2: [m3]

    auto queryParams = QueryParameters{};
    queryParams.desc = false;
    queryParams.sort = SortingCriteria::Album;
    auto tracks = T->ml->audioFiles( &queryParams )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( m3->id(), tracks[0]->id() );
    ASSERT_EQ( m2->id(), tracks[1]->id() );
    ASSERT_EQ( m1->id(), tracks[2]->id() );

    queryParams.desc = true;
    tracks = T->ml->audioFiles( &queryParams )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( m2->id(), tracks[0]->id() );
    ASSERT_EQ( m1->id(), tracks[1]->id() );
    ASSERT_EQ( m3->id(), tracks[2]->id() );
}

static void SetFilename( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    ASSERT_EQ( "media.mkv", m->fileName() );

    m->setFileName( "sea_otter.asf" );
    ASSERT_EQ( "sea_otter.asf", m->fileName() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( "sea_otter.asf", m->fileName() );
}

static void SetPlayCount( Tests* T )
{
    auto m = T->ml->addMedia( "media.avi", IMedia::Type::Video );
    ASSERT_EQ( 0u, m->playCount() );
    auto res = m->setPlayCount( 123 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 123u, m->playCount() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( 123u, m->playCount() );
}

static void CheckDbModel( Tests* T )
{
    auto res = Media::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void SortByTrackId( Tests* T )
{
    // Create 3 albums, each with one track.
    // Aalbum1: track 3
    // Balbum2: track 2
    // Calbum3: track 1
    // To ensure tracks are first grouped by album, and only then by tracks
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );

    // Create the albums in reversed alphabetical order to ensure id & alpha orders
    // are different
    auto album1 = T->ml->createAlbum( "Aalbum1");
    auto album2 = T->ml->createAlbum( "Balbum2" );
    auto album3 = T->ml->createAlbum( "Calbum3" );

    album1->addTrack( m1, 3, 0, 0, nullptr );
    album2->addTrack( m2, 2, 0, 0, nullptr );
    album3->addTrack( m3, 1, 0, 0, nullptr );

    QueryParameters params;
    params.sort = SortingCriteria::TrackId;
    params.desc = false;
    auto tracksQuery = T->ml->audioFiles( &params );
    ASSERT_EQ( 3u, tracksQuery->count() );
    auto tracks = tracksQuery->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( tracks[0]->id(), m1->id() );
    ASSERT_EQ( tracks[1]->id(), m2->id() );
    ASSERT_EQ( tracks[2]->id(), m3->id() );

    params.desc = true;
    tracksQuery = T->ml->audioFiles( &params );
    ASSERT_EQ( 3u, tracksQuery->count() );
    tracks = tracksQuery->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( tracks[0]->id(), m1->id() );
    ASSERT_EQ( tracks[1]->id(), m2->id() );
    ASSERT_EQ( tracks[2]->id(), m3->id() );
}

static void ForceTitle( Tests* T )
{
    auto m = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    ASSERT_NE( nullptr, m );
    ASSERT_EQ( "media.mkv", m->title() );

    // Set an unforced title
    std::string title{ "new title" };
    auto res = m->setTitle( title, false );
    ASSERT_TRUE( res );
    // It should be updated
    ASSERT_EQ( title, m->title() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( title, m->title() );

    // Now force a new title
    title = "forced title";
    res = m->setTitle( title, true );
    ASSERT_TRUE( res );
    // It should still be updated
    ASSERT_EQ( title, m->title() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( title, m->title() );

    // Now set a non forced title, it should be rejected
    std::string rejectedTitle{ "another title" };
    res = m->setTitle( rejectedTitle, false );
    ASSERT_TRUE( res );
    ASSERT_NE( rejectedTitle, m->title() );
    ASSERT_EQ( title, m->title() );

    m = T->ml->media( m->id() );
    ASSERT_NE( rejectedTitle, m->title() );
    ASSERT_EQ( title, m->title() );

    // Check that we can force a new title:
    title = "new forced title";
    res = m->setTitle( title, true );
    ASSERT_TRUE( res );
    // It should still be updated
    ASSERT_EQ( title, m->title() );

    m = T->ml->media( m->id() );
    ASSERT_EQ( title, m->title() );
}

static void FetchInProgress( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    m1->setDuration( 10000 );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    m2->setDuration( 10000 );

    auto m = T->ml->inProgressMedia( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, m.size() );

    m1->setLastPosition( .5f );
    m = T->ml->inProgressMedia( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, m.size() );
    ASSERT_EQ( m1->id(), m[0]->id() );

    m = T->ml->inProgressMedia( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, m.size() );
    ASSERT_EQ( m1->id(), m[0]->id() );

    m = T->ml->inProgressMedia( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 0u, m.size() );

    m2->setLastPosition( 0.8f );

    m = T->ml->inProgressMedia( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 2u, m.size() );

    m = T->ml->inProgressMedia( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, m.size() );
    ASSERT_EQ( m1->id(), m[0]->id() );

    m = T->ml->inProgressMedia( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 1u, m.size() );
    ASSERT_EQ( m2->id(), m[0]->id() );

    m1->setLastPosition( 0.999f );

    m = T->ml->inProgressMedia( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, m.size() );
    ASSERT_EQ( m2->id(), m[0]->id() );

    m1 = T->ml->media( m1->id() );
    ASSERT_EQ( -1.f, m1->lastPosition() );
    ASSERT_EQ( -1, m1->lastTime() );
    ASSERT_EQ( 1u, m1->playCount() );
}

static void ConvertToExternal( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    auto movie = T->ml->createMovie( *m1 );
    ASSERT_NON_NULL( movie );

    auto videos = T->ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 2u, videos.size() );

    ASSERT_TRUE( m1->isDiscoveredMedia() );
    auto res = m1->convertToExternal();
    ASSERT_TRUE( res );
    ASSERT_FALSE( m1->isDiscoveredMedia() );

    videos = T->ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 1u, videos.size() );

    m1 = T->ml->media( m1->id() );
    ASSERT_TRUE( m1->isExternalMedia() );
    ASSERT_EQ( 0, m1->deviceId() );

    auto files = m1->files();
    for ( const auto& f : files )
    {
        ASSERT_TRUE( f->isExternal() );
        ASSERT_FALSE( f->isRemovable() );
        ASSERT_EQ( 0u, static_cast<File*>( f.get() )->folderId() );
    }

    m2 = T->ml->media( m2->id() );
    ASSERT_NE( nullptr, m2 );
    ASSERT_FALSE( m2->isExternalMedia() );
}

static void FlushUserProvidedThumbnails( Tests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    auto t = m->thumbnail( ThumbnailSizeType::Thumbnail );
    auto t2 = m2->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( t, nullptr );
    ASSERT_EQ( t2, nullptr );

    auto res = m->setThumbnail( "file:///path/to/thumb.jpg", ThumbnailSizeType::Thumbnail );
    ASSERT_TRUE( res );

    std::string ownedThumbnailMrl = utils::file::toMrl( T->ml->thumbnailPath() + "thumb.jpg" );
    t2 = std::make_shared<Thumbnail>( T->ml.get(), ownedThumbnailMrl,
                                      Thumbnail::Origin::Media, ThumbnailSizeType::Thumbnail,
                                      true );
    res = m2->setThumbnail( std::move( t2 ) );
    ASSERT_TRUE( res );
    t2 = m2->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NON_NULL( t2 );

    res = T->ml->flushUserProvidedThumbnails();
    ASSERT_TRUE( res );

    /* Ensure we don't fetch something from the media cache */
    m = T->ml->media( m->id() );
    m2 = T->ml->media( m2->id() );
    ASSERT_NON_NULL( m );
    ASSERT_NON_NULL( m2 );

    t = m->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( t, nullptr );
    t2 = m2->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NON_NULL( t2 );

    /*
     * We can't use Thumbnail::fetchAll since the Thumbnail constructor expects
     * both Thumbnail and ThumbnailLinking to be selected from.
     * We're just want to check that removing from the linking table removed
     * the thumbnail as well and that we only have a single thumbnail in DB now
     */
    {
        medialibrary::sqlite::Statement stmt{ T->ml->getConn()->handle(),
                "SELECT COUNT(*) FROM " + Thumbnail::Table::Name
        };
        stmt.execute();
        auto row = stmt.row();
        ASSERT_EQ( 1u, row.nbColumns() );
        auto nbThumbnails = row.extract<int64_t>();
        ASSERT_EQ( nbThumbnails, 1u );
    }
}

int main( int ac, char** av )
{
    INIT_TESTS( Media );

    ADD_TEST( Create );
    ADD_TEST( Fetch );
    ADD_TEST( Duration );
    ADD_TEST( GetThumbnail );
    ADD_TEST( SetProgress );
    ADD_TEST( SetLastPositionNoDuration );
    ADD_TEST( Rating );
    ADD_TEST( Search );
    ADD_TEST( SearchAndSort );
    ADD_TEST( SearchAfterEdit );
    ADD_TEST( SearchAfterDelete );
    ADD_TEST( SearchByLabel );
    ADD_TEST( SearchTracks );
    ADD_TEST( SearchWeirdPatterns );
    ADD_TEST( Favorite );
    ADD_TEST( History );
    ADD_TEST( StreamHistory );
    ADD_TEST( HistoryByType );
    ADD_TEST( ClearHistory );
    ADD_TEST( RemoveFromHistory );
    ADD_TEST( SetReleaseDate );
    ADD_TEST( SortByAlpha );
    ADD_TEST( SortByReleaseDate );
    ADD_TEST( SortByLastModifDate );
    ADD_TEST( SortByFileSize );
    ADD_TEST( SortByFilename );
    ADD_TEST( SetType );
    ADD_TEST( GetMetadata );
    ADD_TEST( MetadataOverride );
    ADD_TEST( MetadataUnset );
    ADD_TEST( MetadataGetBatch );
    ADD_TEST( SetBatch );
    ADD_TEST( MetadataCheckDbModel );
    ADD_TEST( ExternalMrl );
    ADD_TEST( AddStream );
    ADD_TEST( DuplicatedExternalMrl );
    ADD_TEST( SetTitle );
    ADD_TEST( Pagination );
    ADD_TEST( SortFilename );
    ADD_TEST( CreateStream );
    ADD_TEST( SearchExternal );
    ADD_TEST( VacuumOldExternal );
    ADD_TEST( VacuumNeverPlayedMedia );
    ADD_TEST( RemoveExternal );
    ADD_TEST( NbPlaylists );
    ADD_TEST( SortByAlbum );
    ADD_TEST( SetFilename );
    ADD_TEST( SetPlayCount );
    ADD_TEST( CheckDbModel );
    ADD_TEST( SortByTrackId );
    ADD_TEST( ForceTitle );
    ADD_TEST( FetchInProgress );
    ADD_TEST( ConvertToExternal );
    ADD_TEST( FlushUserProvidedThumbnails );

    END_TESTS
}
