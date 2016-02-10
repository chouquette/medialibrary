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

#include "IMediaLibrary.h"
#include "File.h"
#include "Media.h"
#include "Artist.h"
#include "Album.h"
#include "AlbumTrack.h"

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
       a->addTrack( m, i, 1 );
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

    std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    auto m2 = ml->addFile( "media.mkv" );
    m2->increasePlayCount();
    m2->save();

    history = ml->lastMediaPlayed();
    ASSERT_EQ( 2u, history.size() );
    ASSERT_EQ( m2->id(), history[0]->id() );
    ASSERT_EQ( m->id(), history[1]->id() );
}
