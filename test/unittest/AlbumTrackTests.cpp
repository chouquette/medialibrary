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
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto res = album->addTrack( m, 1, 10, 0, nullptr );
    m->save();
    ASSERT_TRUE( res );
    ASSERT_EQ( 10u, m->discNumber() );
    ASSERT_EQ( nullptr, m->artist() );
    ASSERT_EQ( 0, m->artistId() );
    ASSERT_EQ( album->id(), m->albumId() );
    ASSERT_EQ( 0, m->genreId() );
    ASSERT_EQ( nullptr, m->genre() );

    m = std::static_pointer_cast<Media>( T->ml->media( m->id() ) );
    ASSERT_EQ( 10u, m->discNumber() );
}

static void GetAlbum( Tests *T )
{
    auto album = T->ml->createAlbum( "album" );
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto res = album->addTrack( m, 1, 0, 0, nullptr );
    ASSERT_TRUE( res );
    m->save();

    auto albumFromTrack = m->album();
    ASSERT_EQ( album->id(), albumFromTrack->id() );

    m = T->ml->media( m->id() );
    albumFromTrack = m->album();
    auto a2 = T->ml->album( album->id() );
    // Fetching this value twice seems to be problematic on Android.
    // Ensure it works for other platforms at least
    auto aft2 = m->album();
    ASSERT_EQ( albumFromTrack->id(), a2->id() );
    ASSERT_EQ( aft2->id(), a2->id() );
}

int main( int ac, char** av )
{
    INIT_TESTS(AlbumTrack)
    ADD_TEST( Create );
    ADD_TEST( GetAlbum );
    END_TESTS
}
