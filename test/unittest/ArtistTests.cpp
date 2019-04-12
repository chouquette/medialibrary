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

#include "Artist.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Media.h"

class Artists : public Tests
{
};

TEST_F( Artists, Create )
{
    auto a = ml->createArtist( "Flying Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->name(), "Flying Otters" );

    Reload();

    auto a2 = ml->artist( a->id() );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->name(), "Flying Otters" );
}

TEST_F( Artists, CreateDefaults )
{
    // Ensure this won't fail due to duplicate insertions
    // We just reload, which will call the initialization routine again.
    // This is implicitely tested by all other tests, though it seems better
    // to have an explicit one. We might also just run the request twice from here
    // sometime in the future.
    Reload();
}

TEST_F( Artists, ShortBio )
{
    auto a = ml->createArtist( "Raging Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->shortBio(), "" );

    std::string bio("An otter based post-rock band");
    a->setShortBio( bio );
    ASSERT_EQ( a->shortBio(), bio );

    Reload();

    auto a2 = ml->artist( a->id() );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->shortBio(), bio );
}

TEST_F( Artists, ArtworkMrl )
{
    auto a = ml->createArtist( "Dream seaotter" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->thumbnailMrl(), "" );
    ASSERT_FALSE( a->isThumbnailGenerated() );

    std::string artwork("/tmp/otter.png");
    a->setArtworkMrl( artwork, Thumbnail::Origin::UserProvided, false );
    ASSERT_EQ( a->thumbnailMrl(), artwork );
    ASSERT_TRUE( a->isThumbnailGenerated() );

    Reload();

    auto a2 = ml->artist( a->id() );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->thumbnailMrl(), artwork );
    ASSERT_TRUE( a->isThumbnailGenerated() );
}

TEST_F( Artists, Thumbnail )
{
    auto a = ml->createArtist( "artist" );
    auto t = a->thumbnail();
    ASSERT_EQ( nullptr, t );

    std::string mrl = "/path/to/sea/otter/artwork.png";
    auto res = a->setArtworkMrl( mrl, Thumbnail::Origin::UserProvided, false );
    ASSERT_TRUE( res );

    t = a->thumbnail();
    ASSERT_NE( nullptr, t );
    ASSERT_EQ( mrl, t->mrl() );

    Reload();

    a = std::static_pointer_cast<Artist>( ml->artist( a->id() ) );
    t = a->thumbnail();
    ASSERT_NE( nullptr, t );
    ASSERT_EQ( mrl, t->mrl() );
}

// Test the number of albums based on the artist tracks
TEST_F( Artists, Albums )
{
    auto artist = ml->createArtist( "Cannibal Otters" );
    auto album1 = ml->createAlbum( "album1" );
    auto album2 = ml->createAlbum( "album2" );

    ASSERT_NE( artist, nullptr );
    ASSERT_NE( album1, nullptr );
    ASSERT_NE( album2, nullptr );

    auto media1 = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3" ) );
    ASSERT_NE( nullptr, media1 );
    album1->addTrack( media1, 1, 0, artist->id(), nullptr );
    auto media2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3" ) );
    ASSERT_NE( nullptr, media2 );
    album2->addTrack( media2, 1, 0, artist->id(), nullptr );

    auto media3 = std::static_pointer_cast<Media>( ml->addMedia( "track3.mp3" ) );
    ASSERT_NE( nullptr, media3 );
    album2->addTrack( media3, 2, 0, artist->id(), nullptr );

    auto media4 = std::static_pointer_cast<Media>( ml->addMedia( "track4.mp3" ) );
    ASSERT_NE( nullptr, media4 );
    album2->addTrack( media4, 3, 0, artist->id(), nullptr );

    album1->setAlbumArtist( artist );
    album2->setAlbumArtist( artist );

    auto query = artist->albums( nullptr );
    ASSERT_EQ( 2u, query->count() );
    auto albums = query->all();
    ASSERT_EQ( albums.size(), 2u );

    Reload();

    auto artist2 = ml->artist( artist->id() );
    auto albums2 = artist2->albums( nullptr )->all();
    ASSERT_EQ( albums2.size(), 2u );
}

