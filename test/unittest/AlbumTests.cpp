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
#include "AlbumTrack.h"
#include "Artist.h"
#include "Genre.h"
#include "Media.h"
#include "medialibrary/IMediaLibrary.h"

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
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "track.mp3" ) );
    auto track = a->addTrack( f, 10, 0, 0, 0 );
    f->save();
    ASSERT_NE( track, nullptr );

    auto tracks = a->tracks( SortingCriteria::Default, false );
    ASSERT_EQ( tracks.size(), 1u );

    Reload();

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    tracks = a->tracks( SortingCriteria::Default, false );
    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0]->albumTrack()->trackNumber(), track->trackNumber() );
}

TEST_F( Albums, NbTracks )
{
    auto a = ml->createAlbum( "albumtag" );
    for ( auto i = 1u; i <= 10; ++i )
    {
        auto f = std::static_pointer_cast<Media>( ml->addMedia( "track" + std::to_string(i) + ".mp3" ) );
        auto track = a->addTrack( f, i, i, 0, 0 );
        f->save();
        ASSERT_NE( track, nullptr );
    }
    auto tracks = a->tracks( SortingCriteria::Default, false );
    ASSERT_EQ( tracks.size(), a->nbTracks() );

    Reload();

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    tracks = a->tracks( SortingCriteria::Default, false );
    ASSERT_EQ( tracks.size(), a->nbTracks() );
}

