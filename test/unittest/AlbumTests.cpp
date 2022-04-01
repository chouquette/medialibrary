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

#include "Album.h"
#include "Artist.h"
#include "Genre.h"
#include "Media.h"
#include "medialibrary/IMediaLibrary.h"

static void Create( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );
    ASSERT_NE( a, nullptr );

    auto a2 = T->ml->album( a->id() );
    ASSERT_EQ( a2->title(), "album" );
}

static void Fetch( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );

    auto a2 = T->ml->album( a->id() );
    // The shared pointer are expected to point to a different instance
    ASSERT_NE( a, a2 );

    ASSERT_EQ( a->id(), a2->id() );
}

static void AddTrack( Tests* T )
{
    auto a = T->ml->createAlbum( "albumtag" );
    auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    auto res = a->addTrack( f, 10, 0, 0, nullptr );
    ASSERT_TRUE( res );

    auto tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), 1u );

    a = std::static_pointer_cast<Album>( T->ml->album( a->id() ) );
    tracks = a->tracks( nullptr )->all();    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0]->trackNumber(), f->trackNumber() );
}

static void RemoveTrack( Tests* T )
{
    auto a = T->ml->createAlbum( "albumtag" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    auto res = a->addTrack( m, 10, 0, 0, nullptr );
    ASSERT_TRUE( res );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    res = a->addTrack( m2, 11, 0, 0, nullptr );
    ASSERT_TRUE( res );

    auto tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), 2u );

    res = a->removeTrack( *m2 );
    ASSERT_TRUE( res );
    res = a->removeTrack( *m );
    ASSERT_TRUE( res );
}

static void NbTrack( Tests* T )
{
    auto a = T->ml->createAlbum( "albumtag" );
    for ( auto i = 1u; i <= 10; ++i )
    {
        auto f = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "track" + std::to_string(i) + ".mp3", IMedia::Type::Audio ) );
        auto res = a->addTrack( f, i, i, 0, nullptr );
        ASSERT_TRUE( res );
    }
    auto tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), a->nbTracks() );

    a = std::static_pointer_cast<Album>( T->ml->album( a->id() ) );
    tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), a->nbTracks() );
}

static void TracksByGenre( Tests* T )
{
    auto a = T->ml->createAlbum( "albumtag" );
    auto g = T->ml->createGenre( "genre" );

    for ( auto i = 1u; i <= 10; ++i )
    {
        auto f = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "track" + std::to_string(i) + ".mp3", IMedia::Type::Audio ) );
        auto res = a->addTrack( f, i, i, 0, i <= 5 ? g.get() : nullptr );
        ASSERT_TRUE( res );
    }
    auto tracksQuery = a->tracks( nullptr, nullptr );
    ASSERT_EQ( nullptr, tracksQuery );
    tracksQuery = a->tracks( g, nullptr );
    ASSERT_EQ( 5u, tracksQuery->count() );
    auto tracks = tracksQuery->all();
    ASSERT_EQ( 5u, tracks.size() );

    a = std::static_pointer_cast<Album>( T->ml->album( a->id() ) );
    tracks = a->tracks( g, nullptr )->all();
    ASSERT_NE( tracks.size(), a->nbTracks() );
    ASSERT_EQ( 5u, tracks.size() );
}

static void SetReleaseDate( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );

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

    auto a2 = T->ml->album( a->id() );
    ASSERT_EQ( a->releaseYear(), a2->releaseYear() );
}

static void SetShortSummary( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );

    a->setShortSummary( "summary" );
    ASSERT_EQ( a->shortSummary(), "summary" );

    auto a2 = T->ml->album( a->id() );
    ASSERT_EQ( a->shortSummary(), a2->shortSummary() );
}

