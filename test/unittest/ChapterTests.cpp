/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2018-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Chapter.h"
#include "Media.h"

struct ChapterTests : public Tests
{
    std::shared_ptr<Media> m;

    virtual void TestSpecificSetup() override
    {
        m = std::static_pointer_cast<Media>( ml->addMedia( "media.avi", IMedia::Type::Video ) );
    }
};

static void Create( ChapterTests* T )
{
    auto res = T->m->addChapter( 0, 10, "chapter 1" );
    ASSERT_TRUE( res );

    auto chapters = T->m->chapters( nullptr )->all();
    ASSERT_EQ( 1u, chapters.size() );
    auto chapter = chapters[0];
    ASSERT_EQ( 0, chapter->offset() );
    ASSERT_EQ( 10, chapter->duration() );
    ASSERT_EQ( "chapter 1", chapter->name() );
}

static void Fetch( ChapterTests* T )
{
    T->m->addChapter( 0, 10, "chapter 1" );
    T->m->addChapter( 11, 100, "chapter 2" );
    T->m->addChapter( 111, 1, "A different chapter" );

    auto query = T->m->chapters( nullptr );
    ASSERT_EQ( 3u, query->count() );
    auto chapters = query->all();
    ASSERT_EQ( 3u, chapters.size() );
    ASSERT_EQ( 0, chapters[0]->offset() );
    ASSERT_EQ( 11, chapters[1]->offset() );
    ASSERT_EQ( 111, chapters[2]->offset() );

    QueryParameters params;
    params.desc = false;
    params.sort = SortingCriteria::Duration;
    query = T->m->chapters( &params );
    ASSERT_EQ( 3u, query->count() );
    chapters = query->all();
    ASSERT_EQ( 100, chapters[0]->duration() );
    ASSERT_EQ( 10, chapters[1]->duration() );
    ASSERT_EQ( 1, chapters[2]->duration() );

    params.desc = true;
    query = T->m->chapters( &params );
    chapters = query->all();
    ASSERT_EQ( 3u, chapters.size() );
    ASSERT_EQ( 1, chapters[0]->duration() );
    ASSERT_EQ( 10, chapters[1]->duration() );
    ASSERT_EQ( 100, chapters[2]->duration() );

    params.sort = SortingCriteria::Alpha;
    params.desc = false;
    query = T->m->chapters( &params );
    chapters = query->all();
    ASSERT_EQ( 3u, chapters.size() );
    ASSERT_EQ( "A different chapter", chapters[0]->name() );
    ASSERT_EQ( 111, chapters[0]->offset() );
    ASSERT_EQ( "chapter 1", chapters[1]->name() );
    ASSERT_EQ( 0, chapters[1]->offset() );
    ASSERT_EQ( "chapter 2", chapters[2]->name() );
    ASSERT_EQ( 11, chapters[2]->offset() );
}

static void CheckDbModel( ChapterTests* T )
{
    auto res = Chapter::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( ChapterTests );

    ADD_TEST( Create );
    ADD_TEST( Fetch );
    ADD_TEST( CheckDbModel );

    END_TESTS
}