// Test the nb_album DB field (ie. we don't need to create tracks for this test)
TEST_F( Artists, NbAlbums )
{
    auto artist = ml->createArtist( "Cannibal Otters" );
    auto album1 = ml->createAlbum( "album1" );
    auto album2 = ml->createAlbum( "album2" );

    ASSERT_NE( artist, nullptr );
    ASSERT_NE( album1, nullptr );
    ASSERT_NE( album2, nullptr );

    album1->setAlbumArtist( artist );
    album2->setAlbumArtist( artist );

    auto nbAlbums = artist->nbAlbums();
    ASSERT_EQ( nbAlbums, 2u );

    Reload();

    auto artist2 = ml->artist( artist->id() );
    nbAlbums = artist2->nbAlbums();
    ASSERT_EQ( nbAlbums, 2u );
}

TEST_F( Artists, AllSongs )
{
    auto artist = ml->createArtist( "Cannibal Otters" );
    ASSERT_NE( artist, nullptr );

    for (auto i = 1; i <= 3; ++i)
    {
        auto f = std::static_pointer_cast<Media>( ml->addMedia( "song" + std::to_string(i) + ".mp3" ) );
        auto res = artist->addMedia( *f );
        ASSERT_TRUE( res );
    }

    QueryParameters params;
    params.sort = SortingCriteria::Alpha;
    params.desc = false;
    auto songs = artist->tracks( &params )->all();
    ASSERT_EQ( songs.size(), 3u );

    Reload();

    auto artist2 = ml->artist( artist->id() );
    songs = artist2->tracks( &params )->all();
    ASSERT_EQ( songs.size(), 3u );
}

TEST_F( Artists, GetAll )
{
    auto artists = ml->artists( true, nullptr )->all();
    // Ensure we don't include Unknown Artist // Various Artists
    ASSERT_EQ( artists.size(), 0u );

    for ( int i = 0; i < 5; i++ )
    {
        auto a = ml->createArtist( std::to_string( i ) );
        auto alb = ml->createAlbum( std::to_string( i ) );
        auto m = std::static_pointer_cast<Media>( ml->addMedia( "media" + std::to_string( i ) + ".mp3" ) );
        alb->addTrack( m, i + 1, 0, a->id(), nullptr );
        ASSERT_NE( nullptr, alb );
        alb->setAlbumArtist( a );
        ASSERT_NE( a, nullptr );
        a->updateNbTrack( 1 );
    }
    artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( artists.size(), 5u );

    Reload();

    auto artists2 = ml->artists( true, nullptr )->all();
    ASSERT_EQ( artists2.size(), 5u );
}

TEST_F( Artists, GetAllNoAlbum )
{
    auto artists = ml->artists( true, nullptr )->all();
    // Ensure we don't include Unknown Artist // Various Artists
    ASSERT_EQ( artists.size(), 0u );

    for ( int i = 0; i < 3; i++ )
    {
        auto a = ml->createArtist( std::to_string( i ) );
        a->updateNbTrack( 1 );
    }
    artists = ml->artists( false, nullptr )->all();
    ASSERT_EQ( artists.size(), 0u );

    Reload();

    artists = ml->artists( false, nullptr )->all();
    ASSERT_EQ( artists.size(), 0u );

    artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( artists.size(), 3u );
}

TEST_F( Artists, UnknownAlbum )
{
    auto a = ml->createArtist( "Explotters in the sky" );
    auto album = a->unknownAlbum();
    auto album2 = a->unknownAlbum();

    ASSERT_NE( nullptr, album );
    ASSERT_NE( nullptr, album2 );
    ASSERT_EQ( album->id(), album2->id() );

    Reload();

    a = std::static_pointer_cast<Artist>( ml->artist( a->id() ) );
    album2 = a->unknownAlbum();
    ASSERT_NE( nullptr, album2 );
    ASSERT_EQ( album2->id(), album->id() );
}