static void GetThumbnail( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );
    auto t = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( nullptr, t );
    ASSERT_EQ( ThumbnailStatus::Missing,
               a->thumbnailStatus( ThumbnailSizeType::Thumbnail ) );

    std::string mrl = "file:///path/to/sea/otter/artwork.png";
    t = std::make_shared<Thumbnail>( T->ml.get(), mrl, Thumbnail::Origin::UserProvided,
                                     ThumbnailSizeType::Thumbnail, false );
    auto id = t->insert();
    ASSERT_NE( 0, id );
    a = T->ml->MediaLibrary::createAlbum( "album 2" );
    a->setThumbnail( t );

    t = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NE( nullptr, t );
    ASSERT_EQ( mrl, t->mrl() );
    ASSERT_EQ( ThumbnailStatus::Available,
               a->thumbnailStatus( ThumbnailSizeType::Thumbnail ) );

    a = std::static_pointer_cast<Album>( T->ml->album( a->id() ) );
    t = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NE( nullptr, t );
    ASSERT_EQ( mrl, t->mrl() );
    ASSERT_EQ( ThumbnailStatus::Available,
               a->thumbnailStatus( ThumbnailSizeType::Thumbnail ) );
}

static void FetchAlbumFromTrack( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );
    auto f = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file.mp3", IMedia::Type::Audio ) );
    auto res = a->addTrack( f, 1, 0, 0, nullptr );
    ASSERT_TRUE( res );

    f = T->ml->media( f->id() );
    ASSERT_NON_NULL( f );
    auto a2 = f->album();
    ASSERT_EQ( a2->title(), "album" );
}

static void Artists( Tests* T )
{
    auto album = T->ml->createAlbum( "album" );
    auto artist1 = T->ml->createArtist( "john" );
    auto artist2 = T->ml->createArtist( "doe" );

    ASSERT_NE( album, nullptr );
    ASSERT_NE( artist1, nullptr );
    ASSERT_NE( artist2, nullptr );

    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    album->addTrack( m1, 1, 0, artist1->id(), nullptr );

    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    album->addTrack( m2, 2, 0, artist2->id(), nullptr );

    QueryParameters params { SortingCriteria::Default, false };
    auto query = album->artists( &params );
    ASSERT_EQ( 2u, query->count() );
    auto artists = query->all();
    ASSERT_EQ( artists.size(), 2u );
    ASSERT_EQ( artist1->id(), artists[1]->id() );
    ASSERT_EQ( artist2->id(), artists[0]->id() );

    params.desc = true;
    album = std::static_pointer_cast<Album>( T->ml->album( album->id() ) );
    query = album->artists( &params );
    ASSERT_EQ( 2u, query->count() );
    artists = query->all();
    ASSERT_EQ( artists.size(), 2u );
    ASSERT_EQ( artist1->id(), artists[0]->id() );
    ASSERT_EQ( artist2->id(), artists[1]->id() );
}

static void AlbumArtist( Tests* T )
{
    auto album = T->ml->createAlbum( "test" );
    ASSERT_EQ( album->albumArtist(), nullptr );
    auto artist = T->ml->createArtist( "artist" );
    auto res = album->setAlbumArtist( artist );
    ASSERT_TRUE( res );
    // Override with the same artist, expect a success
    res = album->setAlbumArtist( artist );
    ASSERT_TRUE( res );
    auto noartist = std::make_shared<Artist>( T->ml.get(), "dummy artist" );
    ASSERT_EQ( 0, noartist->id() );
    res = album->setAlbumArtist( noartist );
    ASSERT_FALSE( res );
    ASSERT_NE( album->albumArtist(), nullptr );

    album = std::static_pointer_cast<Album>( T->ml->album( album->id() ) );
    auto albumArtist = album->albumArtist();
    ASSERT_NE( albumArtist, nullptr );
    ASSERT_EQ( albumArtist->name(), artist->name() );
}

