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

#include "Artist.h"
#include "Album.h"
#include "Media.h"

static void Create( Tests* T )
{
    auto a = T->ml->createArtist( "Flying Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->name(), "Flying Otters" );

    auto a2 = T->ml->artist( a->id() );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->name(), "Flying Otters" );
}

static void ShortBio( Tests* T )
{
    auto a = T->ml->createArtist( "Raging Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->shortBio(), "" );

    std::string bio("An otter based post-rock band");
    a->setShortBio( bio );
    ASSERT_EQ( a->shortBio(), bio );

    auto a2 = T->ml->artist( a->id() );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->shortBio(), bio );
}

static void ArtworkMrl( Tests* T )
{
    auto a = T->ml->createArtist( "Dream seaotter" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->thumbnailMrl( ThumbnailSizeType::Thumbnail ), "" );
    ASSERT_EQ( ThumbnailStatus::Missing,
               a->thumbnailStatus( ThumbnailSizeType::Thumbnail ) );

    std::string artwork("file:///tmp/otter.png");
    a->setThumbnail( artwork, ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( a->thumbnailMrl( ThumbnailSizeType::Thumbnail ), artwork );
    ASSERT_EQ( ThumbnailStatus::Available,
               a->thumbnailStatus( ThumbnailSizeType::Thumbnail ) );

    auto a2 = T->ml->artist( a->id() );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->thumbnailMrl( ThumbnailSizeType::Thumbnail ), artwork );
    ASSERT_EQ( ThumbnailStatus::Available,
               a->thumbnailStatus( ThumbnailSizeType::Thumbnail ) );
}

static void GetThumbnail( Tests* T )
{
    auto a = T->ml->createArtist( "artist" );
    auto t = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( nullptr, t );

    std::string mrl = "file:///path/to/sea/otter/artwork.png";
    auto res = a->setThumbnail( mrl, ThumbnailSizeType::Thumbnail );
    ASSERT_TRUE( res );

    t = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NE( nullptr, t );
    ASSERT_EQ( mrl, t->mrl() );

    a = std::static_pointer_cast<Artist>( T->ml->artist( a->id() ) );
    t = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NE( nullptr, t );
    ASSERT_EQ( mrl, t->mrl() );
}

// Test the number of albums based on the artist tracks
static void Albums( Tests* T )
{
    auto artist = T->ml->createArtist( "Cannibal Otters" );
    auto album1 = T->ml->createAlbum( "album1" );
    auto album2 = T->ml->createAlbum( "album2" );

    ASSERT_NE( artist, nullptr );
    ASSERT_NE( album1, nullptr );
    ASSERT_NE( album2, nullptr );

    auto media1 = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    ASSERT_NE( nullptr, media1 );
    album1->addTrack( media1, 1, 0, artist->id(), nullptr );
    media1->save();
    auto media2 = std::static_pointer_cast<Media>( T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    ASSERT_NE( nullptr, media2 );
    album2->addTrack( media2, 1, 0, artist->id(), nullptr );
    media2->save();

    auto media3 = std::static_pointer_cast<Media>( T->ml->addMedia( "track3.mp3", IMedia::Type::Audio ) );
    ASSERT_NE( nullptr, media3 );
    album2->addTrack( media3, 2, 0, artist->id(), nullptr );
    media3->save();

    auto media4 = std::static_pointer_cast<Media>( T->ml->addMedia( "track4.mp3", IMedia::Type::Audio ) );
    ASSERT_NE( nullptr, media4 );
    album2->addTrack( media4, 3, 0, artist->id(), nullptr );
    media4->save();

    album1->setAlbumArtist( artist );
    album2->setAlbumArtist( artist );

    auto query = artist->albums( nullptr );
    ASSERT_EQ( 2u, query->count() );
    auto albums = query->all();
    ASSERT_EQ( albums.size(), 2u );

    auto artist2 = T->ml->artist( artist->id() );
    auto albums2 = artist2->albums( nullptr )->all();
    ASSERT_EQ( albums2.size(), 2u );
}

// Test the nb_album DB field (ie. we don't need to create tracks for this test)
static void NbAlbums( Tests* T )
{
    auto artist = T->ml->createArtist( "Cannibal Otters" );
    auto album1 = T->ml->createAlbum( "album1" );
    auto album2 = T->ml->createAlbum( "album2" );

    ASSERT_NE( artist, nullptr );
    ASSERT_NE( album1, nullptr );
    ASSERT_NE( album2, nullptr );

    album1->setAlbumArtist( artist );
    album2->setAlbumArtist( artist );

    artist = std::static_pointer_cast<Artist>( T->ml->artist( artist->id() ) );

    auto nbAlbums = artist->nbAlbums();
    ASSERT_EQ( nbAlbums, 2u );

    auto artist2 = T->ml->artist( artist->id() );
    nbAlbums = artist2->nbAlbums();
    ASSERT_EQ( nbAlbums, 2u );
}

static void AllSongs( Tests* T )
{
    auto artist = T->ml->createArtist( "Cannibal Otters" );
    ASSERT_NE( artist, nullptr );

    for (auto i = 1; i <= 3; ++i)
    {
        auto f = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "song" + std::to_string(i) + ".mp3", IMedia::Type::Audio ) );
        auto res = artist->addMedia( *f );
        ASSERT_TRUE( res );
    }

    QueryParameters params;
    params.sort = SortingCriteria::Alpha;
    params.desc = false;
    auto songs = artist->tracks( &params )->all();
    ASSERT_EQ( songs.size(), 3u );

    auto artist2 = T->ml->artist( artist->id() );
    songs = artist2->tracks( &params )->all();
    ASSERT_EQ( songs.size(), 3u );
}

