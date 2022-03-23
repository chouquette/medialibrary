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

#include "Genre.h"
#include "Album.h"
#include "Media.h"
#include "Artist.h"

struct GenreTests : public Tests
{
    std::shared_ptr<Genre> g;

    virtual void TestSpecificSetup() override
    {
        g = ml->createGenre( "genre" );
    }
};

static void Create( GenreTests* T )
{
    ASSERT_NE( nullptr, T->g );
    ASSERT_EQ( "genre", T->g->name() );
    auto tracks = T->g->tracks( IGenre::TracksIncluded::All, nullptr )->all();
    ASSERT_EQ( 0u, tracks.size() );
    ASSERT_EQ( 0u, T->g->nbPresentTracks() );
}

static void List( GenreTests* T )
{
    auto g2 = T->ml->createGenre( "genre 2" );
    ASSERT_NE( nullptr, g2 );
    auto genres = T->ml->genres( nullptr )->all();
    ASSERT_EQ( 2u, genres.size() );
}

static void ListAlbumTracks( GenreTests* T )
{
    auto a = T->ml->createAlbum( "album" );

    for ( auto i = 1u; i <= 3; i++ )
    {
        auto m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "track" + std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto res = a->addTrack( m, i, 1, 0, i != 1 ? T->g.get() : nullptr );
        ASSERT_TRUE( res );
    }
    auto tracks = T->g->tracks( IGenre::TracksIncluded::All, nullptr )->all();
    ASSERT_EQ( 2u, tracks.size() );

    T->g = std::static_pointer_cast<Genre>( T->ml->genre( T->g->id() ) );
    ASSERT_EQ( 2u, T->g->nbTracks() );
    ASSERT_NE( 0u, T->g->nbPresentTracks() );
}

static void ListArtists( GenreTests* T )
{
    auto artists = T->g->artists( nullptr )->all();
    ASSERT_EQ( 0u, artists.size() );

    auto a = T->ml->createArtist( "artist" );
    auto a2 = T->ml->createArtist( "artist 2" );
    // Ensure we're not just returning all the artists:
    auto a3 = T->ml->createArtist( "artist 3" );
    ASSERT_NON_NULL( a );
    ASSERT_NON_NULL( a2 );
    ASSERT_NON_NULL( a3 );
    auto album = T->ml->createAlbum( "album" );
    auto album2 = T->ml->createAlbum( "album2" );

    for ( auto i = 1u; i <= 5; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto res = album->addTrack( m, i, 1, a->id(), T->g.get() );
        ASSERT_TRUE( res );
    }
    for ( auto i = 1u; i <= 5; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( std::to_string( i ) + "_2.mp3", IMedia::Type::Audio ) );
        auto res = album2->addTrack( m, i, 1, a2->id(), T->g.get() );
        ASSERT_TRUE( res );
    }
    auto query = T->g->artists( nullptr );
    ASSERT_EQ( 2u, query->count() );
    artists = query->all();
    ASSERT_EQ( 2u, artists.size() );
}

