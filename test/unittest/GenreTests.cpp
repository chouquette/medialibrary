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
    auto tracks = g->tracks( nullptr )->all();
    ASSERT_EQ( 0u, tracks.size() );
}

TEST_F( Genres, List )
{
    auto g2 = ml->createGenre( "genre 2" );
    ASSERT_NE( nullptr, g2 );
    auto genres = ml->genres( nullptr )->all();
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
    auto tracks = g->tracks( nullptr )->all();
    ASSERT_EQ( 2u, tracks.size() );
}

TEST_F( Genres, ListArtists )
{
    auto artists = g->artists( nullptr )->all();
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
    artists = g->artists( nullptr )->all();
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

    auto genres = ml->genres( nullptr )->all();
    for ( auto& genre : genres )
    {
        auto albums = genre->albums( nullptr )->all();

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

    auto genres = ml->searchGenre( "genr", nullptr )->all();
    ASSERT_EQ( 1u, genres.size() );
}

TEST_F( Genres, SearchAfterDelete )
{
    auto genres = ml->searchGenre( "genre", nullptr )->all();
    ASSERT_EQ( 1u, genres.size() );

    ml->deleteGenre( g->id() );

    genres = ml->searchGenre( "genre", nullptr )->all();
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
    QueryParameters params { SortingCriteria::Duration, false };
    auto tracks = g->tracks( &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 1u, tracks[0]->albumTrack()->trackNumber() );
    ASSERT_EQ( 2u, tracks[1]->albumTrack()->trackNumber() );

    params.desc = true;
    tracks = g->tracks( &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 1u, tracks[1]->albumTrack()->trackNumber() );
    ASSERT_EQ( 2u, tracks[0]->albumTrack()->trackNumber() );
}

TEST_F( Genres, Sort )
{
    auto g2 = ml->createGenre( "metal" );

    auto genres = ml->genres( nullptr )->all();
    ASSERT_EQ( 2u, genres.size() );
    ASSERT_EQ( g->id(), genres[0]->id() );
    ASSERT_EQ( g2->id(), genres[1]->id() );

    QueryParameters params { SortingCriteria::Default, true };
    genres = ml->genres( &params )->all();
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

TEST_F( Genres, CaseInsensitive )
{
    auto g2 = Genre::fromName( ml.get(), "GENRE" );
    ASSERT_EQ( g->id(), g2->id() );
}

TEST_F( Genres, SearchArtists )
{
    auto artists = g->artists( nullptr )->all();
    ASSERT_EQ( 0u, artists.size() );

    auto a = ml->createArtist( "loutre 1" );
    auto a2 = ml->createArtist( "loutre 2" );
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
    }
    artists = ml->searchArtists( "loutre", nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );

    artists = g->searchArtists( "loutre" )->all();
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( a->id(), artists[0]->id() );
}

TEST_F( Genres, SearchTracks )
{
    auto a = ml->createAlbum( "album" );

    auto m = std::static_pointer_cast<Media>( ml->addMedia( "Hell's Kitchen.mp3" ) );
    m->setType( IMedia::Type::Audio );
    auto t = a->addTrack( m, 1, 1, 0, g.get() );
    m->save();

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "Different genre Hell's Kitchen.mp3" ) );
    m2->setType( IMedia::Type::Audio );
    auto t2 = a->addTrack( m2, 1, 1, 0, nullptr );
    m2->save();

    auto tracks = ml->searchAudio( "kitchen", nullptr )->all();
    ASSERT_EQ( 2u, tracks.size() );

    tracks = g->searchTracks( "kitchen", nullptr )->all();
    ASSERT_EQ( 1u, tracks.size() );
    ASSERT_EQ( m->id(), tracks[0]->id() );
}

TEST_F( Genres, SearchAlbums )
{
    auto a1 = ml->createAlbum( "an album" );

    auto m = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3" ) );
    m->setType( IMedia::Type::Audio );
    auto t = a1->addTrack( m, 1, 1, 0, g.get() );
    m->save();

    auto a2 = ml->createAlbum( "another album" );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "Different genre Hell's Kitchen.mp3" ) );
    m2->setType( IMedia::Type::Audio );
    auto t2 = a2->addTrack( m2, 1, 1, 0, nullptr );
    m2->save();

    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "track3.mp3" ) );
    m3->setType( IMedia::Type::Audio );
    auto t3 = a1->addTrack( m3, 2, 1, 0, g.get() );
    m3->save();

    auto query = ml->searchAlbums( "album", nullptr);
    ASSERT_EQ( 2u, query->count() );
    auto albums = query->all();
    ASSERT_EQ( 2u, albums.size() );

    query = g->searchAlbums( "album", nullptr );
    ASSERT_EQ( 1u, query->count() );
    albums = query->all();
    ASSERT_EQ( 1u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
}
