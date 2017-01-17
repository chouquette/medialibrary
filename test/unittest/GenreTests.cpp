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

#include "Genre.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Media.h"
#include "Artist.h"

class Genres : public Tests
{
protected:
    std::shared_ptr<Genre> g;

    virtual void SetUp() override
    {
        Tests::SetUp();
        g = ml->createGenre( "genre" );
    }
};

TEST_F( Genres, Create )
{
    ASSERT_NE( nullptr, g );
    ASSERT_EQ( "genre", g->name() );
    auto tracks = g->tracks( SortingCriteria::Default, false );
    ASSERT_EQ( 0u, tracks.size() );
}

TEST_F( Genres, List )
{
    auto g2 = ml->createGenre( "genre 2" );
    ASSERT_NE( nullptr, g2 );
    auto genres = ml->genres( SortingCriteria::Default, false );
    ASSERT_EQ( 2u, genres.size() );
}

TEST_F( Genres, ListAlbumTracks )
{
    auto a = ml->createAlbum( "album" );

    for ( auto i = 1u; i <= 3; i++ )
    {
        auto m = std::static_pointer_cast<Media>( ml->addMedia( "track" + std::to_string( i ) + ".mp3" ) );
        auto t = a->addTrack( m, i, 1, 0, nullptr );
        if ( i != 1 )
            t->setGenre( g );
    }
    auto tracks = g->tracks( SortingCriteria::Default, false );
    ASSERT_EQ( 2u, tracks.size() );
}

TEST_F( Genres, ListArtists )
{
    auto artists = g->artists( SortingCriteria::Default, false );
    ASSERT_EQ( 0u, artists.size() );

    auto a = ml->createArtist( "artist" );
    auto a2 = ml->createArtist( "artist 2" );
    // Ensure we're not just returning all the artists:
    auto a3 = ml->createArtist( "artist 3" );
    auto album = ml->createAlbum( "album" );
    auto album2 = ml->createAlbum( "album2" );

    for ( auto i = 1u; i <= 5; ++i )
    {
        auto m = std::static_pointer_cast<Media>( ml->addMedia( std::to_string( i ) + ".mp3" ) );
        auto track = album->addTrack( m, i, 1, a->id(), nullptr );
        track->setGenre( g );
    }
    for ( auto i = 1u; i <= 5; ++i )
    {
        auto m = std::static_pointer_cast<Media>( ml->addMedia( std::to_string( i ) + "_2.mp3" ) );
        auto track = album2->addTrack( m, i, 1, a2->id(), nullptr );
        track->setGenre( g );
    }
    artists = g->artists( SortingCriteria::Default, false );
    ASSERT_EQ( 2u, artists.size() );
}

TEST_F( Genres, ListAlbums )
{
    auto album = ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "some track.mp3" ) );
    auto t = album->addTrack( m, 10, 1, 0, nullptr );
    t->setGenre( g );

    auto album2 = ml->createAlbum( "album2" );
    m = std::static_pointer_cast<Media>( ml->addMedia( "some other track.mp3" ) );
    t = album2->addTrack( m, 10, 1, 0, nullptr );
    t->setGenre( g );

    // We have 2 albums with at least a song with genre "g" (as defined in SetUp)
    // Now we create more albums with "random" genre, all of them should have 1 album
    for ( auto i = 1u; i <= 5u; ++i )
    {
        auto m = std::static_pointer_cast<Media>( ml->addMedia( std::to_string( i ) + ".mp3" ) );
        auto track = album->addTrack( m, i, 1, 0, nullptr );
        auto g = ml->createGenre( std::to_string( i ) );
        track->setGenre( g );
    }

    auto genres = ml->genres( SortingCriteria::Default, false );
    for ( auto& genre : genres )
    {
        auto albums = genre->albums( SortingCriteria::Default, false );

        if ( genre->id() == g->id() )
        {
            // Initial genre with 2 albums:
            ASSERT_EQ( 2u, albums.size() );
        }
        else
        {
            ASSERT_EQ( 1u, albums.size() );
            ASSERT_EQ( album->id(), albums[0]->id() );
        }
    }
}

TEST_F( Genres, Search )
{
    ml->createGenre( "something" );
    ml->createGenre( "blork" );

    auto genres = ml->searchGenre( "genr" );
    ASSERT_EQ( 1u, genres.size() );
}

TEST_F( Genres, SearchAfterDelete )
{
    auto genres = ml->searchGenre( "genre" );
    ASSERT_EQ( 1u, genres.size() );

    ml->deleteGenre( g->id() );

    genres = ml->searchGenre( "genre" );
    ASSERT_EQ( 0u, genres.size() );
}

TEST_F( Genres, SortTracks )
{
    auto a = ml->createAlbum( "album" );

    for ( auto i = 1u; i <= 2; i++ )
    {
        auto m = std::static_pointer_cast<Media>( ml->addMedia( "track" + std::to_string( i ) + ".mp3" ) );
        auto t = a->addTrack( m, i, 1, 0, nullptr );
        m->setDuration( i );
        m->save();
        t->setGenre( g );
    }
    auto tracks = g->tracks( SortingCriteria::Duration, false );
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 1u, tracks[0]->albumTrack()->trackNumber() );
    ASSERT_EQ( 2u, tracks[1]->albumTrack()->trackNumber() );

    tracks = g->tracks( SortingCriteria::Duration, true );
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 1u, tracks[1]->albumTrack()->trackNumber() );
    ASSERT_EQ( 2u, tracks[0]->albumTrack()->trackNumber() );
}

TEST_F( Genres, Sort )
{
    auto g2 = ml->createGenre( "metal" );

    auto genres = ml->genres( SortingCriteria::Default, false );
    ASSERT_EQ( 2u, genres.size() );
    ASSERT_EQ( g->id(), genres[0]->id() );
    ASSERT_EQ( g2->id(), genres[1]->id() );

    genres = ml->genres( SortingCriteria::Default, true );
    ASSERT_EQ( 2u, genres.size() );
    ASSERT_EQ( g->id(), genres[1]->id() );
    ASSERT_EQ( g2->id(), genres[0]->id() );
}

TEST_F( Genres, NbTracks )
{
    ASSERT_EQ( 0u, g->nbTracks() );

    auto a = ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "track.mp3" ) );
    auto t = a->addTrack( m, 1, 1, g->id(), g.get() );

    ASSERT_EQ( 1u, g->nbTracks() );
    Reload();
    g = std::static_pointer_cast<Genre>( ml->genre( g->id() ) );
    ASSERT_EQ( 1u, g->nbTracks() );

    auto g2 = ml->createGenre( "otter metal" );
    t = std::static_pointer_cast<AlbumTrack>( ml->albumTrack( t->id() ) );
    t->setGenre( g2 );
    ASSERT_EQ( 1u, g2->nbTracks() );
    // check that the cache got updated, check for DB deletion afterward
    ASSERT_EQ( 0u, g->nbTracks() );

    Reload();

    g = std::static_pointer_cast<Genre>( ml->genre( g->id() ) );
    g2 = std::static_pointer_cast<Genre>( ml->genre( g2->id() ) );
    ASSERT_EQ( nullptr, g );
    ASSERT_EQ( 1u, g2->nbTracks() );

    ml->deleteTrack( t->id() );
    Reload();

    g2 = std::static_pointer_cast<Genre>( ml->genre( g2->id() ) );
    ASSERT_EQ( nullptr, g2 );
}