static void ListAlbums( GenreTests* T )
{
    auto album = T->ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>(
                T->ml->addMedia( "some track.mp3", IMedia::Type::Audio ) );
    auto res = album->addTrack( m, 10, 1, 0, T->g.get() );
    ASSERT_TRUE( res );

    auto album2 = T->ml->createAlbum( "album2" );
    m = std::static_pointer_cast<Media>(
                T->ml->addMedia( "some other track.mp3", IMedia::Type::Audio ) );
    res = album2->addTrack( m, 10, 1, 0, T->g.get() );
    ASSERT_TRUE( res );

    // We have 2 albums with at least a song with genre "T->g" (as defined in SetUp)
    // Now we create more albums with "random" genre, all of them should have 1 album
    for ( auto i = 1u; i <= 5u; ++i )
    {
        auto media = std::static_pointer_cast<Media>(
                    T->ml->addMedia( std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto g = T->ml->createGenre( std::to_string( i ) );
        res = album->addTrack( media, i, 1, 0, g.get() );
        ASSERT_TRUE( res );
    }

    auto genres = T->ml->genres( nullptr )->all();
    for ( auto& genre : genres )
    {
        auto query = genre->albums( nullptr );
        auto albums = query->all();

        if ( genre->id() == T->g->id() )
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

static void Search( GenreTests* T )
{
    T->ml->createGenre( "something" );
    T->ml->createGenre( "blork" );

    auto genres = T->ml->searchGenre( "genr", nullptr )->all();
    ASSERT_EQ( 1u, genres.size() );
}

static void SearchAfterDelete( GenreTests* T )
{
    auto genres = T->ml->searchGenre( "genre", nullptr )->all();
    ASSERT_EQ( 1u, genres.size() );

    T->ml->deleteGenre( T->g->id() );

    genres = T->ml->searchGenre( "genre", nullptr )->all();
    ASSERT_EQ( 0u, genres.size() );
}

static void SortTracks( GenreTests* T )
{
    auto a = T->ml->createAlbum( "album" );

    for ( auto i = 1u; i <= 2; i++ )
    {
        auto m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "track" + std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto res = a->addTrack( m, i, 1, 0, T->g.get() );
        ASSERT_TRUE( res );
        m->setDuration( i );
        m->setReleaseDate( 10 - i );
    }
    QueryParameters params { SortingCriteria::Duration, false };
    auto tracks = T->g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 1u, tracks[0]->trackNumber() );
    ASSERT_EQ( 2u, tracks[1]->trackNumber() );

    params.desc = true;
    tracks = T->g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 1u, tracks[1]->trackNumber() );
    ASSERT_EQ( 2u, tracks[0]->trackNumber() );

    params.sort = SortingCriteria::ReleaseDate;
    tracks = T->g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 9u, tracks[0]->releaseDate() );
    ASSERT_EQ( 8u, tracks[1]->releaseDate() );

    params.desc = false;
    tracks = T->g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( 8u, tracks[0]->releaseDate() );
    ASSERT_EQ( 9u, tracks[1]->releaseDate() );

    params.sort = SortingCriteria::Alpha;
    tracks = T->g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( "track1.mp3", tracks[0]->title() );
    ASSERT_EQ( "track2.mp3", tracks[1]->title() );

    params.desc = true;
    tracks = T->g->tracks( IGenre::TracksIncluded::All, &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( "track2.mp3", tracks[0]->title() );
    ASSERT_EQ( "track1.mp3", tracks[1]->title() );
}

static void Sort( GenreTests* T )
{
    auto g2 = T->ml->createGenre( "metal" );

    auto genres = T->ml->genres( nullptr )->all();
    ASSERT_EQ( 2u, genres.size() );
    ASSERT_EQ( T->g->id(), genres[0]->id() );
    ASSERT_EQ( g2->id(), genres[1]->id() );

    QueryParameters params { SortingCriteria::Default, true };
    genres = T->ml->genres( &params )->all();
    ASSERT_EQ( 2u, genres.size() );
    ASSERT_EQ( T->g->id(), genres[1]->id() );
    ASSERT_EQ( g2->id(), genres[0]->id() );
}

static void NbTracks( GenreTests* T )
{
    ASSERT_EQ( 0u, T->g->nbTracks() );

    auto a = T->ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>(
                T->ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );

    auto res = a->addTrack( m, 1, 1, T->g->id(), T->g.get() );
    ASSERT_TRUE( res );
    res = a->addTrack( m2, 2, 1, T->g->id(), T->g.get() );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, T->g->nbTracks() );
    T->g = std::static_pointer_cast<Genre>( T->ml->genre( T->g->id() ) );
    ASSERT_EQ( 2u, T->g->nbTracks() );
    ASSERT_EQ( 2u, T->g->nbPresentTracks() );

    T->ml->deleteMedia( m->id() );

    T->g = std::static_pointer_cast<Genre>( T->ml->genre( T->g->id() ) );
    ASSERT_EQ( 1u, T->g->nbTracks() );
    ASSERT_EQ( 1u, T->g->nbPresentTracks() );

    T->ml->deleteMedia( m2->id() );

    T->g = std::static_pointer_cast<Genre>( T->ml->genre( T->g->id() ) );
    ASSERT_EQ( nullptr, T->g );
}

static void CaseInsensitive( GenreTests* T )
{
    auto g2 = Genre::fromName( T->ml.get(), "GENRE" );
    ASSERT_EQ( T->g->id(), g2->id() );
}

static void SearchArtists( GenreTests* T )
{
    auto artists = T->g->artists( nullptr )->all();
    ASSERT_EQ( 0u, artists.size() );

    auto a = T->ml->createArtist( "loutre 1" );
    auto a2 = T->ml->createArtist( "loutre 2" );
    auto a3 = T->ml->createArtist( "loutre 3" );
    auto album = T->ml->createAlbum( "album" );
    auto album2 = T->ml->createAlbum( "album2" );

    for ( auto i = 1u; i <= 5; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        auto res = album->addTrack( m, i, 1, a->id(), T->g.get() );
        ASSERT_TRUE( res );
        a->addMedia( *m );

        m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "dup_" + std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        res = album->addTrack( m, i, 1, a3->id(), T->g.get() );
        ASSERT_TRUE( res );
        a3->addMedia( *m );
    }
    for ( auto i = 1u; i <= 5; ++i )
    {
        auto m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( std::to_string( i ) + "_2.mp3", IMedia::Type::Audio ) );
        auto res = album2->addTrack( m, i, 1, a2->id(), nullptr );
        ASSERT_TRUE( res );
        a2->addMedia( *m );
    }
    artists = T->ml->searchArtists( "loutre", ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 3u, artists.size() );

    artists = T->g->searchArtists( "loutre" )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( a->id(), artists[0]->id() );
    ASSERT_EQ( a3->id(), artists[1]->id() );

    QueryParameters params{};
    params.sort = SortingCriteria::Alpha;
    params.desc = true;
    artists = T->g->searchArtists( "loutre", &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( a3->id(), artists[0]->id() );
    ASSERT_EQ( a->id(), artists[1]->id() );

}

static void SearchTracks( GenreTests* T )
{
    auto a = T->ml->createAlbum( "album" );

    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "Hell's Kitchen.mp3", IMedia::Type::Audio ) );
    auto res = a->addTrack( m, 1, 1, 0, T->g.get() );
    ASSERT_TRUE( res );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "Different genre Hell's Kitchen.mp3", IMedia::Type::Audio ) );
    res = a->addTrack( m2, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );

    auto tracks = T->ml->searchAudio( "kitchen", nullptr )->all();
    ASSERT_EQ( 2u, tracks.size() );

    tracks = T->g->searchTracks( "kitchen", nullptr )->all();
    ASSERT_EQ( 1u, tracks.size() );
    ASSERT_EQ( m->id(), tracks[0]->id() );
}

