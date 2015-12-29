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

#include "MediaLibrary.h"
#include "Movie.h"
#include "Media.h"

class Movies : public Tests
{
};

TEST_F( Movies, Create )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_NE( m, nullptr );
    ASSERT_EQ( m->title(), "movie" );
}

TEST_F( Movies, Fetch )
{
    auto m = ml->createMovie( "movie" );
    auto m2 = ml->movie( "movie" );

    ASSERT_EQ( m, m2 );

    Reload();

    m2 = ml->movie( "movie" );
    ASSERT_NE( m2, nullptr );
    ASSERT_EQ( m2->title(), "movie" );
}

TEST_F( Movies, SetReleaseDate )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_EQ( m->releaseDate(), 0u );
    m->setReleaseDate( 1234 );
    ASSERT_EQ( m->releaseDate(), 1234u );

    Reload();

    auto m2 = ml->movie( "movie" );
    ASSERT_EQ( m2->releaseDate(), 1234u );
}

TEST_F( Movies, SetShortSummary )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_EQ( m->shortSummary().length(), 0u );
    m->setShortSummary( "great movie" );
    ASSERT_EQ( m->shortSummary(), "great movie" );

    Reload();

    auto m2 = ml->movie( "movie" );
    ASSERT_EQ( m2->shortSummary(), "great movie" );
}

TEST_F( Movies, SetArtworkUrl )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_EQ( m->artworkUrl().length(), 0u );
    m->setArtworkUrl( "artwork" );
    ASSERT_EQ( m->artworkUrl(), "artwork" );

    Reload();

    auto m2 = ml->movie( "movie" );
    ASSERT_EQ( m2->artworkUrl(), "artwork" );
}

TEST_F( Movies, SetImdbId )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_EQ( m->imdbId().length(), 0u );
    m->setImdbId( "id" );
    ASSERT_EQ( m->imdbId(), "id" );

    Reload();

    auto m2 = ml->movie( "movie" );
    ASSERT_EQ( m2->imdbId(), "id" );
}

TEST_F( Movies, AssignToFile )
{
    auto f = ml->addFile( "file.avi", nullptr, nullptr );
    auto m = ml->createMovie( "movie" );

    ASSERT_EQ( f->movie(), nullptr );
    f->setMovie( m );
    f->save();
    ASSERT_EQ( f->movie(), m );

    Reload();

    auto f2 = ml->file( "file.avi" );
    auto m2 = f2->movie();
    ASSERT_NE( m2, nullptr );
    ASSERT_EQ( m2->title(), "movie" );
}
