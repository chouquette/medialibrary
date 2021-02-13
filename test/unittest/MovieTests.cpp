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

#include "MediaLibrary.h"
#include "Movie.h"
#include "Media.h"

static void Create( Tests* T )
{
    auto media = std::static_pointer_cast<Media>( T->ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    auto m = T->ml->createMovie( *media );
    ASSERT_NE( m, nullptr );
}

static void Fetch( Tests* T )
{
    auto media = std::static_pointer_cast<Media>( T->ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    media->setTitle( "movie", false );
    // Setting the movie during T->ml::createMovie will save the media, thus saving the title.
    auto m = T->ml->createMovie( *media );
    auto m2 = T->ml->movie( m->id() );

    ASSERT_NE( nullptr, m2 );
    ASSERT_EQ( m->id(), m2->id() );

    m2 = T->ml->movie( m->id() );
    ASSERT_NE( m2, nullptr );
}

static void SetShortSummary( Tests* T )
{
    auto media = std::static_pointer_cast<Media>( T->ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    media->setTitle( "movie", false );
    auto m = T->ml->createMovie( *media );

    ASSERT_EQ( m->shortSummary().length(), 0u );
    m->setShortSummary( "great movie" );
    ASSERT_EQ( m->shortSummary(), "great movie" );

    auto m2 = T->ml->movie( m->id() );
    ASSERT_EQ( m2->shortSummary(), "great movie" );
}

static void SetImdbId( Tests* T )
{
    auto media = std::static_pointer_cast<Media>( T->ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    media->setTitle( "movie", false );
    auto m = T->ml->createMovie( *media );

    ASSERT_EQ( m->imdbId().length(), 0u );
    m->setImdbId( "id" );
    ASSERT_EQ( m->imdbId(), "id" );

    auto m2 = T->ml->movie( m->id() );
    ASSERT_EQ( m2->imdbId(), "id" );
}

static void AssignToFile( Tests* T )
{
    auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "file.avi", IMedia::Type::Video ) );
    ASSERT_EQ( f->movie(), nullptr );

    auto m = T->ml->createMovie( *f );

    ASSERT_EQ( f->movie(), m );

    auto f2 = T->ml->media( f->id() );
    auto m2 = f2->movie();
    ASSERT_NE( m2, nullptr );
}

static void CheckDbModel( Tests* T )
{
    auto res = Movie::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void DeleteByMediaId( Tests* T )
{
    auto media1 = std::static_pointer_cast<Media>( T->ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    auto media2 = std::static_pointer_cast<Media>( T->ml->addMedia( "movie2.mkv", IMedia::Type::Video ) );
    auto movie1 = T->ml->createMovie( *media1 );
    auto movie2 = T->ml->createMovie( *media2 );
    ASSERT_NE( movie1, nullptr );
    ASSERT_NE( movie2, nullptr );

    Movie::deleteByMediaId( T->ml.get(), media1->id() );
    movie1 = std::static_pointer_cast<Movie>( T->ml->movie( movie1->id() ) );
    ASSERT_EQ( nullptr, movie1 );
    movie2 = std::static_pointer_cast<Movie>( T->ml->movie( movie2->id() ) );
    ASSERT_NE( nullptr, movie2 );
}

int main( int ac, char** av )
{
    INIT_TESTS;

    ADD_TEST( Create );
    ADD_TEST( Fetch );
    ADD_TEST( SetShortSummary );
    ADD_TEST( SetImdbId );
    ADD_TEST( AssignToFile );
    ADD_TEST( CheckDbModel );
    ADD_TEST( DeleteByMediaId );

    END_TESTS
}