static void GetAll( Tests* T )
{
    auto artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    // Ensure we don't include Unknown Artist // Various Artists
    ASSERT_EQ( artists.size(), 0u );

    for ( int i = 0; i < 5; i++ )
    {
        auto a = T->ml->createArtist( std::to_string( i ) );
        auto alb = T->ml->createAlbum( std::to_string( i ) );
        auto m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "media" + std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        alb->addTrack( m, i + 1, 0, a->id(), nullptr );
        ASSERT_NE( nullptr, alb );
        alb->setAlbumArtist( a );
        ASSERT_NE( a, nullptr );
        a->addMedia( *m );
    }
    artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( artists.size(), 5u );

    auto artists2 = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( artists2.size(), 5u );
}

static void GetAllNoAlbum( Tests* T )
{
    auto artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    // Ensure we don't include Unknown Artist // Various Artists
    ASSERT_EQ( artists.size(), 0u );

    for ( int i = 0; i < 3; i++ )
    {
        auto a = T->ml->createArtist( std::to_string( i ) );
        auto m = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "media" + std::to_string( i ) + ".mp3", IMedia::Type::Audio ) );
        a->addMedia( *m );
    }
    artists = T->ml->artists( ArtistIncluded::AlbumArtistOnly, nullptr )->all();
    ASSERT_EQ( artists.size(), 0u );

    artists = T->ml->artists( ArtistIncluded::AlbumArtistOnly, nullptr )->all();
    ASSERT_EQ( artists.size(), 0u );

    artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( artists.size(), 3u );
}

static void UnknownAlbum( Tests* T )
{
    auto a = T->ml->createArtist( "Explotters in the sky" );
    auto album = a->unknownAlbum();
    ASSERT_EQ( nullptr, album );
    album = a->createUnknownAlbum();

    auto album2 = a->unknownAlbum();
    ASSERT_NE( nullptr, album );

    ASSERT_NE( nullptr, album );
    ASSERT_NE( nullptr, album2 );
    ASSERT_EQ( album->id(), album2->id() );

    a = std::static_pointer_cast<Artist>( T->ml->artist( a->id() ) );
    album2 = a->unknownAlbum();
    ASSERT_NE( nullptr, album2 );
    ASSERT_EQ( album2->id(), album->id() );
}