TEST_F( Albums, TracksByGenre )
{
    auto a = ml->createAlbum( "albumtag" );
    auto g = ml->createGenre( "genre" );

    for ( auto i = 1u; i <= 10; ++i )
    {
        auto f = std::static_pointer_cast<Media>( ml->addMedia( "track" + std::to_string(i) + ".mp3" ) );
        auto track = a->addTrack( f, i, i, 0, 0 );
        f->save();
        ASSERT_NE( track, nullptr );
        if ( i <= 5 )
            track->setGenre( g );
    }
    auto tracks = a->tracks( g, SortingCriteria::Default, false );
    ASSERT_EQ( 5u, tracks.size() );

    Reload();

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    tracks = a->tracks( g, SortingCriteria::Default, false );
    ASSERT_NE( tracks.size(), a->nbTracks() );
    ASSERT_EQ( 5u, tracks.size() );
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

TEST_F( Albums, SetArtworkMrl )
{
    auto a = ml->createAlbum( "album" );

    a->setArtworkMrl( "artwork" );
    ASSERT_EQ( a->artworkMrl(), "artwork" );

    Reload();

    auto a2 = ml->album( a->id() );
    ASSERT_EQ( a->artworkMrl(), a2->artworkMrl() );
}

TEST_F( Albums, FetchAlbumFromTrack )
{
    auto a = ml->createAlbum( "album" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.mp3" ) );
    auto t = a->addTrack( f, 1, 0, 0, 0 );
    f->save();

    Reload();

    f = ml->media( f->id() );
    auto t2 = f->albumTrack();
    auto a2 = t2->album();
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->title(), "album" );
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

    auto artists = album->artists( false );
    ASSERT_EQ( artists.size(), 2u );

    Reload();

    album = std::static_pointer_cast<Album>( ml->album( album->id() ) );
    artists = album->artists( false );
    ASSERT_EQ( album->albumArtist(), nullptr );
    ASSERT_EQ( artists.size(), 2u );
}

TEST_F( Albums, SortArtists )
{
    auto album = ml->createAlbum( "album" );
    auto artist1 = ml->createArtist( "john" );
    auto artist2 = ml->createArtist( "doe" );

    album->addArtist( artist1 );
    album->addArtist( artist2 );

    auto artists = album->artists( false );
    ASSERT_EQ( artists.size(), 2u );
    ASSERT_EQ( artist1->id(), artists[1]->id() );
    ASSERT_EQ( artist2->id(), artists[0]->id() );

    artists = album->artists( true );
    ASSERT_EQ( artists.size(), 2u );
    ASSERT_EQ( artist1->id(), artists[0]->id() );
    ASSERT_EQ( artist2->id(), artists[1]->id() );
}

TEST_F( Albums, AlbumArtist )
{
    auto album = ml->createAlbum( "test" );
    ASSERT_EQ( album->albumArtist(), nullptr );
    auto artist = ml->createArtist( "artist" );
    album->setAlbumArtist( artist );
    ASSERT_NE( album->albumArtist(), nullptr );

    Reload();

    album = std::static_pointer_cast<Album>( ml->album( album->id() ) );
    auto albumArtist = album->albumArtist();
    ASSERT_NE( albumArtist, nullptr );
    ASSERT_EQ( albumArtist->name(), artist->name() );
}

TEST_F( Albums, SearchByTitle )
{
    ml->createAlbum( "sea otters" );
    ml->createAlbum( "pangolins of fire" );

    auto albums = ml->searchAlbums( "otte" );
    ASSERT_EQ( 1u, albums.size() );
}

TEST_F( Albums, SearchByArtist )
{
    auto a = ml->createAlbum( "sea otters" );
    auto artist = ml->createArtist( "pangolins" );
    a->setAlbumArtist( artist );

    auto albums = ml->searchAlbums( "pangol" );
    ASSERT_EQ( 1u, albums.size() );
}

TEST_F( Albums, SearchNoDuplicate )
{
    auto a = ml->createAlbum( "sea otters" );
    auto artist = ml->createArtist( "otters" );
    a->setAlbumArtist( artist );

    auto albums = ml->searchAlbums( "otters" );
    ASSERT_EQ( 1u, albums.size() );
}

TEST_F( Albums, SearchNoUnknownAlbum )
{
    auto artist = ml->createArtist( "otters" );
    auto album = artist->unknownAlbum();
    ASSERT_NE( nullptr, album );

    auto albums = ml->searchAlbums( "otters" );
    ASSERT_EQ( 0u, albums.size() );
    // Can't search by name since there is no name set for unknown albums
}

TEST_F( Albums, SearchAfterDeletion )
{
    auto a = ml->createAlbum( "sea otters" );
    auto albums = ml->searchAlbums( "sea" );
    ASSERT_EQ( 1u, albums.size() );

    ml->deleteAlbum( a->id() );

    albums = ml->searchAlbums( "sea" );
    ASSERT_EQ( 0u, albums.size() );
}

TEST_F( Albums, SearchAfterArtistUpdate )
{
    auto a = ml->createAlbum( "sea otters" );
    auto artist = ml->createArtist( "pangolin of fire" );
    auto artist2 = ml->createArtist( "pangolin of ice" );
    a->setAlbumArtist( artist );

    auto albums = ml->searchAlbums( "fire" );
    ASSERT_EQ( 1u, albums.size() );

    albums = ml->searchAlbums( "ice" );
    ASSERT_EQ( 0u, albums.size() );

    a->setAlbumArtist( artist2 );

    albums = ml->searchAlbums( "fire" );
    ASSERT_EQ( 0u, albums.size() );

    albums = ml->searchAlbums( "ice" );
    ASSERT_EQ( 1u, albums.size() );
}

TEST_F( Albums, AutoDelete )
{
    auto a = ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3" ) );
    auto t = a->addTrack( m, 1, 1, 0, 0 );

    auto album = ml->album( a->id() );
    ASSERT_NE( nullptr, album );

    ml->deleteTrack( t->id() );

    album = ml->album( a->id() );
    ASSERT_EQ( nullptr, album );
}

TEST_F( Albums, SortTracks )
{
    auto a = ml->createAlbum( "album" );
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "B-track1.mp3" ) );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "A-track2.mp3" ) );
    auto t1 = a->addTrack( m1, 1, 1, 0, 0 );
    auto t2 = a->addTrack( m2, 2, 1, 0, 0 );

    // Default order is by disc number & track number
    auto tracks = a->tracks( SortingCriteria::Default, false );
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( t1->id(), tracks[0]->id() );
    ASSERT_EQ( t2->id(), tracks[1]->id() );

    // Reverse order
    tracks = a->tracks( SortingCriteria::Default, true );
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( t1->id(), tracks[1]->id() );
    ASSERT_EQ( t2->id(), tracks[0]->id() );

    // Try a media based criteria
    tracks = a->tracks( SortingCriteria::Alpha, false );
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( t1->id(), tracks[1]->id() ); // B-track -> first
    ASSERT_EQ( t2->id(), tracks[0]->id() ); // A-track -> second
}

