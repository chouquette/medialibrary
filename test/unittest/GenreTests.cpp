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
    auto tracks = g->tracks( IGenre::TracksIncluded::All, nullptr )->all();
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
        auto m = std::static_pointer_cast<Media>(
                    ml->addMedia( "track" + std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto t = a->addTrack( m, i, 1, 0, i != 1 ? g.get() : nullptr );
    }
    auto tracks = g->tracks( IGenre::TracksIncluded::All, nullptr )->all();
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
        auto m = std::static_pointer_cast<Media>(
                    ml->addMedia( std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto track = album->addTrack( m, i, 1, a->id(), g.get() );
    }
    for ( auto i = 1u; i <= 5; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    ml->addMedia( std::to_string( i ) + "_2.mp3", IMedia::Type::Audio ) );
        auto track = album2->addTrack( m, i, 1, a2->id(), g.get() );
    }
    auto query = g->artists( nullptr );
    ASSERT_EQ( 2u, query->count() );
    artists = query->all();
    ASSERT_EQ( 2u, artists.size() );
}

TEST_F( Genres, ListAlbums )
{
    auto album = ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>(
                ml->addMedia( "some track.mp3", IMedia::Type::Audio ) );
    auto t = album->addTrack( m, 10, 1, 0, g.get() );

    auto album2 = ml->createAlbum( "album2" );
    m = std::static_pointer_cast<Media>(
                ml->addMedia( "some other track.mp3", IMedia::Type::Audio ) );
    t = album2->addTrack( m, 10, 1, 0, g.get() );

    // We have 2 albums with at least a song with genre "g" (as defined in SetUp)
    // Now we create more albums with "random" genre, all of them should have 1 album
    for ( auto i = 1u; i <= 5u; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    ml->addMedia( std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto g = ml->createGenre( std::to_string( i ) );
        auto track = album->addTrack( m, i, 1, 0, g.get() );
    }

    auto genres = ml->genres( nullptr )->all();
    for ( auto& genre : genres )
    {
        auto query = genre->albums( nullptr );
        auto albums = query->all();

        if ( genre->id() == g->id() )
        {
            // Initial genre with 2 albums:
            ASSERT_EQ( 2u, query->count() );
            ASSERT_EQ( 2u, albums.size() );
        }
        else
        {
            ASSERT_EQ( 1u, query->count() );
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
        auto m = std::static_pointer_cast<Media>(
                    ml->addMedia( "track" + std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto t = a->addTrack( m, i, 1, 0, g.get() );
        m->setDuration( i );
        m->setReleaseDate( 10 - i );
        m->save();
    }
    QueryParameters params { SortingCriteria::Duration, false };
    auto tracks = g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 1u, tracks[0]->albumTrack()->trackNumber() );
    ASSERT_EQ( 2u, tracks[1]->albumTrack()->trackNumber() );

    params.desc = true;
    tracks = g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 1u, tracks[1]->albumTrack()->trackNumber() );
    ASSERT_EQ( 2u, tracks[0]->albumTrack()->trackNumber() );

    params.sort = SortingCriteria::ReleaseDate;
    tracks = g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 9u, tracks[0]->releaseDate() );
    ASSERT_EQ( 8u, tracks[1]->releaseDate() );

    params.desc = false;
    tracks = g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 8u, tracks[0]->releaseDate() );
    ASSERT_EQ( 9u, tracks[1]->releaseDate() );

    params.sort = SortingCriteria::Alpha;
    tracks = g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( "track1.mp3", tracks[0]->title() );
    ASSERT_EQ( "track2.mp3", tracks[1]->title() );

    params.desc = true;
    tracks = g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( "track2.mp3", tracks[0]->title() );
    ASSERT_EQ( "track1.mp3", tracks[1]->title() );
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
    auto m = std::static_pointer_cast<Media>(
                ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    auto t = a->addTrack( m, 1, 1, g->id(), g.get() );

    ASSERT_EQ( 1u, g->nbTracks() );
    Reload();
    g = std::static_pointer_cast<Genre>( ml->genre( g->id() ) );
    ASSERT_EQ( 1u, g->nbTracks() );

    ml->deleteMedia( m->id() );

    Reload();

    g = std::static_pointer_cast<Genre>( ml->genre( g->id() ) );
    ASSERT_EQ( nullptr, g );
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
        auto m = std::static_pointer_cast<Media>(
                    ml->addMedia( std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto track = album->addTrack( m, i, 1, a->id(), g.get() );
        a->addMedia( *m );
    }
    for ( auto i = 1u; i <= 5; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    ml->addMedia( std::to_string( i ) + "_2.mp3", IMedia::Type::Audio ) );
        auto track = album2->addTrack( m, i, 1, a2->id(), nullptr );
        a2->addMedia( *m );
    }
    artists = ml->searchArtists( "loutre", ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );

    artists = g->searchArtists( "loutre" )->all();
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( a->id(), artists[0]->id() );
}

TEST_F( Genres, SearchTracks )
{
    auto a = ml->createAlbum( "album" );

    auto m = std::static_pointer_cast<Media>( ml->addMedia( "Hell's Kitchen.mp3", IMedia::Type::Audio ) );
    auto t = a->addTrack( m, 1, 1, 0, g.get() );
    m->save();

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "Different genre Hell's Kitchen.mp3", IMedia::Type::Audio ) );
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

    auto m = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto t = a1->addTrack( m, 1, 1, 0, g.get() );
    m->save();

    auto a2 = ml->createAlbum( "another album" );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "Different genre Hell's Kitchen.mp3", IMedia::Type::Audio ) );
    auto t2 = a2->addTrack( m2, 1, 1, 0, nullptr );
    m2->save();

    auto a3 = ml->createAlbum( "another album" );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "track3.mp3", IMedia::Type::Audio ) );
    auto t3 = a3->addTrack( m3, 1, 1, 0, g.get() );
    m3->save();

    auto query = ml->searchAlbums( "album", nullptr);
    ASSERT_EQ( 3u, query->count() );
    auto albums = query->all();
    ASSERT_EQ( 3u, albums.size() );

    query = g->searchAlbums( "album", nullptr );
    ASSERT_EQ( 2u, query->count() );
    albums = query->all();
    ASSERT_EQ( 2u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
}