static void MusicBrainzId( Tests* T )
{
    auto a = T->ml->createArtist( "Otters Never Say Die" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->musicBrainzId(), "" );

    std::string mbId("{this-id-an-id}");
    a->setMusicBrainzId( mbId );
    ASSERT_EQ( a->musicBrainzId(), mbId );

    auto a2 = T->ml->artist( a->id() );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->musicBrainzId(), mbId );
}

static void Search( Tests* T )
{
    auto a1 = T->ml->createArtist( "artist 1" );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    auto a2 = T->ml->createArtist( "artist 2" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    T->ml->createArtist( "dream seaotter" );
    a1->addMedia( *m1 );
    a2->addMedia( *m2 );

    auto artists = T->ml->searchArtists( "artist", ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artists[0]->id(), a1->id() );
    ASSERT_EQ( artists[1]->id(), a2->id() );

    QueryParameters params { SortingCriteria::Default, true };
    artists = T->ml->searchArtists( "artist", ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artists[0]->id(), a2->id() );
    ASSERT_EQ( artists[1]->id(), a1->id() );
}

static void SearchAfterDelete( Tests* T )
{
    auto a = T->ml->createArtist( "artist 1" );
    auto a2 = T->ml->createArtist( "artist 2" );
    auto a3 = T->ml->createArtist( "dream seaotter" );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    a->addMedia( *m1 );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    a2->addMedia( *m2 );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    a3->addMedia( *m3 );

    auto artists = T->ml->searchArtists( "artist", ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );

    T->ml->deleteArtist( a->id() );

    artists = T->ml->searchArtists( "artist", ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
}

static void SortMedia( Tests* T )
{
    auto artist = T->ml->createArtist( "Russian Otters" );
    auto album = T->ml->createAlbum( "album" );
    for (auto i = 1; i <= 3; ++i)
    {
        auto f = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "song" + std::to_string(i) + ".mp3", IMedia::Type::Audio ) );
        album->addTrack( f, i, 0, artist->id(), nullptr );
        f->setDuration( 10 - i );
        f->setReleaseDate( i );
        f->save();
        artist->addMedia( *f );
    }

    QueryParameters params { SortingCriteria::Duration, false };
    auto tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( "song3.mp3", tracks[0]->title() ); // Duration: 8
    ASSERT_EQ( "song2.mp3", tracks[1]->title() ); // Duration: 9
    ASSERT_EQ( "song1.mp3", tracks[2]->title() ); // Duration: 10

    params.desc = true;
    tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( "song1.mp3", tracks[0]->title() );
    ASSERT_EQ( "song2.mp3", tracks[1]->title() );
    ASSERT_EQ( "song3.mp3", tracks[2]->title() );

    params.sort = SortingCriteria::ReleaseDate;
    tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( 3u, tracks[0]->releaseDate() );
    ASSERT_EQ( 2u, tracks[1]->releaseDate() );
    ASSERT_EQ( 1u, tracks[2]->releaseDate() );

    params.desc = false;
    tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( 1u, tracks[0]->releaseDate() );
    ASSERT_EQ( 2u, tracks[1]->releaseDate() );
    ASSERT_EQ( 3u, tracks[2]->releaseDate() );

    // Ensure the fallback sort is alphabetical
    params.sort = SortingCriteria::NbMedia;
    tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( 1u, tracks[0]->trackNumber() );
    ASSERT_EQ( 2u, tracks[1]->trackNumber() );
    ASSERT_EQ( 3u, tracks[2]->trackNumber() );

    params.desc = true;
    tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 3u, tracks.size() );
    ASSERT_EQ( 3u, tracks[0]->trackNumber() );
    ASSERT_EQ( 2u, tracks[1]->trackNumber() );
    ASSERT_EQ( 1u, tracks[2]->trackNumber() );
}

