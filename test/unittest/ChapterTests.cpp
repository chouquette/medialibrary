/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2018 Hugo Beauzée-Luyssen, Videolabs
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

#include "Chapter.h"
#include "Media.h"

class ChapterTests : public Tests
{
protected:
    std::shared_ptr<Media> m;

    virtual void SetUp() override
    {
        Tests::SetUp();
        m = std::static_pointer_cast<Media>( ml->addMedia( "media.avi" ) );
    }
};

TEST_F( ChapterTests, Create )
{
    auto res = m->addChapter( 0, 10, "chapter 1" );
    ASSERT_TRUE( res );

    auto chapters = m->chapters( nullptr )->all();
    ASSERT_EQ( 1u, chapters.size() );
    auto chapter = chapters[0];
    ASSERT_EQ( 0, chapter->offset() );
    ASSERT_EQ( 10, chapter->duration() );
    ASSERT_EQ( "chapter 1", chapter->name() );
}

TEST_F( ChapterTests, Fetch )
{
    m->addChapter( 0, 10, "chapter 1" );
    m->addChapter( 11, 100, "chapter 2" );
    m->addChapter( 111, 1, "A different chapter" );

    auto query = m->chapters( nullptr );
    ASSERT_EQ( 3u, query->count() );
    auto chapters = query->all();
    ASSERT_EQ( 3u, chapters.size() );
    ASSERT_EQ( 0, chapters[0]->offset() );
    ASSERT_EQ( 11, chapters[1]->offset() );
    ASSERT_EQ( 111, chapters[2]->offset() );

    QueryParameters params;
    params.desc = false;
    params.sort = SortingCriteria::Duration;
    query = m->chapters( &params );
    ASSERT_EQ( 3u, query->count() );
    chapters = query->all();
    ASSERT_EQ( 100, chapters[0]->duration() );
    ASSERT_EQ( 10, chapters[1]->duration() );
    ASSERT_EQ( 1, chapters[2]->duration() );

    params.desc = true;
    query = m->chapters( &params );
    chapters = query->all();
    ASSERT_EQ( 3u, chapters.size() );
    ASSERT_EQ( 1, chapters[0]->duration() );
    ASSERT_EQ( 10, chapters[1]->duration() );
    ASSERT_EQ( 100, chapters[2]->duration() );

    params.sort = SortingCriteria::Alpha;
    params.desc = false;
    query = m->chapters( &params );
    chapters = query->all();
    ASSERT_EQ( 3u, chapters.size() );
    ASSERT_EQ( "A different chapter", chapters[0]->name() );
    ASSERT_EQ( 111, chapters[0]->offset() );
    ASSERT_EQ( "chapter 1", chapters[1]->name() );
    ASSERT_EQ( 0, chapters[1]->offset() );
    ASSERT_EQ( "chapter 2", chapters[2]->name() );
    ASSERT_EQ( 11, chapters[2]->offset() );
}
