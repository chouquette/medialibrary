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

#include "Genre.h"
#include "Album.h"
#include "AlbumTrack.h"

class Genres : public Tests
{
protected:
    std::shared_ptr<Genre> g;

    virtual void SetUp() override
    {
        Tests::SetUp();
        g = ml->createGenre( "genre" );
    }
};

TEST_F( Genres, Create )
{
    ASSERT_NE( nullptr, g );
    ASSERT_EQ( "genre", g->name() );
    auto tracks = g->tracks();
    ASSERT_EQ( 0u, tracks.size() );
}

TEST_F( Genres, List )
{
    auto g2 = ml->createGenre( "genre 2" );
    ASSERT_NE( nullptr, g2 );
    auto genres = ml->genres();
    ASSERT_EQ( 2u, genres.size() );
}

TEST_F( Genres, ListAlbumTracks )
{
    auto a = ml->createAlbum( "album" );

    for ( auto i = 1u; i <= 3; i++ )
    {
        auto m = ml->addFile( "track" + std::to_string( i ) + ".mp3" );
        auto t = a->addTrack( m, i, 1 );
        if ( i != 1 )
            t->setGenre( g );
    }
    auto tracks = g->tracks();
    ASSERT_EQ( 2u, tracks.size() );
}
