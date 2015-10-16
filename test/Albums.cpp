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
#include "File.h"
#include "IMediaLibrary.h"

class Albums : public Tests
{
};

TEST_F( Albums, Create )
{
    auto a = ml->createAlbum( "album" );
    ASSERT_NE( a, nullptr );

    auto a2 = ml->album( "album" );
    ASSERT_EQ( a, a2 );
    ASSERT_EQ( a2->title(), "album" );
}

TEST_F( Albums, Fetch )
{
    auto a = ml->createAlbum( "album" );

    // Clear the cache
    Reload();

    auto a2 = ml->album( "album" );
    // The shared pointer are expected to point to a different instance
    ASSERT_NE( a, a2 );

    ASSERT_EQ( a->id(), a2->id() );
}

TEST_F( Albums, AddTrack )
{
    auto a = ml->createAlbum( "albumtag" );
    auto track = a->addTrack( "track", 10 );
    ASSERT_NE( track, nullptr );

    auto tracks = a->tracks();
    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0], track );

    Reload();

    a = std::static_pointer_cast<Album>( ml->album( "albumtag" ) );
    tracks = a->tracks();
    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0]->title(), track->title() );
}

TEST_F( Albums, AssignTrack )
{
    auto f = ml->addFile( "file.avi", nullptr );
    auto a = ml->createAlbum( "album" );
    auto t = a->addTrack( "track", 1 );

    ASSERT_EQ( f->albumTrack(), nullptr );
    bool res = f->setAlbumTrack( t );
    ASSERT_TRUE( res );
    ASSERT_NE( f->albumTrack(), nullptr );
    ASSERT_EQ( f->albumTrack(), t );

    Reload();

    f = std::static_pointer_cast<File>( ml->file( "file.avi" ) );
    t = std::static_pointer_cast<AlbumTrack>( f->albumTrack() );
    ASSERT_NE( t, nullptr );
    ASSERT_EQ( t->title(), "track" );
}

TEST_F( Albums, DeleteTrack )
{
    auto f = ml->addFile( "file.avi", nullptr );
    auto a = ml->createAlbum( "album" );
    auto t = a->addTrack( "track", 1 );
    f->setAlbumTrack( t );

    bool res = t->destroy();
    ASSERT_TRUE( res );

    auto f2 = ml->file( "file.avi" );
    ASSERT_EQ( f2, nullptr );
}

TEST_F( Albums, SetGenre )
{
    auto a = ml->createAlbum( "album" );
    auto t = a->addTrack( "track", 1 );

    t->setGenre( "happy underground post progressive death metal" );
    ASSERT_EQ( t->genre(), "happy underground post progressive death metal" );

    Reload();

    a = std::static_pointer_cast<Album>( ml->album( "album" ) );
    auto tracks = a->tracks();
    auto t2 = tracks[0];
    ASSERT_EQ( t->genre(), t2->genre() );
}

TEST_F( Albums, SetReleaseDate )
{
    auto a = ml->createAlbum( "album" );

    a->setReleaseDate( 1234 );
    ASSERT_EQ( a->releaseDate(), 1234 );

    Reload();

    auto a2 = ml->album( "album" );
    ASSERT_EQ( a->releaseDate(), a2->releaseDate() );
}

TEST_F( Albums, SetShortSummary )
{
    auto a = ml->createAlbum( "album" );

    a->setShortSummary( "summary" );
    ASSERT_EQ( a->shortSummary(), "summary" );

    Reload();

    auto a2 = ml->album( "album" );
    ASSERT_EQ( a->shortSummary(), a2->shortSummary() );
}

TEST_F( Albums, SetArtworkUrl )
{
    auto a = ml->createAlbum( "album" );

    a->setArtworkUrl( "artwork" );
    ASSERT_EQ( a->artworkUrl(), "artwork" );

    Reload();

    auto a2 = ml->album( "album" );
    ASSERT_EQ( a->artworkUrl(), a2->artworkUrl() );
}

TEST_F( Albums, FetchAlbumFromTrack )
{
    {
        auto a = ml->createAlbum( "album" );
        auto f = ml->addFile( "file.avi", nullptr );
        auto t = a->addTrack( "track 1", 1 );
        f->setAlbumTrack( t );
    }
    Reload();

    auto f = ml->file( "file.avi" );
    auto t = f->albumTrack();
    auto a = t->album();
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->title(), "album" );
}

TEST_F( Albums, DestroyAlbum )
{
    auto a = ml->createAlbum( "album" );
    auto f = ml->addFile( "file.avi", nullptr );
    auto t = a->addTrack( "track 1", 1 );
    f->setAlbumTrack( t );

    bool res = a->destroy();
    ASSERT_TRUE( res );

    f = std::static_pointer_cast<File>( ml->file( "file.avi" ) );
    ASSERT_EQ( f, nullptr );
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

    album = std::static_pointer_cast<Album>( ml->album( "album" ) );
    artists = album->artists();
    ASSERT_EQ( artists.size(), 2u );
}