static void SortAlbumThenArtist( Tests* T )
{
    // First
    auto albumOttersO = T->ml->createAlbum( "otters" );
    // Second
    auto albumPangolinsP = T->ml->createAlbum( "pangolins of fire" );
    // Fourth
    auto albumPangolinsS = T->ml->createAlbum( "see otters" );
    // Third
    auto albumOttersS = T->ml->createAlbum( "sea otters" );
    // Originally the medialibrary handled ordering in case of identical
    // album name by using the insertion order.
    // Here the insertion order is different than the expected sort order.

    auto artistP = T->ml->createArtist( "pangolins" );
    auto artistO = T->ml->createArtist( "otters" );

    albumOttersO->setAlbumArtist( artistO );
    albumPangolinsP->setAlbumArtist( artistP );
    albumOttersS->setAlbumArtist( artistO );
    albumPangolinsS->setAlbumArtist( artistP);

    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    albumOttersO->addTrack( m, 1, 0, 0, nullptr );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    albumPangolinsP->addTrack( m2, 1, 0, 0, nullptr );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    albumOttersS->addTrack( m3, 1, 0, 0, nullptr );
    auto m4 = std::static_pointer_cast<Media>( T->ml->addMedia( "media4.mp3", IMedia::Type::Audio ) );
    albumPangolinsS->addTrack( m4, 1, 0, 0, nullptr );

    QueryParameters params { SortingCriteria::Alpha, false };
    auto albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( albumOttersO->id(), albums[0]->id() );
    ASSERT_EQ( albumPangolinsP->id(), albums[1]->id() );
    ASSERT_EQ( albumOttersS->id(), albums[2]->id() );
    ASSERT_EQ( albumPangolinsS->id(), albums[3]->id() );

    params.desc = true;
    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( albumPangolinsS->id(), albums[0]->id() );
    ASSERT_EQ( albumOttersS->id(), albums[1]->id() );
    ASSERT_EQ( albumPangolinsP->id(), albums[2]->id() );
    ASSERT_EQ( albumOttersO->id(), albums[3]->id() );
}

