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

#include "UnitTests.h"

#include "Bookmark.h"
#include "Media.h"

struct BookmarkTests : public Tests
{
    std::shared_ptr<Media> m;

    virtual void TestSpecificSetup() override
    {
        m = std::static_pointer_cast<Media>( ml->addMedia( "fluffyotters.mkv", IMedia::Type::Video ) );
    }
};

static void Create( BookmarkTests* T )
{
    auto b = Bookmark::create( T->ml.get(), 1, T->m->id() );
    ASSERT_NE( nullptr, b );
    ASSERT_NE( 0, b->id() );
    ASSERT_EQ( 1, b->time() );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );
    ASSERT_EQ( b->mediaId(), T->m->id() );
    ASSERT_NE( 0, b->creationDate() );
    ASSERT_EQ( IBookmark::Type::Simple, b->type() );

    b = Bookmark::fetch( T->ml.get(), b->id() );
    ASSERT_NE( 0, b->id() );
    ASSERT_EQ( 1, b->time() );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );
    ASSERT_EQ( b->mediaId(), T->m->id() );
    ASSERT_NE( 0, b->creationDate() );
    ASSERT_EQ( IBookmark::Type::Simple, b->type() );
}

static void SetName( BookmarkTests* T )
{
    auto b = Bookmark::create( T->ml.get(), 1, T->m->id() );
    ASSERT_NE( nullptr, b );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );

    auto newName = "so much fluff";
    auto res = b->setName( newName );
    ASSERT_TRUE( res );
    ASSERT_EQ( newName, b->name() );
    ASSERT_EQ( "", b->description() );

    b = Bookmark::fetch( T->ml.get(), b->id() );
    ASSERT_EQ( newName, b->name() );
    ASSERT_EQ( "", b->description() );
}

static void SetDescription( BookmarkTests* T )
{
    auto b = Bookmark::create( T->ml.get(), 1, T->m->id() );
    ASSERT_NE( nullptr, b );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );

    auto newDesc = "This is when the otters hold hands and it's so cute zomg!!";
    auto res = b->setDescription( newDesc );
    ASSERT_TRUE( res );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( newDesc, b->description() );

    b = Bookmark::fetch( T->ml.get(), b->id() );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( newDesc, b->description() );
}

static void SetNameAndDesc( BookmarkTests* T )
{
    auto b = std::static_pointer_cast<Bookmark>( T->m->addBookmark( 123 ) );
    ASSERT_EQ( "", b->name() );
    ASSERT_EQ( "", b->description() );

    auto newName = "snow riding";
    auto newDesc = "This when the otter does luge on its tummy and it's ridiculously cute";
    auto res = b->setNameAndDescription( newName, newDesc );
    ASSERT_TRUE( res );

    ASSERT_EQ( newName, b->name() );
    ASSERT_EQ( newDesc, b->description() );

    b = Bookmark::fetch( T->ml.get(), b->id() );
    ASSERT_EQ( newName, b->name() );
    ASSERT_EQ( newDesc, b->description() );
}

