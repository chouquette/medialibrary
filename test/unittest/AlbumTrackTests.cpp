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
#include "Media.h"
#include "Genre.h"

class AlbumTracks : public Tests
{
};

TEST_F( AlbumTracks, Create )
{
    auto album = ml->createAlbum( "album" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3" ) );
    auto track = album->addTrack( f, 1, 10, 0, nullptr );
    f->save();
    ASSERT_NE( nullptr, track );
    ASSERT_EQ( 10u, track->discNumber() );

    Reload();

    f = std::static_pointer_cast<Media>( ml->media( f->id() ) );
    ASSERT_EQ( 10u, f->albumTrack()->discNumber() );
}

TEST_F( AlbumTracks, SetGenre )
{
    auto a = ml->createAlbum( "album" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "track.mp3" ) );
    auto t = a->addTrack( f, 1, 0, 0, nullptr );
    f->save();
    auto genre = ml->createGenre( "happy underground post progressive death metal" );

    ASSERT_EQ( nullptr, t->genre() );
    t->setGenre( genre );
    ASSERT_EQ( t->genre()->name(), "happy underground post progressive death metal" );

    Reload();

    a = std::static_pointer_cast<Album>( ml->album( a->id() ) );
    auto tracks = a->tracks( nullptr )->all();
    ASSERT_EQ( tracks.size(), 1u );
    auto t2 = tracks[0];
    ASSERT_EQ( t->genre()->id(), t2->albumTrack()->genre()->id() );
}

TEST_F( AlbumTracks, Media )
{
    auto album = ml->createAlbum( "album" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3" ) );
    auto track = album->addTrack( f, 1, 10, 0, nullptr );
    f->save();

    auto media = track->media();
    ASSERT_NE( nullptr, media );
    ASSERT_EQ( media->id(), f->id() );

    Reload();

    track = ml->albumTrack( track->id() );
    media = track->media();
    ASSERT_NE( nullptr, media );
    ASSERT_EQ( media->id(), f->id() );
}

TEST_F( AlbumTracks, Album )
{
    auto album = ml->createAlbum( "album" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3" ) );
    auto track = album->addTrack( f, 1, 0, 0, nullptr );
    f->save();

    auto albumFromTrack = track->album();
    ASSERT_EQ( album, albumFromTrack );

    Reload();

    track = ml->albumTrack( track->id() );
    albumFromTrack = track->album();
    auto a2 = ml->album( album->id() );
    // Fetching this value twice seems to be problematic on Android.
    // Ensure it works for other platforms at least
    auto aft2 = track->album();
    ASSERT_EQ( albumFromTrack, a2 );
    ASSERT_EQ( aft2, a2 );
}