static void SortMediaByAlbum( Tests* T )
{
    auto artist = T->ml->createArtist( "Russian Otters" );

    /*
     * Create 2 albums with the same name to ensure we're correctly grouping
     * the tracks regardless of the album name
     */
    std::shared_ptr<Album> albums[] = {
        std::static_pointer_cast<Album>( T->ml->createAlbum( "album1" ) ),
        std::static_pointer_cast<Album>( T->ml->createAlbum( "album1" ) ),
    };
    // Iterate by track first to interleave ids and ensure we're sorting correctly
    for (auto iTrack = 1; iTrack <= 2; ++iTrack)
    {
        for ( auto iAlbum = 0; iAlbum < 2; ++iAlbum )
        {
            auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "alb" +
                            std::to_string( 9 - iAlbum ) + "_song" +
                            std::to_string( 10 - iTrack) + ".mp3", IMedia::Type::Audio ) );
            artist->addMedia( *f );
            albums[iAlbum]->addTrack( f, iTrack, 0, artist->id(), nullptr );
            f->save();
        }
    }

    QueryParameters params { SortingCriteria::Album, false };
    auto tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 4u, tracks.size() );
    ASSERT_EQ( "alb9_song9.mp3", tracks[0]->title() );
    ASSERT_EQ( 1u, tracks[0]->trackNumber() );
    ASSERT_EQ( "alb9_song8.mp3", tracks[1]->title() );
    ASSERT_EQ( 2u, tracks[1]->trackNumber() );
    ASSERT_EQ( "alb8_song9.mp3", tracks[2]->title() );
    ASSERT_EQ( 1u, tracks[2]->trackNumber() );
    ASSERT_EQ( "alb8_song8.mp3", tracks[3]->title() );
    ASSERT_EQ( 2u, tracks[3]->trackNumber() );

    params.desc = true;
    tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 4u, tracks.size() );
    ASSERT_EQ( "alb8_song9.mp3", tracks[0]->title() );
    ASSERT_EQ( 1u, tracks[0]->trackNumber() );
    ASSERT_EQ( "alb8_song8.mp3", tracks[1]->title() );
    ASSERT_EQ( 2u, tracks[1]->trackNumber() );
    ASSERT_EQ( "alb9_song9.mp3", tracks[2]->title() );
    ASSERT_EQ( 1u, tracks[2]->trackNumber() );
    ASSERT_EQ( "alb9_song8.mp3", tracks[3]->title() );
    ASSERT_EQ( 2u, tracks[3]->trackNumber() );
}

