/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Bookmark.h"
#include "Media.h"

class Bookmarks : public Tests
{
protected:
    std::shared_ptr<Media> m;
    virtual void SetUp() override
    {
        Tests::SetUp();
        m = std::static_pointer_cast<Media>( ml->addMedia( "fluffyotters.mkv", IMedia::Type::Video ) );
    }
};

TEST_F( Bookmarks, Create )
{
    auto b = Bookmark::create( ml.get(), 1, m->id() );
    ASSERT_NE( nullptr, b );
    ASSERT_NE( 0, b->id() );
    ASSERT_EQ( 1, b->time() );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );

    Reload();

    b = Bookmark::fetch( ml.get(), b->id() );
    ASSERT_NE( 0, b->id() );
    ASSERT_EQ( 1, b->time() );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );
}

TEST_F( Bookmarks, SetName )
{
    auto b = Bookmark::create( ml.get(), 1, m->id() );
    ASSERT_NE( nullptr, b );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );

    auto newName = "so much fluff";
    auto res = b->setName( newName );
    ASSERT_TRUE( res );
    ASSERT_EQ( newName, b->name() );
    ASSERT_EQ( "", b->description() );

    Reload();

    b = Bookmark::fetch( ml.get(), b->id() );
    ASSERT_EQ( newName, b->name() );
    ASSERT_EQ( "", b->description() );
}

TEST_F( Bookmarks, SetDescription )
{
    auto b = Bookmark::create( ml.get(), 1, m->id() );
    ASSERT_NE( nullptr, b );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );

    auto newDesc = "This is when the otters hold hands and it's so cute zomg!!";
    auto res = b->setDescription( newDesc );
    ASSERT_TRUE( res );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( newDesc, b->description() );

    Reload();

    b = Bookmark::fetch( ml.get(), b->id() );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( newDesc, b->description() );
}

TEST_F( Bookmarks, SetNameAndDesc )
{
    auto b = std::static_pointer_cast<Bookmark>( m->addBookmark( 123 ) );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );

    auto newName = "snow riding";
    auto newDesc = "This when the otter does luge on its tummy and it's ridiculously cute";
    auto res = b->setNameAndDescription( newName, newDesc );
    ASSERT_TRUE( res );

    ASSERT_EQ( newName, b->name() );
    ASSERT_EQ( newDesc, b->description() );

    Reload();

    b = Bookmark::fetch( ml.get(), b->id() );
    ASSERT_EQ( newName, b->name() );
    ASSERT_EQ( newDesc, b->description() );
}

TEST_F( Bookmarks, List )
{
    for ( auto i = 0; i < 3; ++i )
    {
        auto b = m->addBookmark( i );
        ASSERT_NE( nullptr, b );
        b->setName( "bookmark_" + std::to_string( i ) );
    }
    QueryParameters params;
    params.sort = SortingCriteria::Default;
    params.desc = false;
    auto query = m->bookmarks( &params );
    ASSERT_EQ( 3u, query->count() );
    auto bookmarks = query->all();
    ASSERT_EQ( 3u, bookmarks.size() );
    ASSERT_EQ( 0, bookmarks[0]->time() );
    ASSERT_EQ( "bookmark_0", bookmarks[0]->name() );
    ASSERT_EQ( 1, bookmarks[1]->time() );
    ASSERT_EQ( "bookmark_1", bookmarks[1]->name() );
    ASSERT_EQ( 2, bookmarks[2]->time() );
    ASSERT_EQ( "bookmark_2", bookmarks[2]->name() );

    params.desc = true;
    query = m->bookmarks( &params );
    ASSERT_EQ( 3u, query->count() );
    bookmarks = query->all();
    ASSERT_EQ( 3u, bookmarks.size() );
    ASSERT_EQ( 0, bookmarks[2]->time() );
    ASSERT_EQ( "bookmark_0", bookmarks[2]->name() );
    ASSERT_EQ( 1, bookmarks[1]->time() );
    ASSERT_EQ( "bookmark_1", bookmarks[1]->name() );
    ASSERT_EQ( 2, bookmarks[0]->time() );
    ASSERT_EQ( "bookmark_2", bookmarks[0]->name() );
}