static void SearchByTitle( Tests* T )
{
    auto a1 = T->ml->createAlbum( "sea otters" );
    auto a2 = T->ml->createAlbum( "pangolins of fire" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a1->addTrack( m, 1, 0, 0, nullptr );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    a2->addTrack( m2, 1, 0, 0, nullptr );

    auto albums = T->ml->searchAlbums( "otte", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
}

static void SearchByArtist( Tests* T )
{
    auto a = T->ml->createAlbum( "sea otters" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a->addTrack( m, 1, 0, 0, nullptr );
    auto artist = T->ml->createArtist( "pangolins" );
    a->setAlbumArtist( artist );

    auto albums = T->ml->searchAlbums( "pangol", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
}

static void SearchNoDuplicate( Tests* T )
{
    auto a = T->ml->createAlbum( "sea otters" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a->addTrack( m, 1, 0, 0, nullptr );
    auto artist = T->ml->createArtist( "otters" );
    a->setAlbumArtist( artist );

    auto albums = T->ml->searchAlbums( "otters", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
}

static void SearchNoUnknownAlbum( Tests* T )
{
    auto artist = T->ml->createArtist( "otters" );
    auto album = artist->createUnknownAlbum();
    ASSERT_TRUE( album->isUnknownAlbum() );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    album->addTrack( m, 1, 0, 0, nullptr );
    ASSERT_NE( nullptr, album );

    auto albums = T->ml->searchAlbums( "otters", nullptr )->all();
    ASSERT_EQ( 0u, albums.size() );
    // Can't search by name since there is no name set for unknown albums
}

static void SearchAfterDeletion( Tests* T )
{
    auto a = T->ml->createAlbum( "sea otters" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a->addTrack( m, 1, 0, 0, nullptr );
    auto albums = T->ml->searchAlbums( "sea", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );

    T->ml->deleteAlbum( a->id() );

    albums = T->ml->searchAlbums( "sea", nullptr )->all();
    ASSERT_EQ( 0u, albums.size() );
}

static void SearchAfterArtistUpdate( Tests* T )
{
    auto a = T->ml->createAlbum( "sea otters" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a->addTrack( m, 1, 0, 0, nullptr );
    auto artist = T->ml->createArtist( "pangolin of fire" );
    auto artist2 = T->ml->createArtist( "pangolin of ice" );
    a->setAlbumArtist( artist );

    auto albums = T->ml->searchAlbums( "fire", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );

    albums = T->ml->searchAlbums( "ice", nullptr )->all();
    ASSERT_EQ( 0u, albums.size() );

    a->setAlbumArtist( artist2 );

    albums = T->ml->searchAlbums( "fire", nullptr )->all();
    ASSERT_EQ( 0u, albums.size() );

    albums = T->ml->searchAlbums( "ice", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
}

static void AutoDelete( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    auto res = a->addTrack( m, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );

    auto album = T->ml->album( a->id() );
    ASSERT_NE( nullptr, album );

    T->ml->deleteMedia( m->id() );

    album = T->ml->album( a->id() );
    ASSERT_EQ( nullptr, album );
}

static void SortTracks( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "B-track1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "A-track2.mp3", IMedia::Type::Audio ) );
    auto res = a->addTrack( m1, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );
    res = a->addTrack( m2, 2, 1, 0, nullptr );
    ASSERT_TRUE( res );

    // Default order is by disc number & track number
    auto tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( m1->id(), tracks[0]->id() );
    ASSERT_EQ( m2->id(), tracks[1]->id() );

    // Reverse order
    QueryParameters params { SortingCriteria::TrackId, true };
    tracks = a->tracks( &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( m1->id(), tracks[1]->id() );
    ASSERT_EQ( m2->id(), tracks[0]->id() );

    // Try a media based criteria
    params = { SortingCriteria::Alpha, false };
    tracks = a->tracks( &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( m1->id(), tracks[1]->id() ); // B-track -> first
    ASSERT_EQ( m2->id(), tracks[0]->id() ); // A-track -> second
}

static void Sort( Tests* T )
{
    auto a1 = T->ml->createAlbum( "A" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a1->addTrack( m, 1, 0, 0, nullptr );
    a1->setReleaseYear( 1000, false );
    auto a2 = T->ml->createAlbum( "B" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    a2->addTrack( m2, 1, 0, 0, nullptr );
    a2->setReleaseYear( 2000, false );
    auto a3 = T->ml->createAlbum( "C" );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    a3->addTrack( m3, 1, 0, 0, nullptr );
    m3->setReleaseDate( 1000 );
    auto m4 = std::static_pointer_cast<Media>( T->ml->addMedia( "media4.mp3", IMedia::Type::Audio ) );
    a3->addTrack( m4, 2, 0, 0, nullptr );
    m4->setReleaseDate( 995 );
    a3->setReleaseYear( 1000, false );

    QueryParameters params { SortingCriteria::ReleaseDate, false };
    auto albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
    ASSERT_EQ( a3->id(), albums[1]->id() );
    ASSERT_EQ( a2->id(), albums[2]->id() );

    // Also try to list tracks ordered by release dates:
    auto tracksQuery = a3->tracks( &params );
    ASSERT_EQ( 2u, tracksQuery->count() );
    auto tracks = tracksQuery->all();
    ASSERT_EQ( m4->id(), tracks[0]->id() );
    ASSERT_EQ( m3->id(), tracks[1]->id() );

    params.desc = true;
    albums = T->ml->albums( &params )->all();
    // We do not invert the lexical order when sorting by DESC release date:
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a2->id(), albums[0]->id() );
    ASSERT_EQ( a1->id(), albums[1]->id() );
    ASSERT_EQ( a3->id(), albums[2]->id() );

    tracks = a3->tracks( &params )->all();
    ASSERT_EQ( m3->id(), tracks[0]->id() );
    ASSERT_EQ( m4->id(), tracks[1]->id() );

    // When listing all albums, default order is lexical order
    albums = T->ml->albums( nullptr )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a3->id(), albums[2]->id() );

    params.sort = SortingCriteria::Default;
    params.desc = true;
    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a3->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a1->id(), albums[2]->id() );
}

static void SortByPlayCount( Tests* T )
{
    auto a1 = T->ml->createAlbum( "North" );
    auto f1 = std::static_pointer_cast<Media>( T->ml->addMedia( "first.opus", IMedia::Type::Audio ) );
    auto res = a1->addTrack( f1, 1, 0, 0, nullptr );
    ASSERT_TRUE( res );
    auto f2 = std::static_pointer_cast<Media>( T->ml->addMedia( "second.opus", IMedia::Type::Audio ) );
    res = a1->addTrack( f2, 2, 0, 0, nullptr );
    ASSERT_TRUE( res );

    ASSERT_TRUE( f1->setPlayCount( 2 ) );

    ASSERT_TRUE( f2->setPlayCount( 1 ) );

    auto a2 = T->ml->createAlbum( "East" );
    auto f3 = std::static_pointer_cast<Media>( T->ml->addMedia( "third.opus", IMedia::Type::Audio ) );
    res = a2->addTrack( f3, 1, 0, 0, nullptr );
    ASSERT_TRUE( res );

    ASSERT_TRUE( f3->setPlayCount( 4 ) );

    auto a3 = T->ml->createAlbum( "South" );
    auto f4 = std::static_pointer_cast<Media>( T->ml->addMedia( "fourth.opus", IMedia::Type::Audio ) );
    res = a3->addTrack( f4, 1, 0, 0, nullptr );
    ASSERT_TRUE( res );

    ASSERT_TRUE( f4->setPlayCount( 1 ) );

    auto a4 = T->ml->createAlbum( "West" );
    auto f5 = std::static_pointer_cast<Media>( T->ml->addMedia( "fifth.opus", IMedia::Type::Audio ) );
    res = a4->addTrack( f5, 1, 0, 0, nullptr );
    ASSERT_TRUE( res );

    ASSERT_TRUE( f5->setPlayCount( 1 ) );

    QueryParameters params { SortingCriteria::PlayCount, false };
    auto query = T->ml->albums( &params );
    ASSERT_EQ( 4u, query->count() );
    auto albums = query->all(); // Expect descending order
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( a2->id(), albums[0]->id() ); // 4 plays
    ASSERT_EQ( a1->id(), albums[1]->id() ); // 3 plays
    // album 3 & 4 discriminated by lexicographic order of album titles
    ASSERT_EQ( a3->id(), albums[2]->id() ); // 1 play
    ASSERT_EQ( a4->id(), albums[3]->id() ); // 1 play

    params.desc = true;
    albums = T->ml->albums( &params )->all(); // Expect ascending order
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( a3->id(), albums[0]->id() ); // 1 play
    ASSERT_EQ( a4->id(), albums[1]->id() ); // 1 play
    ASSERT_EQ( a1->id(), albums[2]->id() ); // 3 plays
    ASSERT_EQ( a2->id(), albums[3]->id() ); // 4 plays

    // ♪ Listening North album ♫
    ASSERT_TRUE( f1->setPlayCount( f1->playCount() + 1 ) );
    ASSERT_TRUE( f2->setPlayCount( f2->playCount() + 1 ) );

    params.desc = false;
    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() ); // 5 plays
    ASSERT_EQ( a2->id(), albums[1]->id() ); // 4 plays
    ASSERT_EQ( a3->id(), albums[2]->id() ); // 1 play
    ASSERT_EQ( a4->id(), albums[3]->id() ); // 1 play
}

static void SortByArtist( Tests* T )
{
    auto artist1 = T->ml->createArtist( "Artist" );
    auto artist2 = T->ml->createArtist( "tsitrA" );

    // Create albums with a non-alphabetical order to avoid a false positive (where sorting by pkey
    // is the same as sorting by title)
    auto a1 = T->ml->createAlbum( "C" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a1->addTrack( m, 1, 0, 0, nullptr );
    a1->setAlbumArtist( artist1 );
    auto a2 = T->ml->createAlbum( "B" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    a2->addTrack( m2, 1, 0, 0, nullptr );
    a2->setAlbumArtist( artist2 );
    auto a3 = T->ml->createAlbum( "A" );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    a3->addTrack( m3, 1, 0, 0, nullptr );
    a3->setAlbumArtist( artist1 );

    QueryParameters params { SortingCriteria::Artist, false };
    auto albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a3->id(), albums[0]->id() );
    ASSERT_EQ( a1->id(), albums[1]->id() );
    ASSERT_EQ( a2->id(), albums[2]->id() );

    params.desc = true;
    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    // We expect Artist to be sorted in reverse order, but still in alphabetical order for albums
    ASSERT_EQ( a2->id(), albums[0]->id() );
    ASSERT_EQ( a3->id(), albums[1]->id() );
    ASSERT_EQ( a1->id(), albums[2]->id() );
}

static void SortByNonSensical( Tests* T )
{
    // Not that this sorting criteria makes a lot of sense, but it used to
    // trigger a crash on vlc desktop, because the criteria handling was
    // different when adding the joins and when selecting the fields.
    // Basically any non-explicitely handled sorting criteria was causing a crash
    auto artist1 = T->ml->createArtist( "Artist" );
    auto artist2 = T->ml->createArtist( "Artist 2" );

    // Create albums with a non-alphabetical order to avoid a false positive (where sorting by pkey
    // is the same as sorting by title)
    auto a1 = T->ml->createAlbum( "A" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a1->addTrack( m, 1, 0, 0, nullptr );
    a1->setAlbumArtist( artist1 );

    auto a2 = T->ml->createAlbum( "B" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    a2->addTrack( m2, 1, 0, 0, nullptr );
    a2->setAlbumArtist( artist2 );

    auto a3 = T->ml->createAlbum( "C" );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    a3->addTrack( m3, 1, 0, 0, nullptr );
    a3->setAlbumArtist( artist1 );

    QueryParameters params { static_cast<SortingCriteria>( -1 ), false };
    auto albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a3->id(), albums[2]->id() );

    params.desc = true;
    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    // We expect Artist to be sorted in reverse order, but still in alphabetical order for albums
    ASSERT_EQ( a3->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a1->id(), albums[2]->id() );
}

static void Duration( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );
    ASSERT_EQ( 0u, a->duration() );

    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    m->setDuration( 100 );
    auto res = a->addTrack( m, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );
    ASSERT_EQ( 100u, a->duration() );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    m2->setDuration( 200 );
    res = a->addTrack( m2, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );
    ASSERT_EQ( 300u, a->duration() );

    // Check that we don't add negative durations (default sqlite duration is -1)
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "track3.mp3", IMedia::Type::Audio ) );
    res = a->addTrack( m3, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );
    ASSERT_EQ( 300u, a->duration() );


    auto a2 = T->ml->album( a->id() );
    ASSERT_EQ( 300u, a2->duration() );

    // Check that the duration is updated when a media/track gets removed
    T->ml->deleteMedia( m2->id() );

    a2 = T->ml->album( a->id() );
    ASSERT_EQ( 100u, a2->duration() );

    // And check that we don't remove negative durations
    T->ml->deleteMedia( m3->id() );
    a2 = T->ml->album( a->id() );
    ASSERT_EQ( 100u, a2->duration() );
}

static void SearchAndSort( Tests* T )
{
    auto alb1 = T->ml->createAlbum( "Z album" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    alb1->addTrack( m, 1, 0, 0, nullptr );

    auto alb2 = T->ml->createAlbum( "A album" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    alb2->addTrack( m2, 1, 0, 0, nullptr );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "track3.mp3", IMedia::Type::Audio ) );
    alb2->addTrack( m3, 2, 0, 0, nullptr );

    QueryParameters params { SortingCriteria::Alpha, false };
    auto albs = T->ml->searchAlbums( "album", &params )->all();
    ASSERT_EQ( 2u, albs.size() );
    ASSERT_EQ( albs[0]->id(), alb2->id() );
    ASSERT_EQ( albs[1]->id(), alb1->id() );

    params.sort = SortingCriteria::TrackNumber;
    // Sorting by tracknumber is descending by default, so we expect album 2 first
    albs = T->ml->searchAlbums( "album", &params )->all();
    ASSERT_EQ( 2u, albs.size() );
    ASSERT_EQ( albs[0]->id(), alb2->id() );
    ASSERT_EQ( albs[1]->id(), alb1->id() );
}

static void SearchTracks( Tests* T )
{
    auto alb = T->ml->createAlbum( "Mustelidae" );

    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    m1->setTitle( "otter otter run run", true );
    alb->addTrack( m1, 1, 1, 0, nullptr );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    m2->setTitle( "weasel weasel", true );
    alb->addTrack( m2, 1, 1, 0, nullptr );

    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "random media.aac", IMedia::Type::Audio ) );
    m3->setTitle( "otters are cute but not on this album", true );

    auto allMedia = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 2u, allMedia.size() );

    auto albumTracksSearch = alb->searchTracks( "otter", nullptr )->all();
    ASSERT_EQ( 1u, albumTracksSearch.size() );
}

static void NbDiscs( Tests* T )
{
    auto alb = T->ml->createAlbum( "disc" );
    ASSERT_EQ( 1u, alb->nbDiscs() );

    auto res = alb->setNbDiscs( 123 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 123u, alb->nbDiscs() );

    alb = std::static_pointer_cast<Album>( T->ml->album( alb->id() ) );
    ASSERT_EQ( 123u, alb->nbDiscs() );
}

static void CheckDbModel( Tests* T )
{
    auto res = Album::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void SortByDuration( Tests* T )
{
    auto shortAlb = T->ml->createAlbum( "Short" );
    auto short1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "short1.mp3", IMedia::Type::Audio ) );
    // The media duration needs to be known when inserting an album track
    short1->setDuration( 123 );
    shortAlb->addTrack( short1, 1, 0, 0, nullptr );
    auto short2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "short2.mp3", IMedia::Type::Audio ) );
    short2->setDuration( 456 );
    shortAlb->addTrack( short2, 2, 0, 0, nullptr );

    auto longAlb = T->ml->createAlbum( "Long" );
    auto long1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "long1.mp3", IMedia::Type::Audio ) );
    long1->setDuration( 999999 );
    longAlb->addTrack( long1, 1, 0, 0, nullptr );
    auto long2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "long2.mp3", IMedia::Type::Audio ) );
    long2->setDuration( 888888 );
    longAlb->addTrack( long2, 2, 0, 0, nullptr );

    QueryParameters params{ SortingCriteria::Duration, false };
    auto albumsQuery = T->ml->albums( &params );
    ASSERT_NE( nullptr, albumsQuery );
    ASSERT_EQ( 2u, albumsQuery->count() );
    auto albums = albumsQuery->all();
    ASSERT_EQ( 2u, albums.size() );
    ASSERT_EQ( shortAlb->id(), albums[0]->id() );
    ASSERT_EQ( short1->duration() + short2->duration(), albums[0]->duration() );
    ASSERT_EQ( longAlb->id(), albums[1]->id() );
    ASSERT_EQ( long1->duration() + long2->duration(), albums[1]->duration() );

    params.desc = true;

    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( 2u, albums.size() );
    ASSERT_EQ( longAlb->id(), albums[0]->id() );
    ASSERT_EQ( shortAlb->id(), albums[1]->id() );

    // Now try sorting the tracks by duration
    auto tracksQuery = albums[0]->tracks( &params );
    ASSERT_EQ( 2u, tracksQuery->count() );
    auto tracks = tracksQuery->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( long1->id(), tracks[0]->id() );
    ASSERT_EQ( long2->id(), tracks[1]->id() );

    params.desc = false;
    tracks = albums[0]->tracks( &params )->all();

    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( long2->id(), tracks[0]->id() );
    ASSERT_EQ( long1->id(), tracks[1]->id() );
}