TEST_F( Genres, WithThumbnail )
{
    auto a1 = ml->createAlbum( "an album" );

    auto m = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto t = a1->addTrack( m, 1, 1, 0, g.get() );
    m->save();
    m->setThumbnail( "file:///path/to/thumbnail.png", ThumbnailSizeType::Thumbnail );

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    auto t2 = a1->addTrack( m2, 1, 1, 0, g.get() );
    m2->save();

    auto tracks = g->tracks( IGenre::TracksIncluded::WithThumbnailOnly, nullptr );
    ASSERT_EQ( 1u, tracks->count() );
    ASSERT_EQ( 1u, tracks->all().size() );

    tracks = g->tracks( IGenre::TracksIncluded::All, nullptr );
    ASSERT_EQ( 2u, tracks->count() );
    ASSERT_EQ( 2u, tracks->all().size() );
}

TEST_F( Genres, CheckDbModel )
{
    auto res = Genre::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( Genres, Thumbnails )
{
    ASSERT_FALSE( g->hasThumbnail( ThumbnailSizeType::Thumbnail ) );
    ASSERT_FALSE( g->hasThumbnail( ThumbnailSizeType::Banner ) );

    auto mrl = std::string{ "file:///path/to/thumbnail.jpg" };
    auto res = g->setThumbnail( mrl, ThumbnailSizeType::Thumbnail, false );
    ASSERT_TRUE( res );
    ASSERT_TRUE( g->hasThumbnail( ThumbnailSizeType::Thumbnail ) );

    ASSERT_EQ( mrl, g->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );

    Reload();

    g = std::static_pointer_cast<Genre>( ml->genre( g->id() ) );
    ASSERT_EQ( mrl, g->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );
    ASSERT_TRUE( g->hasThumbnail( ThumbnailSizeType::Thumbnail ) );

    // Update it, and expect the thumbnail to be updated, ie. no new thumbnail
    // should be created
    mrl = "file:///path/to/new/thumbnail.png";
    res = g->setThumbnail( mrl, ThumbnailSizeType::Thumbnail, false );
    ASSERT_TRUE( res );
    ASSERT_EQ( mrl, g->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );

    Reload();

    g = std::static_pointer_cast<Genre>( ml->genre( g->id() ) );
    ASSERT_EQ( mrl, g->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );

    ASSERT_EQ( 1u, ml->countNbThumbnails() );
}