TEST_F( Bookmarks, SortByName )
{
    for ( auto i = 0; i < 3; ++i )
    {
        auto b = m->addBookmark( 3 - i );
        ASSERT_NE( nullptr, b );
        b->setName( "bookmark_" + std::to_string( i ) );
    }
    QueryParameters params{};
    params.sort = SortingCriteria::Alpha;
    params.desc = false;
    auto query = m->bookmarks( &params );
    ASSERT_EQ( 3u, query->count() );
    auto bookmarks = query->all();
    ASSERT_EQ( 3u, bookmarks.size() );
    ASSERT_EQ( 3, bookmarks[0]->time() );
    ASSERT_EQ( "bookmark_0", bookmarks[0]->name() );
    ASSERT_EQ( 2, bookmarks[1]->time() );
    ASSERT_EQ( "bookmark_1", bookmarks[1]->name() );
    ASSERT_EQ( 1, bookmarks[2]->time() );
    ASSERT_EQ( "bookmark_2", bookmarks[2]->name() );

    params.desc = true;
    query = m->bookmarks( nullptr );
    ASSERT_EQ( 3u, query->count() );
    bookmarks = query->all();
    ASSERT_EQ( 3u, bookmarks.size() );
    ASSERT_EQ( 1, bookmarks[0]->time() );
    ASSERT_EQ( "bookmark_2", bookmarks[0]->name() );
    ASSERT_EQ( 2, bookmarks[1]->time() );
    ASSERT_EQ( "bookmark_1", bookmarks[1]->name() );
    ASSERT_EQ( 3, bookmarks[2]->time() );
    ASSERT_EQ( "bookmark_0", bookmarks[2]->name() );
}

TEST_F( Bookmarks, Delete )
{
    for ( auto i = 0; i < 3; ++i )
        m->addBookmark( i );
    auto query = m->bookmarks( nullptr );
    ASSERT_EQ( 3u, query->count() );
    ASSERT_EQ( 3u, query->all().size() );

    auto res = m->removeBookmark( 0 );
    ASSERT_TRUE( res );
    query = m->bookmarks( nullptr );
    ASSERT_EQ( 2u, query->count() );
    ASSERT_EQ( 2u, query->all().size() );

    res = m->removeBookmark( 0 );
    ASSERT_TRUE( res );
    query = m->bookmarks( nullptr );
    ASSERT_EQ( 2u, query->count() );
    ASSERT_EQ( 2u, query->all().size() );

    res = m->removeBookmark( 1 );
    ASSERT_TRUE( res );
    query = m->bookmarks( nullptr );
    ASSERT_EQ( 1u, query->count() );
    ASSERT_EQ( 1u, query->all().size() );

    res = m->removeBookmark( 2 );
    ASSERT_TRUE( res );
    query = m->bookmarks( nullptr );
    ASSERT_EQ( 0u, query->count() );
    ASSERT_EQ( 0u, query->all().size() );
}

TEST_F( Bookmarks, UniqueTime )
{
    auto res = m->addBookmark( 0 );
    ASSERT_NE( nullptr, res );
    res = m->addBookmark( 0 );
    ASSERT_EQ( nullptr, res );
}

TEST_F( Bookmarks, Move )
{
    auto b = std::static_pointer_cast<Bookmark>( m->addBookmark( 123 ) );
    auto b2 = m->addBookmark( 456 );
    ASSERT_NE( nullptr, b );
    ASSERT_NE( nullptr, b2 );

    auto res = b->move( 321 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 321, b->time() );

    Reload();

    b = Bookmark::fetch( ml.get(), b->id() );
    ASSERT_EQ( 321, b->time() );

    res = b->move( b2->time() );
    ASSERT_FALSE( res );
}

TEST_F( Bookmarks, DeleteAll )
{
    for ( auto i = 0; i < 3; ++i )
        m->addBookmark( i );
    auto query = m->bookmarks( nullptr );
    ASSERT_EQ( 3u, query->count() );

    m->removeAllBookmarks();
    query = m->bookmarks( nullptr );
    ASSERT_EQ( 0u, query->count() );
    ASSERT_EQ( 0u, query->all().size() );
}

TEST_F( Bookmarks, CheckDbModel )
{
    auto res = Bookmark::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}