static void SortAlbum( Tests* T )
{
    auto artist = T->ml->createArtist( "Dream Seaotter" );
    auto album1 = T->ml->createAlbum( "album1" );
    auto media1 = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    album1->addTrack( media1, 1, 0, artist->id(), nullptr );
    media1->save();
    album1->setReleaseYear( 2000, false );
    auto album2 = T->ml->createAlbum( "album2" );
    auto media2 = std::static_pointer_cast<Media>( T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    album2->addTrack( media2, 1, 0, artist->id(), nullptr );
    media2->save();
    album2->setReleaseYear( 1000, false );
    auto album3 = T->ml->createAlbum( "album3" );
    auto media3 = std::static_pointer_cast<Media>( T->ml->addMedia( "track3.mp3", IMedia::Type::Audio ) );
    album3->addTrack( media3, 1, 0, artist->id(), nullptr );
    media3->save();
    album3->setReleaseYear( 2000, false );

    album1->setAlbumArtist( artist );
    album2->setAlbumArtist( artist );
    album3->setAlbumArtist( artist );

    // Default order is by descending year, discriminated by lexical order
    auto albums = artist->albums( nullptr )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( album1->id(), albums[0]->id() );
    ASSERT_EQ( album3->id(), albums[1]->id() );
    ASSERT_EQ( album2->id(), albums[2]->id() );

    QueryParameters params { SortingCriteria::Default, true };
    albums = artist->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( album2->id(), albums[0]->id() );
    ASSERT_EQ( album1->id(), albums[1]->id() );
    ASSERT_EQ( album3->id(), albums[2]->id() );

    params.sort = SortingCriteria::Alpha;
    params.desc = false;
    albums = artist->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( album1->id(), albums[0]->id() );
    ASSERT_EQ( album2->id(), albums[1]->id() );
    ASSERT_EQ( album3->id(), albums[2]->id() );

    params.desc = true;
    albums = artist->albums( &params )->all();
    ASSERT_EQ( 3u, albums.size() );
    ASSERT_EQ( album3->id(), albums[0]->id() );
    ASSERT_EQ( album2->id(), albums[1]->id() );
    ASSERT_EQ( album1->id(), albums[2]->id() );
}

static void Sort( Tests* T )
{
    // Keep in mind that artists are only listed when they are marked as album artist at least once
    auto a1 = T->ml->createArtist( "A" );
    auto alb1 = T->ml->createAlbum( "albumA" );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "mediaA.mp3", IMedia::Type::Audio ) );
    alb1->setAlbumArtist( a1 );
    auto a2 = T->ml->createArtist( "B" );
    auto alb2 = T->ml->createAlbum( "albumB" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "mediaB.mp3", IMedia::Type::Audio ) );
    alb2->setAlbumArtist( a2 );

    a1->addMedia( *m1 );
    a2->addMedia( *m2 );

    QueryParameters params { SortingCriteria::Alpha, false };
    auto artists = T->ml->artists( ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( a1->id(), artists[0]->id() );
    ASSERT_EQ( a2->id(), artists[1]->id() );

    params.desc = true;
    artists = T->ml->artists( ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( a1->id(), artists[1]->id() );
    ASSERT_EQ( a2->id(), artists[0]->id() );
}

static void DeleteWhenNoAlbum( Tests* T )
{
    auto artist = T->ml->createArtist( "artist" );
    auto album = T->ml->createAlbum( "album 1" );
    album->setAlbumArtist( artist );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto res = album->addTrack( m1, 1, 1, artist->id(), nullptr );
    ASSERT_TRUE( res );
    artist->addMedia( *m1 );

    auto artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );

    T->ml->deleteMedia( m1->id() );
    artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 0u, artists.size() );

    artists = T->ml->artists( ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 0u, artists.size() );
}

static void UpdateNbTracks( Tests* T )
{
    auto artist = T->ml->createArtist( "artist" );
    ASSERT_EQ( 0u, artist->nbTracks() );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    artist->addMedia( *m1 );

    artist = std::static_pointer_cast<Artist>( T->ml->artist( artist->id() ) );
    ASSERT_EQ( 1u, artist->nbTracks() );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    artist->addMedia( *m2 );

    artist = std::static_pointer_cast<Artist>( T->ml->artist( artist->id() ) );
    ASSERT_EQ( 2u, artist->nbTracks() );

    T->ml->deleteMedia( m1->id() );

    artist = std::static_pointer_cast<Artist>( T->ml->artist( artist->id() ) );
    ASSERT_EQ( 1u, artist->nbTracks() );

    T->ml->deleteMedia( m2->id() );

    artist = std::static_pointer_cast<Artist>( T->ml->artist( artist->id() ) );
    ASSERT_EQ( nullptr, artist );
}