TEST_F( Artists, MusicBrainzId )
{
    auto a = ml->createArtist( "Otters Never Say Die" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->musicBrainzId(), "" );

    std::string mbId("{this-id-an-id}");
    a->setMusicBrainzId( mbId );
    ASSERT_EQ( a->musicBrainzId(), mbId );

    Reload();

    auto a2 = ml->artist( a->id() );
    ASSERT_NE( a2, nullptr );
    ASSERT_EQ( a2->musicBrainzId(), mbId );
}

TEST_F( Artists, Search )
{
    auto a1 = ml->createArtist( "artist 1" );
    auto a2 = ml->createArtist( "artist 2" );
    ml->createArtist( "dream seaotter" );
    a1->updateNbTrack( 1 );
    a2->updateNbTrack( 1 );

    auto artists = ml->searchArtists( "artist", true, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artists[0]->id(), a1->id() );
    ASSERT_EQ( artists[1]->id(), a2->id() );

    QueryParameters params { SortingCriteria::Default, true };
    artists = ml->searchArtists( "artist", true, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( artists[0]->id(), a2->id() );
    ASSERT_EQ( artists[1]->id(), a1->id() );
}

TEST_F( Artists, SearchAfterDelete )
{
    auto a = ml->createArtist( "artist 1" );
    auto a2 = ml->createArtist( "artist 2" );
    auto a3 = ml->createArtist( "dream seaotter" );
    a->updateNbTrack( 1 );
    a2->updateNbTrack( 1 );
    a3->updateNbTrack( 1 );

    auto artists = ml->searchArtists( "artist", true, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );

    ml->deleteArtist( a->id() );

    artists = ml->searchArtists( "artist", true, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );
}

TEST_F( Artists, SortMedia )
{
    auto artist = ml->createArtist( "Russian Otters" );

    for (auto i = 1; i <= 3; ++i)
    {
        auto f = std::static_pointer_cast<Media>( ml->addMedia( "song" + std::to_string(i) + ".mp3" ) );
        f->setDuration( 10 - i );
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
}

TEST_F( Artists, SortMediaByAlbum )
{
    auto artist = ml->createArtist( "Russian Otters" );

    std::shared_ptr<Album> albums[] = {
        std::static_pointer_cast<Album>( ml->createAlbum( "album1" ) ),
        std::static_pointer_cast<Album>( ml->createAlbum( "album2" ) ),
    };
    // Iterate by track first to interleave ids and ensure we're sorting correctly
    for (auto iTrack = 1; iTrack <= 2; ++iTrack)
    {
        for ( auto iAlbum = 0; iAlbum < 2; ++iAlbum )
        {
            auto f = std::static_pointer_cast<Media>( ml->addMedia( "alb" +
                            std::to_string( 9 - iAlbum ) + "_song" +
                            std::to_string( 10 - iTrack) + ".mp3" ) );
            artist->addMedia( *f );
            albums[iAlbum]->addTrack( f, iTrack, 0, artist->id(), nullptr );
            f->save();
        }
    }

    QueryParameters params { SortingCriteria::Album, false };
    auto tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 4u, tracks.size() );
    ASSERT_EQ( "alb9_song9.mp3", tracks[0]->title() );
    ASSERT_EQ( 1u, tracks[0]->albumTrack()->trackNumber() );
    ASSERT_EQ( "alb9_song8.mp3", tracks[1]->title() );
    ASSERT_EQ( 2u, tracks[1]->albumTrack()->trackNumber() );
    ASSERT_EQ( "alb8_song9.mp3", tracks[2]->title() );
    ASSERT_EQ( 1u, tracks[2]->albumTrack()->trackNumber() );
    ASSERT_EQ( "alb8_song8.mp3", tracks[3]->title() );
    ASSERT_EQ( 2u, tracks[3]->albumTrack()->trackNumber() );

    params.desc = true;
    tracks = artist->tracks( &params )->all();
    ASSERT_EQ( 4u, tracks.size() );
    ASSERT_EQ( "alb8_song9.mp3", tracks[0]->title() );
    ASSERT_EQ( 1u, tracks[0]->albumTrack()->trackNumber() );
    ASSERT_EQ( "alb8_song8.mp3", tracks[1]->title() );
    ASSERT_EQ( 2u, tracks[1]->albumTrack()->trackNumber() );
    ASSERT_EQ( "alb9_song9.mp3", tracks[2]->title() );
    ASSERT_EQ( 1u, tracks[2]->albumTrack()->trackNumber() );
    ASSERT_EQ( "alb9_song8.mp3", tracks[3]->title() );
    ASSERT_EQ( 2u, tracks[3]->albumTrack()->trackNumber() );
}

TEST_F( Artists, SortAlbum )
{
    auto artist = ml->createArtist( "Dream Seaotter" );
    auto album1 = ml->createAlbum( "album1" );
    auto media1 = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3" ) );
    album1->addTrack( media1, 1, 0, artist->id(), nullptr );
    album1->setReleaseYear( 2000, false );
    auto album2 = ml->createAlbum( "album2" );
    auto media2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3" ) );
    album2->addTrack( media2, 1, 0, artist->id(), nullptr );
    album2->setReleaseYear( 1000, false );
    auto album3 = ml->createAlbum( "album3" );
    auto media3 = std::static_pointer_cast<Media>( ml->addMedia( "track3.mp3" ) );
    album3->addTrack( media3, 1, 0, artist->id(), nullptr );
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

TEST_F( Artists, Sort )
{
    // Keep in mind that artists are only listed when they are marked as album artist at least once
    auto a1 = ml->createArtist( "A" );
    auto alb1 = ml->createAlbum( "albumA" );
    alb1->setAlbumArtist( a1 );
    auto a2 = ml->createArtist( "B" );
    auto alb2 = ml->createAlbum( "albumB" );
    alb2->setAlbumArtist( a2 );

    a1->updateNbTrack( 1 );
    a2->updateNbTrack( 2 );

    QueryParameters params { SortingCriteria::Alpha, false };
    auto artists = ml->artists( true, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( a1->id(), artists[0]->id() );
    ASSERT_EQ( a2->id(), artists[1]->id() );

    params.desc = true;
    artists = ml->artists( true, &params )->all();
    ASSERT_EQ( 2u, artists.size() );
    ASSERT_EQ( a1->id(), artists[1]->id() );
    ASSERT_EQ( a2->id(), artists[0]->id() );
}

TEST_F( Artists, DeleteWhenNoAlbum )
{
    auto artist = ml->createArtist( "artist" );
    auto album = ml->createAlbum( "album 1" );
    album->setAlbumArtist( artist );
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3" ) );
    auto track1 = album->addTrack( m1, 1, 1, artist->id(), nullptr );
    artist->updateNbTrack( 1 );

    auto artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );

    ml->deleteTrack( track1->id() );
    artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( 0u, artists.size() );

    Reload();

    artists = ml->artists( true, nullptr )->all();
    ASSERT_EQ( 0u, artists.size() );
}

TEST_F( Artists, UpdateNbTracks )
{
    auto artist = ml->createArtist( "artist" );
    ASSERT_EQ( 0u, artist->nbTracks() );
    artist->updateNbTrack( 1 );
    ASSERT_EQ( 1u, artist->nbTracks() );

    Reload();

    artist = std::static_pointer_cast<Artist>( ml->artist( artist->id() ) );
    ASSERT_EQ( 1u, artist->nbTracks() );

    artist->updateNbTrack( -1 );
    ASSERT_EQ( 0u, artist->nbTracks() );

    Reload();

    artist = std::static_pointer_cast<Artist>( ml->artist( artist->id() ) );
    ASSERT_EQ( 0u, artist->nbTracks() );
}

TEST_F( Artists, SortTracksMultiDisc )
{
    MediaPtr media[6];
    auto album = ml->createAlbum( "album" );
    auto artist = ml->createArtist( "artist" );
    for ( auto i = 0; i < 3; ++i )
    {
        auto j = i * 2;
        auto media1 = std::static_pointer_cast<Media>( ml->addMedia( "track_" + std::to_string( j ) + ".mp3" ) );
        auto media2 = std::static_pointer_cast<Media>( ml->addMedia( "track_" + std::to_string( j + 1 ) + ".mp3" ) );
        album->addTrack( media1, i, 1, artist->id(), nullptr );
        album->addTrack( media2, i, 2, artist->id(), nullptr );
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

TEST_F( Artists, Query )
{
    auto artist1 = ml->createArtist( "artist1" );
    artist1->updateNbTrack( 1 );
    auto artist2 = ml->createArtist( "artist2" );
    artist2->updateNbTrack( 1 );

    auto query = ml->artists( true, nullptr );
    auto artists = query->items( 1, 0 );
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( artist1->id(), artists[0]->id() );
    artists = query->items( 1, 1 );
    ASSERT_EQ( 1u, artists.size() );
    ASSERT_EQ( artist2->id(), artists[0]->id() );
    artists = query->all();
    ASSERT_EQ( 2u, artists.size() );
}

TEST_F( Artists, SearchAlbums )
{
    auto artist = ml->createArtist( "artist" );
    auto alb1 = ml->createAlbum( "album" );
    alb1->setAlbumArtist( artist );
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "media1.mp3" ) );
    alb1->addTrack( m1, 1, 0, 0, nullptr );
    m1->save();
    auto alb2 = ml->createAlbum( "other album" );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mp3" ) );
    alb2->addTrack( m2, 1, 0, 0, nullptr );
    m2->save();

    auto allAlbums = ml->searchAlbums( "album", nullptr )->all();
    ASSERT_EQ( 2u, allAlbums.size() );

    auto artistAlbums = artist->searchAlbums( "album", nullptr )->all();
    ASSERT_EQ( 1u, artistAlbums.size() );
    ASSERT_EQ( alb1->id(), artistAlbums[0]->id() );
}

TEST_F( Artists, SearchTracks )
{
    auto artist1 = ml->createArtist( "artist" );
    auto album1 = ml->createAlbum( "album" );
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3", Media::Type::Audio ) );
    m1->setTitleBuffered( "sea otter" );
    auto track1 = album1->addTrack( m1, 1, 0, artist1->id(), nullptr );
    m1->save();

    auto artist2 = ml->createArtist( "artist2" );
    auto album2 = ml->createAlbum( "album2" );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    m2->setTitleBuffered( "sea cucumber" );
    album2->addTrack( m2, 1, 0, artist2->id(), nullptr );
    m2->save();

    auto allTracks = ml->searchAudio( "sea" )->all();
    ASSERT_EQ( 2u, allTracks.size() );

    auto artistTracks = artist1->searchTracks( "sea", nullptr )->all();
    ASSERT_EQ( 1u, artistTracks.size() );
    ASSERT_EQ( track1->id(), artistTracks[0]->id() );
}

TEST_F( Artists, SearchAll )
{
    auto artist1 = ml->createArtist( "artist 1" );
    auto artist2 = ml->createArtist( "artist 2" );

    auto album1 = ml->createAlbum( "album1" );
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "media1.mp3" ) );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mp3" ) );
    album1->addTrack( m1, 1, 0, artist1->id(), nullptr );
    album1->addTrack( m2, 2, 0, artist1->id(), nullptr );
    m1->save();
    m2->save();
    artist1->updateNbTrack( 2 );
    // Artist 1 now has 0 album but 2 tracks

    auto album2 = ml->createAlbum( "album2" );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "media3.mp3" ) );
    album2->addTrack( m3, 1, 0, artist2->id(), nullptr );
    album2->setAlbumArtist( artist2 );
    artist2->updateNbTrack( 1 );
    m3->save();

    auto artists = ml->searchArtists( "artist", false, nullptr )->all();
    ASSERT_EQ( 1u, artists.size() );

    artists = ml->searchArtists( "artist", true, nullptr )->all();
    ASSERT_EQ( 2u, artists.size() );
}