static void List( BookmarkTests* T )
{
    for ( auto i = 0; i < 3; ++i )
    {
        auto b = T->m->addBookmark( i );
        ASSERT_NE( nullptr, b );
        b->setName( "bookmark_" + std::to_string( i ) );
    }
    QueryParameters params;
    params.sort = SortingCriteria::Default;
    params.desc = false;
    auto query = T->m->bookmarks( &params );
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
    query = T->m->bookmarks( &params );
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

static void SortByName( BookmarkTests* T )
{
    for ( auto i = 0; i < 3; ++i )
    {
        auto b = T->m->addBookmark( 3 - i );
        ASSERT_NE( nullptr, b );
        b->setName( "bookmark_" + std::to_string( i ) );
    }
    QueryParameters params{};
    params.sort = SortingCriteria::Alpha;
    params.desc = false;
    auto query = T->m->bookmarks( &params );
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
    query = T->m->bookmarks( nullptr );
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

static void Delete( BookmarkTests* T )
{
    for ( auto i = 0; i < 3; ++i )
        T->m->addBookmark( i );
    auto query = T->m->bookmarks( nullptr );
    ASSERT_EQ( 3u, query->count() );
    ASSERT_EQ( 3u, query->all().size() );

    auto res = T->m->removeBookmark( 0 );
    ASSERT_TRUE( res );
    query = T->m->bookmarks( nullptr );
    ASSERT_EQ( 2u, query->count() );
    ASSERT_EQ( 2u, query->all().size() );

    res = T->m->removeBookmark( 0 );
    ASSERT_TRUE( res );
    query = T->m->bookmarks( nullptr );
    ASSERT_EQ( 2u, query->count() );
    ASSERT_EQ( 2u, query->all().size() );

    res = T->m->removeBookmark( 1 );
    ASSERT_TRUE( res );
    query = T->m->bookmarks( nullptr );
    ASSERT_EQ( 1u, query->count() );
    ASSERT_EQ( 1u, query->all().size() );

    res = T->m->removeBookmark( 2 );
    ASSERT_TRUE( res );
    query = T->m->bookmarks( nullptr );
    ASSERT_EQ( 0u, query->count() );
    ASSERT_EQ( 0u, query->all().size() );
}

static void UniqueTime( BookmarkTests* T )
{
    auto res = T->m->addBookmark( 0 );
    ASSERT_NE( nullptr, res );
    res = T->m->addBookmark( 0 );
    ASSERT_EQ( nullptr, res );
}

static void Move( BookmarkTests* T )
{
    auto b = std::static_pointer_cast<Bookmark>( T->m->addBookmark( 123 ) );
    auto b2 = T->m->addBookmark( 456 );
    ASSERT_NE( nullptr, b );
    ASSERT_NE( nullptr, b2 );

    auto res = b->move( 321 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 321, b->time() );

    b = Bookmark::fetch( T->ml.get(), b->id() );
    ASSERT_EQ( 321, b->time() );

    res = b->move( b2->time() );
    ASSERT_FALSE( res );
}

static void DeleteAll( BookmarkTests* T )
{
    for ( auto i = 0; i < 3; ++i )
        T->m->addBookmark( i );
    auto query = T->m->bookmarks( nullptr );
    ASSERT_EQ( 3u, query->count() );

    T->m->removeAllBookmarks();
    query = T->m->bookmarks( nullptr );
    ASSERT_EQ( 0u, query->count() );
    ASSERT_EQ( 0u, query->all().size() );
}

static void CheckDbModel( BookmarkTests* T )
{
    auto res = Bookmark::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void OrderByCreationDate( BookmarkTests* T )
{
    auto forceCreationDate = [T]( int64_t bookmarkId, time_t t ) {
        const std::string req = "UPDATE " + Bookmark::Table::Name +
                " SET creation_date = ? WHERE id_bookmark = ?";
        return sqlite::Tools::executeUpdate( T->ml->getConn(), req, t, bookmarkId );
    };
    auto b1 = T->m->addBookmark( 0 );
    auto b2 = T->m->addBookmark( 10 );
    auto b3 = T->m->addBookmark( 100 );
    forceCreationDate( b1->id(), 111 );
    forceCreationDate( b2->id(), 333 );
    forceCreationDate( b3->id(), 222 );

    QueryParameters params{ SortingCriteria::InsertionDate, false };
    auto bookmarks = T->m->bookmarks( &params )->all();
    ASSERT_EQ( 3u, bookmarks.size() );
    ASSERT_EQ( b1->id(), bookmarks[0]->id() );
    ASSERT_EQ( b3->id(), bookmarks[1]->id() );
    ASSERT_EQ( b2->id(), bookmarks[2]->id() );

    params.desc = true;
    bookmarks = T->m->bookmarks( &params )->all();
    ASSERT_EQ( 3u, bookmarks.size() );
    ASSERT_EQ( b2->id(), bookmarks[0]->id() );
    ASSERT_EQ( b3->id(), bookmarks[1]->id() );
    ASSERT_EQ( b1->id(), bookmarks[2]->id() );
}

static void Fetch( BookmarkTests* T )
{
    auto b = Bookmark::create( T->ml.get(), 1, T->m->id() );
    ASSERT_NE( nullptr, b );

    auto b2 = T->ml->bookmark( b->id() );
    ASSERT_NE( b2, nullptr );

    b2 = T->ml->bookmark( b->id() + 1 );
    ASSERT_EQ( nullptr, b2 );
}

static void FetchByTime( BookmarkTests* T )
{
    auto b = T->m->addBookmark( 123 );
    ASSERT_NON_NULL( b );

    auto m2 = T->ml->addMedia( "other.mkv", IMedia::Type::Video );
    ASSERT_NON_NULL( m2 );
    auto b2 = m2->addBookmark( 321 );
    ASSERT_NON_NULL( b2 );

    auto fetched = T->m->bookmark( 123 );
    ASSERT_NON_NULL( fetched );

    ASSERT_EQ( b->id(), fetched->id() );

    fetched = T->m->bookmark( 321 );
    ASSERT_EQ( nullptr, fetched );

    fetched = m2->bookmark( 123 );
    ASSERT_EQ( nullptr, fetched );

    fetched = m2->bookmark( 321 );
    ASSERT_NON_NULL( fetched );
    ASSERT_EQ( b2->id(), fetched->id() );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( BookmarkTests );

    ADD_TEST( Create );
    ADD_TEST( SetName );
    ADD_TEST( SetDescription );
    ADD_TEST( SetNameAndDesc );
    ADD_TEST( List );
    ADD_TEST( SortByName );
    ADD_TEST( Delete );
    ADD_TEST( UniqueTime );
    ADD_TEST( Move );
    ADD_TEST( DeleteAll );
    ADD_TEST( CheckDbModel );
    ADD_TEST( OrderByCreationDate );
    ADD_TEST( Fetch );
    ADD_TEST( FetchByTime );

    END_TESTS
}
