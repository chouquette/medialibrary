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
    ASSERT_EQ( a2->title(), "album" );
}

TEST_F( Albums, Fetch )
{
    auto a = ml->createAlbum( "album" );

    auto a2 = ml->album( a->id() );
    // The shared pointer are expected to point to a different instance
    ASSERT_NE( a, a2 );

    ASSERT_EQ( a->id(), a2->id() );
}

TEST_F( Albums, AddTrack )
{
    auto a = ml->createAlbum( "albumtag" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    auto track = a->addTrack( f, 10, 0, 0, nullptr );
    f->save();
    ASSERT_NE( track, nullptr );

    auto tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), 1u );

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0]->albumTrack()->trackNumber(), track->trackNumber() );
}

TEST_F( Albums, RemoveTracks )
{
    auto a = ml->createAlbum( "albumtag" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    auto track = a->addTrack( m, 10, 0, 0, nullptr );
    m->save();
    ASSERT_NE( track, nullptr );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    auto track2 = a->addTrack( m2, 11, 0, 0, nullptr );
    m2->save();

    auto tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), 2u );

    auto res = a->removeTrack( *m2, *track2 );
    ASSERT_TRUE( res );
    res = a->removeTrack( *m, *track );
    ASSERT_TRUE( res );
}

TEST_F( Albums, NbTracks )
{
    auto a = ml->createAlbum( "albumtag" );
    for ( auto i = 1u; i <= 10; ++i )
    {
        auto f = std::static_pointer_cast<Media>(
                    ml->addMedia( "track" + std::to_string(i) + ".mp3", IMedia::Type::Audio ) );
        auto track = a->addTrack( f, i, i, 0, nullptr );
        f->save();
        ASSERT_NE( track, nullptr );
    }
    auto tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), a->nbTracks() );

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), a->nbTracks() );
}

TEST_F( Albums, TracksByGenre )
{
    auto a = ml->createAlbum( "albumtag" );
    auto g = ml->createGenre( "genre" );

    for ( auto i = 1u; i <= 10; ++i )
    {
        auto f = std::static_pointer_cast<Media>(
                    ml->addMedia( "track" + std::to_string(i) + ".mp3", IMedia::Type::Audio ) );
        auto track = a->addTrack( f, i, i, 0, i <= 5 ? g.get() : nullptr );
        f->save();
        ASSERT_NE( track, nullptr );
    }
    auto tracksQuery = a->tracks( nullptr, nullptr );
    ASSERT_EQ( nullptr, tracksQuery );
    tracksQuery = a->tracks( g, nullptr );
    ASSERT_EQ( 5u, tracksQuery->count() );
    auto tracks = tracksQuery->all();
    ASSERT_EQ( 5u, tracks.size() );

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    tracks = a->tracks( g, nullptr )->all();
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

    auto a2 = ml->album( a->id() );
    ASSERT_EQ( a->releaseYear(), a2->releaseYear() );
}

TEST_F( Albums, SetShortSummary )
{
    auto a = ml->createAlbum( "album" );

    a->setShortSummary( "summary" );
    ASSERT_EQ( a->shortSummary(), "summary" );

    auto a2 = ml->album( a->id() );
    ASSERT_EQ( a->shortSummary(), a2->shortSummary() );
}

TEST_F( Albums, Thumbnail )
{
    auto a = ml->createAlbum( "album" );
    auto t = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( nullptr, t );
    ASSERT_EQ( ThumbnailStatus::Missing,
               a->thumbnailStatus( ThumbnailSizeType::Thumbnail ) );

    std::string mrl = "file:///path/to/sea/otter/artwork.png";
    t = std::make_shared<Thumbnail>( ml.get(), mrl, Thumbnail::Origin::UserProvided,
                                     ThumbnailSizeType::Thumbnail, false );
    auto id = t->insert();
    ASSERT_NE( 0, id );
    a = ml->MediaLibrary::createAlbum( "album 2" );
    a->setThumbnail( t );

    t = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NE( nullptr, t );
    ASSERT_EQ( mrl, t->mrl() );
    ASSERT_EQ( ThumbnailStatus::Available,
               a->thumbnailStatus( ThumbnailSizeType::Thumbnail ) );

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    t = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NE( nullptr, t );
    ASSERT_EQ( mrl, t->mrl() );
    ASSERT_EQ( ThumbnailStatus::Available,
               a->thumbnailStatus( ThumbnailSizeType::Thumbnail ) );
}