TEST_F( Albums, Sort )
{
    auto a1 = ml->createAlbum( "A" );
    a1->setReleaseYear( 1000, false );
    auto a2 = ml->createAlbum( "B" );
    a2->setReleaseYear( 2000, false );
    auto a3 = ml->createAlbum( "C" );
    a3->setReleaseYear( 1000, false );

    auto albums = ml->albums( SortingCriteria::ReleaseDate, false );
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
    ASSERT_EQ( a3->id(), albums[1]->id() );
    ASSERT_EQ( a2->id(), albums[2]->id() );

    albums = ml->albums( SortingCriteria::ReleaseDate, true );
    // We do not invert the lexical order when sorting by DESC release date:
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a2->id(), albums[0]->id() );
    ASSERT_EQ( a1->id(), albums[1]->id() );
    ASSERT_EQ( a3->id(), albums[2]->id() );

    // When listing all albums, default order is lexical order
    albums = ml->albums( SortingCriteria::Default, false );
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a3->id(), albums[2]->id() );

    albums = ml->albums( SortingCriteria::Default, true );
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a3->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a1->id(), albums[2]->id() );
}

TEST_F( Albums, SortByArtist )
{
    auto artist1 = ml->createArtist( "Artist" );
    auto artist2 = ml->createArtist( "tsitrA" );

    // Create albums with a non-alphabetical order to avoid a false positive (where sorting by pkey
    // is the same as sorting by title)
    auto a1 = ml->createAlbum( "C" );
    a1->setAlbumArtist( artist1 );
    auto a2 = ml->createAlbum( "B" );
    a2->setAlbumArtist( artist2 );
    auto a3 = ml->createAlbum( "A" );
    a3->setAlbumArtist( artist1 );

    auto albums = ml->albums( SortingCriteria::Artist, false );
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a3->id(), albums[0]->id() );
    ASSERT_EQ( a1->id(), albums[1]->id() );
    ASSERT_EQ( a2->id(), albums[2]->id() );

    albums = ml->albums( SortingCriteria::Artist, true );
    ASSERT_EQ( 3u, albums.size() );
    // We expect Artist to be sorted in reverse order, but still in alphabetical order for albums
    ASSERT_EQ( a2->id(), albums[0]->id() );
    ASSERT_EQ( a3->id(), albums[1]->id() );
    ASSERT_EQ( a1->id(), albums[2]->id() );
}

TEST_F( Albums, Duration )
{
    auto a = ml->createAlbum( "album" );
    ASSERT_EQ( 0u, a->duration() );

    auto m = std::static_pointer_cast<Media>( ml->addMedia( "track.mp3" ) );
    m->setDuration( 100 );
    m->save();
    a->addTrack( m, 1, 1, 0, 0 );
    ASSERT_EQ( 100u, a->duration() );

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3" ) );
    m2->setDuration( 200 );
    m2->save();
    a->addTrack( m2, 1, 1, 0, 0 );
    ASSERT_EQ( 300u, a->duration() );

    Reload();

    auto a2 = ml->album( a->id() );
    ASSERT_EQ( 300u, a2->duration() );
}
