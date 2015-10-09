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

#include "IAlbum.h"
#include "IAlbumTrack.h"
#include "IArtist.h"

class AlbumTracks : public Tests
{
};

TEST_F( AlbumTracks, Artists )
{
    auto album = ml->createAlbum( "album" );
    auto artist1 = ml->createArtist( "artist 1" );
    auto artist2 = ml->createArtist( "artist 2" );
    album->addTrack( "track 1", 1 );
    album->addTrack( "track 2", 2 );
    album->addTrack( "track 3", 3 );

    ASSERT_NE( artist1, nullptr );
    ASSERT_NE( artist2, nullptr );

    auto tracks = album->tracks();
    for ( auto& t : tracks )
    {
        t->addArtist( artist1 );
        t->addArtist( artist2 );

        auto artists = t->artists();
        ASSERT_EQ( artists.size(), 2u );
    }

    auto artists = ml->artists();
    for ( auto& a : artists )
    {
        auto tracks = a->tracks();
        ASSERT_EQ( tracks.size(), 3u );
    }

    Reload();

    album = ml->album( "album" );
    tracks = album->tracks();
    for ( auto& t : tracks )
    {
        auto artists = t->artists();
        ASSERT_EQ( artists.size(), 2u );
    }

    artists = ml->artists();
    for ( auto& a : artists )
    {
        auto tracks = a->tracks();
        ASSERT_EQ( tracks.size(), 3u );
    }
}
