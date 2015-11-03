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

#include "Tests.h"

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "Media.h"

class AlbumTracks : public Tests
{
};


TEST_F( AlbumTracks, Artist )
{
    auto album = ml->createAlbum( "album" );
    auto f = ml->addFile( "track1.mp3", nullptr );
    auto track = album->addTrack( f, 1 );

    ASSERT_EQ( track->artist(), "" );
    track->setArtist( "artist" );
    ASSERT_EQ( track->artist(), "artist" );

    Reload();

    // Don't reuse the "track" and "f" variable, their type differ
    auto file = ml->file( "track1.mp3" );
    auto albumTrack = file->albumTrack();
    ASSERT_EQ( albumTrack->artist(), "artist" );
}

TEST_F( AlbumTracks, SetReleaseYear )
{
    auto a = ml->createAlbum( "album" );
    auto m = ml->addFile( "test.mp3", nullptr );
    auto t = a->addTrack( m, 1 );

    ASSERT_EQ( 0, t->releaseYear() );

    t->setReleaseYear( 1234 );
    ASSERT_EQ( t->releaseYear(), 1234 );

    Reload();

    auto m2 = ml->file( "test.mp3" );
    auto t2 = m2->albumTrack();
    ASSERT_EQ( t->releaseYear(), t2->releaseYear() );
}