static void SortByInsertionDate( Tests* T )
{
    auto alb1 = T->ml->createAlbum( "album 1" );
    auto alb2 = T->ml->createAlbum( "album 2" );
    ASSERT_NON_NULL( alb1 );
    ASSERT_NON_NULL( alb2 );

    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    ASSERT_NON_NULL( m1 );
    ASSERT_NON_NULL( m2 );

    auto res = T->ml->setMediaInsertionDate( m1->id(), 987 );
    ASSERT_TRUE( res );
    res = T->ml->setMediaInsertionDate( m2->id(), 123 );
    ASSERT_TRUE( res );

    alb1->addTrack( m1, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );
    alb2->addTrack( m2, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );

    QueryParameters params{};
    params.sort = SortingCriteria::InsertionDate;
    params.desc = false;
    auto albums = T->ml->albums( &params )->all();
    ASSERT_EQ( albums.size(), 2u );
    ASSERT_EQ( albums[0]->id(), alb2->id() );
    ASSERT_EQ( albums[1]->id(), alb1->id() );

    params.desc = true;
    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( albums.size(), 2u );
    ASSERT_EQ( albums[0]->id(), alb1->id() );
    ASSERT_EQ( albums[1]->id(), alb2->id() );

    auto m3 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    ASSERT_NON_NULL( m3 );

    // Now insert a new track to album2 and force its insertion date before album1's media
    res = T->ml->setMediaInsertionDate( m3->id(), 12 );
    ASSERT_TRUE( res );
    res = alb1->addTrack( m3, 2, 1, 0, nullptr );
    ASSERT_TRUE( res );

    params.desc = false;
    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( albums.size(), 2u );
    ASSERT_EQ( albums[0]->id(), alb1->id() );
    ASSERT_EQ( albums[1]->id(), alb2->id() );

    params.desc = true;
    albums = T->ml->albums( &params )->all();
    ASSERT_EQ( albums.size(), 2u );
    ASSERT_EQ( albums[0]->id(), alb2->id() );
    ASSERT_EQ( albums[1]->id(), alb1->id() );
}

