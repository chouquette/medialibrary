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
#include "AlbumTrack.h"
#include "Artist.h"
#include "Media.h"
#include "IMediaLibrary.h"

class Albums : public Tests
{
};

TEST_F( Albums, Create )
{
    auto a = ml->createAlbum( "album" );
    ASSERT_NE( a, nullptr );

    auto a2 = ml->album( a->id() );
    ASSERT_EQ( a, a2 );
    ASSERT_EQ( a2->title(), "album" );
}

TEST_F( Albums, Fetch )
{
    auto a = ml->createAlbum( "album" );

    // Clear the cache
    Reload();

    auto a2 = ml->album( a->id() );
    // The shared pointer are expected to point to a different instance
    ASSERT_NE( a, a2 );

    ASSERT_EQ( a->id(), a2->id() );
}

TEST_F( Albums, AddTrack )
{
    auto a = ml->createAlbum( "albumtag" );
    auto f = ml->addFile( "track.mp3", nullptr );
    auto track = a->addTrack( f, 10 );
    ASSERT_NE( track, nullptr );

    auto tracks = a->tracks();
    ASSERT_EQ( tracks.size(), 1u );

    Reload();

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    tracks = a->tracks();
    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0]->albumTrack()->trackNumber(), track->trackNumber() );
}

TEST_F( Albums, NbTracks )
{
    auto a = ml->createAlbum( "albumtag" );
    for ( auto i = 1u; i <= 10; ++i )
    {
        auto f = ml->addFile( "track" + std::to_string(i) + ".mp3", nullptr );
        auto track = a->addTrack( f, i );
        ASSERT_NE( track, nullptr );
    }
    auto tracks = a->tracks();
    ASSERT_EQ( tracks.size(), a->nbTracks() );

    Reload();

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    tracks = a->tracks();
    ASSERT_EQ( tracks.size(), a->nbTracks() );
}

TEST_F( Albums, SetGenre )
{
    auto a = ml->createAlbum( "album" );
    auto f = ml->addFile( "track.mp3", nullptr );
    auto t = a->addTrack( f, 1 );

    t->setGenre( "happy underground post progressive death metal" );
    ASSERT_EQ( t->genre(), "happy underground post progressive death metal" );

    Reload();

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    auto tracks = a->tracks();
    ASSERT_EQ( tracks.size(), 1u );
    auto t2 = tracks[0];
    ASSERT_EQ( t->genre(), t2->albumTrack()->genre() );
}

TEST_F( Albums, SetReleaseDate )
{
    auto a = ml->createAlbum( "album" );

    ASSERT_EQ( 0u, a->releaseYear() );

    a->setReleaseYear( 1234, false );
    ASSERT_EQ( a->releaseYear(), 1234u );

    a->setReleaseYear( 4321, false );
    // We now have conflicting dates, it should be restored to 0.
    ASSERT_EQ( 0u, a->releaseYear() );

    // Check that this is not considered initial state anymore, and that pretty much any other date will be ignored.
    a->setReleaseYear( 666, false );
    ASSERT_EQ( 0u, a->releaseYear() );

    // Now check that forcing a date actually forces it
    a->setReleaseYear( 9876, true );
    ASSERT_EQ( 9876u, a->releaseYear() );

    Reload();

    auto a2 = ml->album( a->id() );
    ASSERT_EQ( a->releaseYear(), a2->releaseYear() );
}

TEST_F( Albums, SetShortSummary )
{
    auto a = ml->createAlbum( "album" );

    a->setShortSummary( "summary" );
    ASSERT_EQ( a->shortSummary(), "summary" );

    Reload();

    auto a2 = ml->album( a->id() );
    ASSERT_EQ( a->shortSummary(), a2->shortSummary() );
}

TEST_F( Albums, SetArtworkUrl )
{
    auto a = ml->createAlbum( "album" );

    a->setArtworkUrl( "artwork" );
    ASSERT_EQ( a->artworkUrl(), "artwork" );

    Reload();

    auto a2 = ml->album( a->id() );
    ASSERT_EQ( a->artworkUrl(), a2->artworkUrl() );
}

TEST_F( Albums, FetchAlbumFromTrack )
{
    {
        auto a = ml->createAlbum( "album" );
        auto f = ml->addFile( "file.mp3", nullptr );
        auto t = a->addTrack( f, 1 );
        f->setAlbumTrack( t );
    }
    Reload();

    auto f = ml->file( "file.mp3" );
    auto t = f->albumTrack();
    auto a = t->album();
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->title(), "album" );
}

TEST_F( Albums, Artists )
{
    auto album = ml->createAlbum( "album" );
    auto artist1 = ml->createArtist( "john" );
    auto artist2 = ml->createArtist( "doe" );

    ASSERT_NE( album, nullptr );
    ASSERT_NE( artist1, nullptr );
    ASSERT_NE( artist2, nullptr );

    auto res = album->addArtist( artist1 );
    ASSERT_EQ( res, true );

    res = album->addArtist( artist2 );
    ASSERT_EQ( res, true );

    auto artists = album->artists();
    ASSERT_EQ( artists.size(), 2u );

    Reload();

    album = std::static_pointer_cast<Album>( ml->album( album->id() ) );
    artists = album->artists();
    ASSERT_EQ( album->albumArtist(), nullptr );
    ASSERT_EQ( artists.size(), 2u );
}

TEST_F( Albums, AlbumArtist )
{
    auto album = ml->createAlbum( "test" );
    ASSERT_EQ( album->albumArtist(), nullptr );
    auto artist = ml->createArtist( "artist" );
    album->setAlbumArtist( artist.get() );
    ASSERT_NE( album->albumArtist(), nullptr );

    Reload();

    album = std::static_pointer_cast<Album>( ml->album( album->id() ) );
    auto albumArtist = album->albumArtist();
    ASSERT_NE( albumArtist, nullptr );
    ASSERT_EQ( albumArtist->name(), artist->name() );
}