static void SearchAlbums( GenreTests* T )
{
    auto a1 = T->ml->createAlbum( "an album" );

    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto res = a1->addTrack( m, 1, 1, 0, T->g.get() );
    ASSERT_TRUE( res );

    auto a2 = T->ml->createAlbum( "another album" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "Different genre Hell's Kitchen.mp3", IMedia::Type::Audio ) );
    res = a2->addTrack( m2, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );

    auto a3 = T->ml->createAlbum( "another album" );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "track3.mp3", IMedia::Type::Audio ) );
    res = a3->addTrack( m3, 1, 1, 0, T->g.get() );
    ASSERT_TRUE( res );

    auto query = T->ml->searchAlbums( "album", nullptr);
    ASSERT_EQ( 3u, query->count() );
    auto albums = query->all();
    ASSERT_EQ( 3u, albums.size() );

    query = T->g->searchAlbums( "album", nullptr );
    ASSERT_EQ( 2u, query->count() );
    albums = query->all();
    ASSERT_EQ( 2u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
}

static void WithThumbnail( GenreTests* T )
{
    auto a1 = T->ml->createAlbum( "an album" );

    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto res = a1->addTrack( m, 1, 1, 0, T->g.get() );
    ASSERT_TRUE( res );
    m->setThumbnail( "file:///path/to/thumbnail.png", ThumbnailSizeType::Thumbnail );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    res = a1->addTrack( m2, 1, 1, 0, T->g.get() );
    ASSERT_TRUE( res );

    auto tracks = T->g->tracks( IGenre::TracksIncluded::WithThumbnailOnly, nullptr );
    ASSERT_EQ( 1u, tracks->count() );
    ASSERT_EQ( 1u, tracks->all().size() );

    tracks = T->g->tracks( IGenre::TracksIncluded::All, nullptr );
    ASSERT_EQ( 2u, tracks->count() );
    ASSERT_EQ( 2u, tracks->all().size() );
}

static void CheckDbModel( GenreTests* T )
{
    auto res = Genre::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void GetThumbnails( GenreTests* T )
{
    ASSERT_FALSE( T->g->hasThumbnail( ThumbnailSizeType::Thumbnail ) );
    ASSERT_FALSE( T->g->hasThumbnail( ThumbnailSizeType::Banner ) );

    auto mrl = std::string{ "file:///path/to/thumbnail.jpg" };
    auto res = T->g->setThumbnail( mrl, ThumbnailSizeType::Thumbnail, false );
    ASSERT_TRUE( res );
    ASSERT_TRUE( T->g->hasThumbnail( ThumbnailSizeType::Thumbnail ) );

    ASSERT_EQ( mrl, T->g->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );

    T->g = std::static_pointer_cast<Genre>( T->ml->genre( T->g->id() ) );
    ASSERT_EQ( mrl, T->g->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );
    ASSERT_TRUE( T->g->hasThumbnail( ThumbnailSizeType::Thumbnail ) );

    // Update it, and expect the thumbnail to be updated, ie. no new thumbnail
    // should be created
    mrl = "file:///path/to/new/thumbnail.png";
    res = T->g->setThumbnail( mrl, ThumbnailSizeType::Thumbnail, false );
    ASSERT_TRUE( res );
    ASSERT_EQ( mrl, T->g->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );

    T->g = std::static_pointer_cast<Genre>( T->ml->genre( T->g->id() ) );
    ASSERT_EQ( mrl, T->g->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );

    ASSERT_EQ( 1u, T->ml->countNbThumbnails() );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( GenreTests );

    ADD_TEST( Create );
    ADD_TEST( List );
    ADD_TEST( ListAlbumTracks );
    ADD_TEST( ListArtists );
    ADD_TEST( ListAlbums );
    ADD_TEST( Search );
    ADD_TEST( SearchAfterDelete );
    ADD_TEST( SortTracks );
    ADD_TEST( Sort );
    ADD_TEST( NbTracks );
    ADD_TEST( CaseInsensitive );
    ADD_TEST( SearchArtists );
    ADD_TEST( SearchTracks );
    ADD_TEST( SearchAlbums );
    ADD_TEST( WithThumbnail );
    ADD_TEST( CheckDbModel );
    ADD_TEST( GetThumbnails );

    END_TESTS
}