static void ConvertToExternal( Tests* T )
{
    auto a = T->ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>(
                T->ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    auto res = m->setDuration( 10 );
    ASSERT_TRUE( res );
    res = m2->setDuration( 90 );
    ASSERT_TRUE( res );

    res = a->addTrack( m, 1, 1, 0, nullptr );
    ASSERT_TRUE( res );
    res = a->addTrack( m2, 2, 1, 0, nullptr );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, a->nbTracks() );
    ASSERT_EQ( 100, a->duration() );
    a = std::static_pointer_cast<Album>( T->ml->album( a->id() ) );
    ASSERT_EQ( 2u, a->nbTracks() );
    ASSERT_EQ( 2u, a->nbPresentTracks() );
    ASSERT_EQ( 100, a->duration() );

    auto deviceId = m->deviceId();
    auto folderId = m->folderId();
    res = m->convertToExternal();
    ASSERT_TRUE( res );

    a = std::static_pointer_cast<Album>( T->ml->album( a->id() ) );
    ASSERT_EQ( 1u, a->nbTracks() );
    ASSERT_EQ( 1u, a->nbPresentTracks() );

    res = m->markAsInternal( IMedia::Type::Audio, m->duration(), deviceId, folderId );
    ASSERT_TRUE( res );

    /*
     * The swich to internal in itself doesn't add the genre back. Outside of a
     * test configuration, a switch back to internal is followed by a refresh
     * for the media.
     * Here, we need to simulate this
     */
    a = std::static_pointer_cast<Album>( T->ml->album( a->id() ) );
    ASSERT_EQ( 1u, a->nbTracks() );
    ASSERT_EQ( 1u, a->nbPresentTracks() );
    ASSERT_EQ( 90, a->duration() );

    res = m->markAsAlbumTrack( a->id(), 1, 1, 0, nullptr );
    ASSERT_TRUE( res );

    a = std::static_pointer_cast<Album>( T->ml->album( a->id() ) );
    ASSERT_EQ( 2u, a->nbTracks() );
    ASSERT_EQ( 2u, a->nbPresentTracks() );
    ASSERT_EQ( 100, a->duration() );

    res = m->convertToExternal();
    ASSERT_TRUE( res );
    res = m2->convertToExternal();
    ASSERT_TRUE( res );

    a = std::static_pointer_cast<Album>( T->ml->album( a->id() ) );
    ASSERT_EQ( nullptr, a );
}

