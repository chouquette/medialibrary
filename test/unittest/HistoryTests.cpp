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

#include "medialibrary/IHistoryEntry.h"
#include "History.h"
#include "Media.h"

#include <thread>

class HistoryTest : public Tests
{

};

TEST_F( HistoryTest, InsertMrl )
{
    ml->addToHistory( "upnp://stream" );
    auto hList = ml->lastStreamsPlayed();
    ASSERT_EQ( 1u, hList.size() );
    auto h = hList[0];
    ASSERT_EQ( h->mrl(), "upnp://stream" );
    ASSERT_NE( 0u, h->insertionDate() );
}

TEST_F( HistoryTest, MaxEntries )
{
    for ( auto i = 0u; i < History::MaxEntries; ++i )
    {
        ml->addToHistory( std::to_string( i ) );
    }
    auto hList = ml->lastStreamsPlayed();
    ASSERT_EQ( History::MaxEntries, hList.size() );
    ml->addToHistory( "new-media" );
    hList = ml->lastStreamsPlayed();
    ASSERT_EQ( History::MaxEntries, hList.size() );
}

TEST_F( HistoryTest, Ordering )
{
    ml->addToHistory( "first-stream" );
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    ml->addToHistory( "second-stream" );
    auto hList = ml->lastStreamsPlayed();
    ASSERT_EQ( 2u, hList.size() );
    ASSERT_EQ( hList[0]->mrl(), "second-stream" );
    ASSERT_EQ( hList[1]->mrl(), "first-stream" );
}

TEST_F( HistoryTest, UpdateInsertionDate )
{
    ml->addToHistory( "stream" );
    auto hList = ml->lastStreamsPlayed();
    ASSERT_EQ( 1u, hList.size() );
    auto date = hList[0]->insertionDate();
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    ml->addToHistory( "stream" );
    hList = ml->lastStreamsPlayed();
    ASSERT_EQ( 1u, hList.size() );
    ASSERT_NE( date, hList[0]->insertionDate() );
}

TEST_F( HistoryTest, FavoriteMrl )
{
    ml->addToHistory( "stream" );
    auto hList = ml->lastStreamsPlayed();
    ASSERT_EQ( 1u, hList.size() );
    auto item = hList[0];
    ASSERT_FALSE( item->isFavorite() );
    item->setFavorite( true );
    ASSERT_TRUE( item->isFavorite() );
}

TEST_F( HistoryTest, ReinsertFavorited )
{
    ml->addToHistory( "stream" );
    auto hList = ml->lastStreamsPlayed();
    auto item = hList[0];
    auto date = item->insertionDate();
    item->setFavorite( true );
    std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    ml->addToHistory( "stream" );
    hList = ml->lastStreamsPlayed();
    item = hList[0];
    ASSERT_NE( date, item->insertionDate() );
    ASSERT_TRUE( item->isFavorite() );
}