TEST_F( Albums, FetchAlbumFromTrack )
{
    auto a = ml->createAlbum( "album" );
    auto f = std::static_pointer_cast<Media>(
                ml->addMedia( "file.mp3", IMedia::Type::Audio ) );
    auto t = a->addTrack( f, 1, 0, 0, nullptr );
    f->save();

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

    auto m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    album->addTrack( m1, 1, 0, artist1->id(), nullptr );
    m1->save();

    auto m2 = std::static_pointer_cast<Media>(
                ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    album->addTrack( m2, 2, 0, artist2->id(), nullptr );
    m2->save();

    QueryParameters params { SortingCriteria::Default, false };
    auto query = album->artists( &params );
    ASSERT_EQ( 2u, query->count() );
    auto artists = query->all();
    ASSERT_EQ( artists.size(), 2u );
    ASSERT_EQ( artist1->id(), artists[1]->id() );
    ASSERT_EQ( artist2->id(), artists[0]->id() );

    params.desc = true;
    album = std::static_pointer_cast<Album>( ml->album( album->id() ) );
    query = album->artists( &params );
    ASSERT_EQ( 2u, query->count() );
    artists = query->all();
    ASSERT_EQ( artists.size(), 2u );
    ASSERT_EQ( artist1->id(), artists[0]->id() );
    ASSERT_EQ( artist2->id(), artists[1]->id() );
}

TEST_F( Albums, AlbumArtist )
{
    auto album = ml->createAlbum( "test" );
    ASSERT_EQ( album->albumArtist(), nullptr );
    auto artist = ml->createArtist( "artist" );
    auto res = album->setAlbumArtist( artist );
    ASSERT_TRUE( res );
    // Override with the same artist, expect a success
    res = album->setAlbumArtist( artist );
    ASSERT_TRUE( res );
    auto noartist = std::make_shared<Artist>( ml.get(), "dummy artist" );
    ASSERT_EQ( 0, noartist->id() );
    res = album->setAlbumArtist( noartist );
    ASSERT_FALSE( res );
    ASSERT_NE( album->albumArtist(), nullptr );

    album = std::static_pointer_cast<Album>( ml->album( album->id() ) );
    auto albumArtist = album->albumArtist();
    ASSERT_NE( albumArtist, nullptr );
    ASSERT_EQ( albumArtist->name(), artist->name() );
}

TEST_F( Albums, SortAlbumThenArtist )
{
    // First
    auto albumOttersO = ml->createAlbum( "otters" );
    // Second
    auto albumPangolinsP = ml->createAlbum( "pangolins of fire" );
    // Fourth
    auto albumPangolinsS = ml->createAlbum( "see otters" );
    // Third
    auto albumOttersS = ml->createAlbum( "sea otters" );
    // Originally the medialibrary handled ordering in case of identical
    // album name by using the insertion order.
    // Here the insertion order is different than the expected sort order.

    auto artistP = ml->createArtist( "pangolins" );
    auto artistO = ml->createArtist( "otters" );

    albumOttersO->setAlbumArtist( artistO );
    albumPangolinsP->setAlbumArtist( artistP );
    albumOttersS->setAlbumArtist( artistO );
    albumPangolinsS->setAlbumArtist( artistP);

    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    albumOttersO->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    albumPangolinsP->addTrack( m2, 1, 0, 0, nullptr );
    m2->save();
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    albumOttersS->addTrack( m3, 1, 0, 0, nullptr );
    m3->save();
    auto m4 = std::static_pointer_cast<Media>( ml->addMedia( "media4.mp3", IMedia::Type::Audio ) );
    albumPangolinsS->addTrack( m4, 1, 0, 0, nullptr );
    m3->save();

    QueryParameters params { SortingCriteria::Alpha, false };
    auto albums = ml->albums( &params )->all();
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( albumOttersO->id(), albums[0]->id() );
    ASSERT_EQ( albumPangolinsP->id(), albums[1]->id() );
    ASSERT_EQ( albumOttersS->id(), albums[2]->id() );
    ASSERT_EQ( albumPangolinsS->id(), albums[3]->id() );

    params.desc = true;
    albums = ml->albums( &params )->all();
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( albumPangolinsS->id(), albums[0]->id() );
    ASSERT_EQ( albumOttersS->id(), albums[1]->id() );
    ASSERT_EQ( albumPangolinsP->id(), albums[2]->id() );
    ASSERT_EQ( albumOttersO->id(), albums[3]->id() );
}

TEST_F( Albums, SearchByTitle )
{
    auto a1 = ml->createAlbum( "sea otters" );
    auto a2 = ml->createAlbum( "pangolins of fire" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a1->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    a2->addTrack( m2, 1, 0, 0, nullptr );
    m2->save();

    auto query = ml->searchAlbums( "", nullptr );
    ASSERT_EQ( nullptr, query );

    auto albums = ml->searchAlbums( "otte", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
}

TEST_F( Albums, SearchByArtist )
{
    auto a = ml->createAlbum( "sea otters" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    auto artist = ml->createArtist( "pangolins" );
    a->setAlbumArtist( artist );

    auto albums = ml->searchAlbums( "pangol", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
}

TEST_F( Albums, SearchNoDuplicate )
{
    auto a = ml->createAlbum( "sea otters" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    auto artist = ml->createArtist( "otters" );
    a->setAlbumArtist( artist );

    auto albums = ml->searchAlbums( "otters", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
}

TEST_F( Albums, SearchNoUnknownAlbum )
{
    auto artist = ml->createArtist( "otters" );
    auto album = artist->unknownAlbum();
    ASSERT_TRUE( album->isUnknownAlbum() );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    album->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    ASSERT_NE( nullptr, album );

    auto albums = ml->searchAlbums( "otters", nullptr )->all();
    ASSERT_EQ( 0u, albums.size() );
    // Can't search by name since there is no name set for unknown albums
}

TEST_F( Albums, SearchAfterDeletion )
{
    auto a = ml->createAlbum( "sea otters" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    auto albums = ml->searchAlbums( "sea", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );

    ml->deleteAlbum( a->id() );

    albums = ml->searchAlbums( "sea", nullptr )->all();
    ASSERT_EQ( 0u, albums.size() );
}

TEST_F( Albums, SearchAfterArtistUpdate )
{
    auto a = ml->createAlbum( "sea otters" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    auto artist = ml->createArtist( "pangolin of fire" );
    auto artist2 = ml->createArtist( "pangolin of ice" );
    a->setAlbumArtist( artist );

    auto albums = ml->searchAlbums( "fire", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );

    albums = ml->searchAlbums( "ice", nullptr )->all();
    ASSERT_EQ( 0u, albums.size() );

    a->setAlbumArtist( artist2 );

    albums = ml->searchAlbums( "fire", nullptr )->all();
    ASSERT_EQ( 0u, albums.size() );

    albums = ml->searchAlbums( "ice", nullptr )->all();
    ASSERT_EQ( 1u, albums.size() );
}

TEST_F( Albums, AutoDelete )
{
    auto a = ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    auto t = a->addTrack( m, 1, 1, 0, nullptr );

    auto album = ml->album( a->id() );
    ASSERT_NE( nullptr, album );

    ml->deleteMedia( m->id() );

    album = ml->album( a->id() );
    ASSERT_EQ( nullptr, album );
}

TEST_F( Albums, SortTracks )
{
    auto a = ml->createAlbum( "album" );
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "B-track1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "A-track2.mp3", IMedia::Type::Audio ) );
    auto t1 = a->addTrack( m1, 1, 1, 0, nullptr );
    auto t2 = a->addTrack( m2, 2, 1, 0, nullptr );
    m1->save();
    m2->save();

    // Default order is by disc number & track number
    auto tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( t1->id(), tracks[0]->id() );
    ASSERT_EQ( t2->id(), tracks[1]->id() );

    // Reverse order
    QueryParameters params { SortingCriteria::TrackId, true };
    tracks = a->tracks( &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( t1->id(), tracks[1]->id() );
    ASSERT_EQ( t2->id(), tracks[0]->id() );

    // Try a media based criteria
    params = { SortingCriteria::Alpha, false };
    tracks = a->tracks( &params )->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( t1->id(), tracks[1]->id() ); // B-track -> first
    ASSERT_EQ( t2->id(), tracks[0]->id() ); // A-track -> second
}

TEST_F( Albums, Sort )
{
    auto a1 = ml->createAlbum( "A" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a1->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    a1->setReleaseYear( 1000, false );
    auto a2 = ml->createAlbum( "B" );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    a2->addTrack( m2, 1, 0, 0, nullptr );
    m2->save();
    a2->setReleaseYear( 2000, false );
    auto a3 = ml->createAlbum( "C" );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    a3->addTrack( m3, 1, 0, 0, nullptr );
    m3->setReleaseDate( 1000 );
    m3->save();
    auto m4 = std::static_pointer_cast<Media>( ml->addMedia( "media4.mp3", IMedia::Type::Audio ) );
    a3->addTrack( m4, 2, 0, 0, nullptr );
    m4->setReleaseDate( 995 );
    m4->save();
    a3->setReleaseYear( 1000, false );

    QueryParameters params { SortingCriteria::ReleaseDate, false };
    auto albums = ml->albums( &params )->all();
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
    albums = ml->albums( &params )->all();
    // We do not invert the lexical order when sorting by DESC release date:
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a2->id(), albums[0]->id() );
    ASSERT_EQ( a1->id(), albums[1]->id() );
    ASSERT_EQ( a3->id(), albums[2]->id() );

    tracks = a3->tracks( &params )->all();
    ASSERT_EQ( m3->id(), tracks[0]->id() );
    ASSERT_EQ( m4->id(), tracks[1]->id() );

    // When listing all albums, default order is lexical order
    albums = ml->albums( nullptr )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a3->id(), albums[2]->id() );

    params.sort = SortingCriteria::Default;
    params.desc = true;
    albums = ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a3->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a1->id(), albums[2]->id() );
}

TEST_F( Albums, SortByPlayCount )
{
    auto a1 = ml->createAlbum( "North" );
    auto f1 = std::static_pointer_cast<Media>( ml->addMedia( "first.opus", IMedia::Type::Audio ) );
    auto t1 = a1->addTrack( f1, 1, 0, 0, nullptr );
    f1->save();
    auto f2 = std::static_pointer_cast<Media>( ml->addMedia( "second.opus", IMedia::Type::Audio ) );
    auto t2 = a1->addTrack( f2, 2, 0, 0, nullptr );
    f2->save();

    ASSERT_TRUE( f1->setPlayCount( 2 ) );

    ASSERT_TRUE( f2->setPlayCount( 1 ) );

    auto a2 = ml->createAlbum( "East" );
    auto f3 = std::static_pointer_cast<Media>( ml->addMedia( "third.opus", IMedia::Type::Audio ) );
    auto t3 = a2->addTrack( f3, 1, 0, 0, nullptr );
    f3->save();

    ASSERT_TRUE( f3->setPlayCount( 4 ) );

    auto a3 = ml->createAlbum( "South" );
    auto f4 = std::static_pointer_cast<Media>( ml->addMedia( "fourth.opus", IMedia::Type::Audio ) );
    auto t4 = a3->addTrack( f4, 1, 0, 0, nullptr );
    f4->save();

    ASSERT_TRUE( f4->setPlayCount( 1 ) );

    auto a4 = ml->createAlbum( "West" );
    auto f5 = std::static_pointer_cast<Media>( ml->addMedia( "fifth.opus", IMedia::Type::Audio ) );
    auto t5 = a4->addTrack( f5, 1, 0, 0, nullptr );
    f5->save();

    ASSERT_TRUE( f5->setPlayCount( 1 ) );

    QueryParameters params { SortingCriteria::PlayCount, false };
    auto query = ml->albums( &params );
    ASSERT_EQ( 4u, query->count() );
    auto albums = query->all(); // Expect descending order
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( a2->id(), albums[0]->id() ); // 4 plays
    ASSERT_EQ( a1->id(), albums[1]->id() ); // 3 plays
    // album 3 & 4 discriminated by lexicographic order of album titles
    ASSERT_EQ( a3->id(), albums[2]->id() ); // 1 play
    ASSERT_EQ( a4->id(), albums[3]->id() ); // 1 play

    params.desc = true;
    albums = ml->albums( &params )->all(); // Expect ascending order
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( a3->id(), albums[0]->id() ); // 1 play
    ASSERT_EQ( a4->id(), albums[1]->id() ); // 1 play
    ASSERT_EQ( a1->id(), albums[2]->id() ); // 3 plays
    ASSERT_EQ( a2->id(), albums[3]->id() ); // 4 plays

    // ♪ Listening North album ♫
    ASSERT_TRUE( f1->setPlayCount( f1->playCount() + 1 ) );
    ASSERT_TRUE( f2->setPlayCount( f2->playCount() + 1 ) );

    params.desc = false;
    albums = ml->albums( &params )->all();
    ASSERT_EQ( 4u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() ); // 5 plays
    ASSERT_EQ( a2->id(), albums[1]->id() ); // 4 plays
    ASSERT_EQ( a3->id(), albums[2]->id() ); // 1 play
    ASSERT_EQ( a4->id(), albums[3]->id() ); // 1 play
}

TEST_F( Albums, SortByArtist )
{
    auto artist1 = ml->createArtist( "Artist" );
    auto artist2 = ml->createArtist( "tsitrA" );

    // Create albums with a non-alphabetical order to avoid a false positive (where sorting by pkey
    // is the same as sorting by title)
    auto a1 = ml->createAlbum( "C" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a1->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    a1->setAlbumArtist( artist1 );
    auto a2 = ml->createAlbum( "B" );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    a2->addTrack( m2, 1, 0, 0, nullptr );
    m2->save();
    a2->setAlbumArtist( artist2 );
    auto a3 = ml->createAlbum( "A" );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    a3->addTrack( m3, 1, 0, 0, nullptr );
    m3->save();
    a3->setAlbumArtist( artist1 );

    QueryParameters params { SortingCriteria::Artist, false };
    auto albums = ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a3->id(), albums[0]->id() );
    ASSERT_EQ( a1->id(), albums[1]->id() );
    ASSERT_EQ( a2->id(), albums[2]->id() );

    params.desc = true;
    albums = ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    // We expect Artist to be sorted in reverse order, but still in alphabetical order for albums
    ASSERT_EQ( a2->id(), albums[0]->id() );
    ASSERT_EQ( a3->id(), albums[1]->id() );
    ASSERT_EQ( a1->id(), albums[2]->id() );
}

TEST_F( Albums, SortByNonSensical )
{
    // Not that this sorting criteria makes a lot of sense, but it used to
    // trigger a crash on vlc desktop, because the criteria handling was
    // different when adding the joins and when selecting the fields.
    // Basically any non-explicitely handled sorting criteria was causing a crash
    auto artist1 = ml->createArtist( "Artist" );
    auto artist2 = ml->createArtist( "Artist 2" );

    // Create albums with a non-alphabetical order to avoid a false positive (where sorting by pkey
    // is the same as sorting by title)
    auto a1 = ml->createAlbum( "A" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3", IMedia::Type::Audio ) );
    a1->addTrack( m, 1, 0, 0, nullptr );
    m->save();
    a1->setAlbumArtist( artist1 );

    auto a2 = ml->createAlbum( "B" );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    a2->addTrack( m2, 1, 0, 0, nullptr );
    m2->save();
    a2->setAlbumArtist( artist2 );

    auto a3 = ml->createAlbum( "C" );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    a3->addTrack( m3, 1, 0, 0, nullptr );
    m3->save();
    a3->setAlbumArtist( artist1 );

    QueryParameters params { SortingCriteria::InsertionDate, false };
    auto albums = ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( a1->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a3->id(), albums[2]->id() );

    params.desc = true;
    albums = ml->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    // We expect Artist to be sorted in reverse order, but still in alphabetical order for albums
    ASSERT_EQ( a3->id(), albums[0]->id() );
    ASSERT_EQ( a2->id(), albums[1]->id() );
    ASSERT_EQ( a1->id(), albums[2]->id() );
}

TEST_F( Albums, Duration )
{
    auto a = ml->createAlbum( "album" );
    ASSERT_EQ( 0u, a->duration() );

    auto m = std::static_pointer_cast<Media>( ml->addMedia( "track.mp3", IMedia::Type::Audio ) );
    m->setDuration( 100 );
    m->save();
    a->addTrack( m, 1, 1, 0, nullptr );
    ASSERT_EQ( 100u, a->duration() );

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    m2->setDuration( 200 );
    m2->save();
    auto t2 = a->addTrack( m2, 1, 1, 0, nullptr );
    ASSERT_EQ( 300u, a->duration() );

    // Check that we don't add negative durations (default sqlite duration is -1)
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "track3.mp3", IMedia::Type::Audio ) );
    auto t3 = a->addTrack( m3, 1, 1, 0, nullptr );
    ASSERT_EQ( 300u, a->duration() );

    auto a2 = ml->album( a->id() );
    ASSERT_EQ( 300u, a2->duration() );

    // Check that the duration is updated when a media/track gets removed
    ml->deleteMedia( m2->id() );

    a2 = ml->album( a->id() );
    ASSERT_EQ( 100u, a2->duration() );

    // And check that we don't remove negative durations
    ml->deleteMedia( m3->id() );
    a2 = ml->album( a->id() );
    ASSERT_EQ( 100u, a2->duration() );
}

TEST_F( Albums, SearchAndSort )
{
    auto alb1 = ml->createAlbum( "Z album" );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    alb1->addTrack( m, 1, 0, 0, nullptr );

    auto alb2 = ml->createAlbum( "A album" );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    alb2->addTrack( m2, 1, 0, 0, nullptr );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "track3.mp3", IMedia::Type::Audio ) );
    alb2->addTrack( m3, 2, 0, 0, nullptr );

    QueryParameters params { SortingCriteria::Alpha, false };
    auto albs = ml->searchAlbums( "album", &params )->all();
    ASSERT_EQ( 2u, albs.size() );
    ASSERT_EQ( albs[0]->id(), alb2->id() );
    ASSERT_EQ( albs[1]->id(), alb1->id() );

    params.sort = SortingCriteria::TrackNumber;
    // Sorting by tracknumber is descending by default, so we expect album 2 first
    albs = ml->searchAlbums( "album", &params )->all();
    ASSERT_EQ( 2u, albs.size() );
    ASSERT_EQ( albs[0]->id(), alb2->id() );
    ASSERT_EQ( albs[1]->id(), alb1->id() );
}

TEST_F( Albums, SearchTracks )
{
    auto alb = ml->createAlbum( "Mustelidae" );

    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    m1->setTitleBuffered( "otter otter run run" );
    alb->addTrack( m1, 1, 1, 0, nullptr );
    m1->save();

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    m2->setTitleBuffered( "weasel weasel" );
    alb->addTrack( m2, 1, 1, 0, nullptr );
    m2->save();

    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "random media.aac", IMedia::Type::Audio ) );
    m3->setTitleBuffered( "otters are cute but not on this album" );
    m3->save();

    auto allMedia = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 2u, allMedia.size() );

    auto albumTracksSearch = alb->searchTracks( "otter", nullptr )->all();
    ASSERT_EQ( 1u, albumTracksSearch.size() );
}

TEST_F( Albums, NbDiscs )
{
    auto alb = ml->createAlbum( "disc" );
    ASSERT_EQ( 1u, alb->nbDiscs() );

    auto res = alb->setNbDiscs( 123 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 123u, alb->nbDiscs() );

    alb = std::static_pointer_cast<Album>( ml->album( alb->id() ) );
    ASSERT_EQ( 123u, alb->nbDiscs() );
}

TEST_F( Albums, CheckDbModel )
{
    auto res = Album::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F ( Albums, SortByDuration )
{
    auto shortAlb = ml->createAlbum( "Short" );
    auto short1 = std::static_pointer_cast<Media>(
                ml->addMedia( "short1.mp3", IMedia::Type::Audio ) );
    // The media duration needs to be known when inserting an album track
    short1->setDuration( 123 );
    short1->save();
    shortAlb->addTrack( short1, 1, 0, 0, nullptr );
    short1->save();
    auto short2 = std::static_pointer_cast<Media>(
                ml->addMedia( "short2.mp3", IMedia::Type::Audio ) );
    short2->setDuration( 456 );
    short2->save();
    shortAlb->addTrack( short2, 2, 0, 0, nullptr );
    short2->save();

    auto longAlb = ml->createAlbum( "Long" );
    auto long1 = std::static_pointer_cast<Media>(
                ml->addMedia( "long1.mp3", IMedia::Type::Audio ) );
    long1->setDuration( 999999 );
    long1->save();
    longAlb->addTrack( long1, 1, 0, 0, nullptr );
    long1->save();
    auto long2 = std::static_pointer_cast<Media>(
                ml->addMedia( "long2.mp3", IMedia::Type::Audio ) );
    long2->setDuration( 888888 );
    long2->save();
    longAlb->addTrack( long2, 2, 0, 0, nullptr );
    long2->save();

    QueryParameters params{ SortingCriteria::Duration, false };
    auto albumsQuery = ml->albums( &params );
    ASSERT_NE( nullptr, albumsQuery );
    ASSERT_EQ( 2u, albumsQuery->count() );
    auto albums = albumsQuery->all();
    ASSERT_EQ( 2u, albums.size() );
    ASSERT_EQ( shortAlb->id(), albums[0]->id() );
    ASSERT_EQ( short1->duration() + short2->duration(), albums[0]->duration() );
    ASSERT_EQ( longAlb->id(), albums[1]->id() );
    ASSERT_EQ( long1->duration() + long2->duration(), albums[1]->duration() );

    params.desc = true;

    albums = ml->albums( &params )->all();
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