static void SortTracksMultiDisc( Tests* T )
{
    MediaPtr media[6];
    auto album = T->ml->createAlbum( "album" );
    auto artist = T->ml->createArtist( "artist" );
    for ( auto i = 0; i < 3; ++i )
    {
        auto j = i * 2;
        auto media1 = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "track_" + std::to_string( j ) + ".mp3", IMedia::Type::Audio ) );
        auto media2 = std::static_pointer_cast<Media>(
                    T->ml->addMedia( "track_" + std::to_string( j + 1 ) + ".mp3", IMedia::Type::Audio ) );
        album->addTrack( media1, i, 1, artist->id(), nullptr );
        media1->save();
        album->addTrack( media2, i, 2, artist->id(), nullptr );
        media2->save();
        artist->addMedia( *media1 );
        artist->addMedia( *media2 );
        media[j] = media1;
        media[j + 1] = media2;
    }
    /*
     * media is now:
     * [ Disc 1 - Track 1 ]
     * [ Disc 2 - Track 1 ]
     * [ Disc 1 - Track 2 ]
     * [ Disc 2 - Track 2 ]
     * [ Disc 1 - Track 3 ]
     * [ Disc 2 - Track 3 ]
     */
    QueryParameters params { SortingCriteria::Album, false };
    auto tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 6u, tracks.size() );
    ASSERT_EQ( media[0]->id(), tracks[0]->id() );
    ASSERT_EQ( media[2]->id(), tracks[1]->id() );
    ASSERT_EQ( media[4]->id(), tracks[2]->id() );
    ASSERT_EQ( media[1]->id(), tracks[3]->id() );
    ASSERT_EQ( media[3]->id(), tracks[4]->id() );
    ASSERT_EQ( media[5]->id(), tracks[5]->id() );

    params.desc = true;
    tracks = artist->tracks( &params )->all();
    // Ordering by album doesn't invert tracks ordering (anymore)
    ASSERT_EQ( media[0]->id(), tracks[0]->id() );
    ASSERT_EQ( media[2]->id(), tracks[1]->id() );
    ASSERT_EQ( media[4]->id(), tracks[2]->id() );
    ASSERT_EQ( media[1]->id(), tracks[3]->id() );
    ASSERT_EQ( media[3]->id(), tracks[4]->id() );
    ASSERT_EQ( media[5]->id(), tracks[5]->id() );
}

