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

#include "Tests.h"

#include "MediaLibrary.h"
#include "Movie.h"
#include "Media.h"

class Movies : public Tests
{
};

TEST_F( Movies, Create )
{
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    auto m = ml->createMovie( *media );
    ASSERT_NE( m, nullptr );
}

TEST_F( Movies, Fetch )
{
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    media->setTitle( "movie", false );
    // Setting the movie during ml::createMovie will save the media, thus saving the title.
    auto m = ml->createMovie( *media );
    auto m2 = ml->movie( m->id() );

    ASSERT_NE( nullptr, m2 );
    ASSERT_EQ( m->id(), m2->id() );

    m2 = ml->movie( m->id() );
    ASSERT_NE( m2, nullptr );
}

TEST_F( Movies, SetShortSummary )
{
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    media->setTitle( "movie", false );
    auto m = ml->createMovie( *media );

    ASSERT_EQ( m->shortSummary().length(), 0u );
    m->setShortSummary( "great movie" );
    ASSERT_EQ( m->shortSummary(), "great movie" );

    auto m2 = ml->movie( m->id() );
    ASSERT_EQ( m2->shortSummary(), "great movie" );
}

TEST_F( Movies, SetImdbId )
{
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    media->setTitle( "movie", false );
    auto m = ml->createMovie( *media );

    ASSERT_EQ( m->imdbId().length(), 0u );
    m->setImdbId( "id" );
    ASSERT_EQ( m->imdbId(), "id" );

    auto m2 = ml->movie( m->id() );
    ASSERT_EQ( m2->imdbId(), "id" );
}

TEST_F( Movies, AssignToFile )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.avi", IMedia::Type::Video ) );
    ASSERT_EQ( f->movie(), nullptr );

    auto m = ml->createMovie( *f );

    ASSERT_EQ( f->movie(), m );

    auto f2 = ml->media( f->id() );
    auto m2 = f2->movie();
    ASSERT_NE( m2, nullptr );
}

TEST_F( Movies, CheckDbModel )
{
    auto res = Movie::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( Movies, DeleteByMediaId )
{
    auto media1 = std::static_pointer_cast<Media>( ml->addMedia( "movie.mkv", IMedia::Type::Video ) );
    auto media2 = std::static_pointer_cast<Media>( ml->addMedia( "movie2.mkv", IMedia::Type::Video ) );
    auto movie1 = ml->createMovie( *media1 );
    auto movie2 = ml->createMovie( *media2 );
    ASSERT_NE( movie1, nullptr );
    ASSERT_NE( movie2, nullptr );

    Movie::deleteByMediaId( ml.get(), media1->id() );
    movie1 = std::static_pointer_cast<Movie>( ml->movie( movie1->id() ) );
    ASSERT_EQ( nullptr, movie1 );
    movie2 = std::static_pointer_cast<Movie>( ml->movie( movie2->id() ) );
    ASSERT_NE( nullptr, movie2 );
}
