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
#include "AlbumTrack.h"
#include "Artist.h"
#include "Media.h"
#include "Genre.h"

static void Create( Tests *T )
{
    auto album = T->ml->createAlbum( "album" );
    auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto track = album->addTrack( f, 1, 10, 0, nullptr );
    f->save();
    ASSERT_NE( nullptr, track );
    ASSERT_EQ( 10u, track->discNumber() );
    ASSERT_EQ( nullptr, track->artist() );
    ASSERT_EQ( 0, track->artistId() );
    ASSERT_EQ( album->id(), track->albumId() );
    ASSERT_EQ( 0, track->genreId() );
    ASSERT_EQ( nullptr, track->genre() );

    f = std::static_pointer_cast<Media>( T->ml->media( f->id() ) );
    ASSERT_EQ( 10u, f->albumTrack()->discNumber() );
}

static void GetAlbum( Tests *T )
{
    auto album = T->ml->createAlbum( "album" );
    auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto track = album->addTrack( f, 1, 0, 0, nullptr );
    f->save();

    auto albumFromTrack = track->album();
    ASSERT_EQ( album->id(), albumFromTrack->id() );

    track = T->ml->albumTrack( track->id() );
    albumFromTrack = track->album();
    auto a2 = T->ml->album( album->id() );
    // Fetching this value twice seems to be problematic on Android.
    // Ensure it works for other platforms at least
    auto aft2 = track->album();
    ASSERT_EQ( albumFromTrack->id(), a2->id() );
    ASSERT_EQ( aft2->id(), a2->id() );
}

static void CheckDbModel( Tests *T )
{
    auto res = AlbumTrack::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void DeleteByMediaId( Tests *T )
{
    auto album = T->ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    auto track = album->addTrack( m, 1, 1, 0, nullptr );
    auto track2 = album->addTrack( m2, 2, 1, 0, nullptr );
    m->save();
    m2->save();

    auto tracks = album->tracks( nullptr )->all();
    ASSERT_EQ( 2u, tracks.size() );

    auto res = AlbumTrack::deleteByMediaId( T->ml.get(), m->id() );
    ASSERT_TRUE( res );

    tracks = album->tracks( nullptr )->all();
    ASSERT_EQ( 1u, tracks.size() );

    res = AlbumTrack::deleteByMediaId( T->ml.get(), m2->id() );
    ASSERT_TRUE( res );

    tracks = album->tracks( nullptr )->all();
    ASSERT_EQ( 0u, tracks.size() );
}

int main( int ac, char** av )
{
    INIT_TESTS
    ADD_TEST( Create );
    ADD_TEST( GetAlbum );
    ADD_TEST( CheckDbModel );
    ADD_TEST( DeleteByMediaId );
    END_TESTS
}