static void CheckQuery( Tests* T )
{
    auto artist1 = T->ml->createArtist( "artist1" );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    artist1->addMedia( *m1 );
    auto artist2 = T->ml->createArtist( "artist2" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    artist2->addMedia( *m2 );

    auto query = T->ml->artists( ArtistIncluded::All, nullptr );
    auto artists = query->items( 1, 0 );
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( artist1->id(), artists[0]->id() );
    artists = query->items( 1, 1 );
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( artist2->id(), artists[0]->id() );
    artists = query->all();
    ASSERT_EQ( 2u, artists.size() );
}

static void SearchAlbums( Tests* T )
{
    auto artist = T->ml->createArtist( "artist" );
    auto alb1 = T->ml->createAlbum( "album" );
    alb1->setAlbumArtist( artist );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    alb1->addTrack( m1, 1, 0, 0, nullptr );
    m1->save();
    auto alb2 = T->ml->createAlbum( "other album" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    alb2->addTrack( m2, 1, 0, 0, nullptr );
    m2->save();

    auto allAlbums = T->ml->searchAlbums( "album", nullptr )->all();
    ASSERT_EQ( 2u, allAlbums.size() );

    auto artistAlbums = artist->searchAlbums( "album", nullptr )->all();
    ASSERT_EQ( 1u, artistAlbums.size() );
    ASSERT_EQ( alb1->id(), artistAlbums[0]->id() );
}

static void SearchTracks( Tests* T )
{
    auto artist1 = T->ml->createArtist( "artist" );
    auto album1 = T->ml->createAlbum( "album" );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", Media::Type::Audio ) );
    m1->setTitle( "sea otter", true );
    auto res = album1->addTrack( m1, 1, 0, artist1->id(), nullptr );
    ASSERT_TRUE( res );
    m1->save();

    auto artist2 = T->ml->createArtist( "artist2" );
    auto album2 = T->ml->createAlbum( "album2" );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    m2->setTitle( "sea cucumber", true );
    album2->addTrack( m2, 1, 0, artist2->id(), nullptr );
    m2->save();

    auto allTracks = T->ml->searchAudio( "sea" )->all();
    ASSERT_EQ( 2u, allTracks.size() );

    auto artistTracks = artist1->searchTracks( "sea", nullptr )->all();
    ASSERT_EQ( 1u, artistTracks.size() );
    ASSERT_EQ( m1->id(), artistTracks[0]->id() );
}

static void SearchAll( Tests* T )
{
    auto artist1 = T->ml->createArtist( "artist 1" );
    auto artist2 = T->ml->createArtist( "artist 2" );

    auto album1 = T->ml->createAlbum( "album1" );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    album1->addTrack( m1, 1, 0, artist1->id(), nullptr );
    album1->addTrack( m2, 2, 0, artist1->id(), nullptr );
    m1->save();
    m2->save();
    artist1->addMedia( *m1 );
    artist1->addMedia( *m2 );
    // Artist 1 now has 0 album but 2 tracks

    auto album2 = T->ml->createAlbum( "album2" );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );
    album2->addTrack( m3, 1, 0, artist2->id(), nullptr );
    album2->setAlbumArtist( artist2 );
    artist2->addMedia( *m3 );
    m3->save();

    auto artists = T->ml->searchArtists( "artist", ArtistIncluded::AlbumArtistOnly, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );

    artists = T->ml->searchArtists( "artist", ArtistIncluded::All, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );
}

static void CheckDbModel( Tests* T )
{
    auto res = Artist::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void SortByNbAlbums( Tests* T )
{
    auto artist1 = T->ml->createArtist( "Z artist" );
    auto artist2 = T->ml->createArtist( "A artist" );
    ASSERT_NON_NULL( artist1 );
    ASSERT_NON_NULL( artist2 );

    auto art1alb1 = T->ml->createAlbum( "art1alb1" );
    auto art2alb1 = T->ml->createAlbum( "art2alb1" );
    auto art2alb2 = T->ml->createAlbum( "art2alb2" );
    ASSERT_NON_NULL( art1alb1 );
    ASSERT_NON_NULL( art2alb1 );
    ASSERT_NON_NULL( art2alb2 );

    auto res = art1alb1->setAlbumArtist( artist1 );
    ASSERT_TRUE( res );
    res = art2alb1->setAlbumArtist( artist2 );
    ASSERT_TRUE( res );
    res = art2alb2->setAlbumArtist( artist2 );
    ASSERT_TRUE( res );

    QueryParameters params{};
    params.sort = SortingCriteria::NbAlbum;
    params.desc = false;
    // Bypass the is_present check, since there are no track present
    params.includeMissing = true;
    auto artists = T->ml->artists( ArtistIncluded::AlbumArtistOnly, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artist1->id(), artists[0]->id() );
    ASSERT_EQ( artist2->id(), artists[1]->id() );

    params.desc = true;
    artists = T->ml->artists( ArtistIncluded::AlbumArtistOnly, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artist2->id(), artists[0]->id() );
    ASSERT_EQ( artist1->id(), artists[1]->id() );
}

static void SortByNbTracks( Tests* T )
{
    auto artist1 = T->ml->createArtist( "A artist" );
    auto artist2 = T->ml->createArtist( "Z artist" );
    ASSERT_NON_NULL( artist1 );
    ASSERT_NON_NULL( artist2 );

    auto album = T->ml->createAlbum( "compilation" );

    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mp3", IMedia::Type::Audio ) );
    auto m3 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media3.mp3", IMedia::Type::Audio ) );

    album->addTrack( m1, 1, 1, artist1->id(), nullptr );
    m1->save();
    artist1->addMedia( *m1 );
    album->addTrack( m2, 2, 1, artist1->id(), nullptr );
    m2->save();
    artist1->addMedia( *m2 );
    album->addTrack( m3, 3, 1, artist2->id(), nullptr );
    m3->save();
    artist2->addMedia( *m3 );

    QueryParameters params{};
    params.sort = SortingCriteria::TrackNumber;
    params.desc = false;
    auto artists = T->ml->artists( ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artist2->id(), artists[0]->id() );
    ASSERT_EQ( artist1->id(), artists[1]->id() );

    params.desc = true;
    artists = T->ml->artists( ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artist1->id(), artists[0]->id() );
    ASSERT_EQ( artist2->id(), artists[1]->id() );
}

static void SortByLastPlayedDate( Tests* T )
{
    auto artist1 = T->ml->createArtist( "A artist" );
    auto artist2 = T->ml->createArtist( "Z artist" );

    auto a1m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "a1m1.mp3", IMedia::Type::Audio ) );
    auto a1m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "a1m2.mp3", IMedia::Type::Audio ) );
    auto a2m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "a2m1.mp3", IMedia::Type::Audio ) );
    auto a2m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "a2m2.mp3", IMedia::Type::Audio ) );
    ASSERT_NON_NULL( a1m1 );
    ASSERT_NON_NULL( a1m2 );
    ASSERT_NON_NULL( a2m1 );
    ASSERT_NON_NULL( a2m2 );

    auto res = artist1->addMedia( *a1m1 );
    ASSERT_TRUE( res );
    res = artist1->addMedia( *a1m2 );
    ASSERT_TRUE( res );
    res = artist2->addMedia( *a2m1 );
    ASSERT_TRUE( res );
    res = artist2->addMedia( *a2m2 );
    ASSERT_TRUE( res );

    res = T->ml->setMediaLastPlayedDate( a1m1->id(), 0 );
    ASSERT_TRUE( res );
    res = T->ml->setMediaLastPlayedDate( a1m2->id(), 1 );
    ASSERT_TRUE( res );
    res = T->ml->setMediaLastPlayedDate( a2m1->id(), 0 );
    ASSERT_TRUE( res );
    res = T->ml->setMediaLastPlayedDate( a2m2->id(), 0 );
    ASSERT_TRUE( res );

    QueryParameters params{};
    params.sort = SortingCriteria::LastPlaybackDate;
    params.desc = false;

    auto artists = T->ml->artists( ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artist2->id(), artists[0]->id() );
    ASSERT_EQ( artist1->id(), artists[1]->id() );

    params.desc = true;
    artists = T->ml->artists( ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artist1->id(), artists[0]->id() );
    ASSERT_EQ( artist2->id(), artists[1]->id() );

    res = T->ml->setMediaLastPlayedDate( a2m1->id(), 10 );
    ASSERT_TRUE( res );

    params.desc = false;
    artists = T->ml->artists( ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artist1->id(), artists[0]->id() );
    ASSERT_EQ( artist2->id(), artists[1]->id() );

    res = T->ml->setMediaLastPlayedDate( a1m1->id(), 100 );
    ASSERT_TRUE( res );

    artists = T->ml->artists( ArtistIncluded::All, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artist2->id(), artists[0]->id() );
    ASSERT_EQ( artist1->id(), artists[1]->id() );
}