int main( int ac, char** av )
{
    INIT_TESTS( Album )
    ADD_TEST( Create );
    ADD_TEST( Fetch );
    ADD_TEST( AddTrack );
    ADD_TEST( RemoveTrack );
    ADD_TEST( NbTrack );
    ADD_TEST( TracksByGenre );
    ADD_TEST( SetReleaseDate );
    ADD_TEST( SetShortSummary );
    ADD_TEST( GetThumbnail );
    ADD_TEST( FetchAlbumFromTrack );
    ADD_TEST( Artists );
    ADD_TEST( AlbumArtist );
    ADD_TEST( SortAlbumThenArtist );
    ADD_TEST( SearchByTitle );
    ADD_TEST( SearchByArtist );
    ADD_TEST( SearchNoDuplicate );
    ADD_TEST( SearchNoUnknownAlbum );
    ADD_TEST( SearchAfterDeletion );
    ADD_TEST( SearchAfterArtistUpdate );
    ADD_TEST( AutoDelete );
    ADD_TEST( SortTracks );
    ADD_TEST( Sort );
    ADD_TEST( SortByPlayCount );
    ADD_TEST( SortByArtist );
    ADD_TEST( SortByNonSensical );
    ADD_TEST( Duration );
    ADD_TEST( SearchAndSort );
    ADD_TEST( SearchTracks );
    ADD_TEST( NbDiscs );
    ADD_TEST( CheckDbModel );
    ADD_TEST( SortByDuration );
    ADD_TEST( SortByInsertionDate );
    ADD_TEST( ConvertToExternal );

    END_TESTS
}