int main( int ac, char** av )
{
    INIT_TESTS( Artist )

    ADD_TEST( Create );
    ADD_TEST( ShortBio );
    ADD_TEST( ArtworkMrl );
    ADD_TEST( GetThumbnail );
    ADD_TEST( Albums );
    ADD_TEST( NbAlbums );
    ADD_TEST( AllSongs );
    ADD_TEST( GetAll );
    ADD_TEST( GetAllNoAlbum );
    ADD_TEST( UnknownAlbum );
    ADD_TEST( MusicBrainzId );
    ADD_TEST( Search );
    ADD_TEST( SearchAfterDelete );
    ADD_TEST( SortMedia );
    ADD_TEST( SortMediaByAlbum );
    ADD_TEST( SortAlbum );
    ADD_TEST( Sort );
    ADD_TEST( DeleteWhenNoAlbum );
    ADD_TEST( UpdateNbTracks );
    ADD_TEST( SortTracksMultiDisc );
    ADD_TEST( CheckQuery );
    ADD_TEST( SearchAlbums );
    ADD_TEST( SearchTracks );
    ADD_TEST( SearchAll );
    ADD_TEST( CheckDbModel );
    ADD_TEST( SortByNbAlbums );
    ADD_TEST( SortByNbTracks );
    ADD_TEST( SortByLastPlayedDate );

    END_TESTS
}
