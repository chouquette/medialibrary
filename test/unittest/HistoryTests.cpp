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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "Tests.h"

#include "medialibrary/IHistoryEntry.h"
#include "History.h"
#include "Media.h"

#include "compat/Thread.h"

class HistoryTest : public Tests
{

};

TEST_F( HistoryTest, InsertMrl )
{
    auto m = ml->addMedia( "upnp://stream" );
    ml->addToStreamHistory( m );
    auto hList = ml->lastStreamsPlayed();
    ASSERT_EQ( 1u, hList.size() );
    auto h = hList[0];
    ASSERT_EQ( h->media()->files()[0]->mrl(), "upnp://stream" );
    ASSERT_NE( 0u, h->insertionDate() );
}

TEST_F( HistoryTest, MaxEntries )
{
    for ( auto i = 0u; i < History::MaxEntries; ++i )
    {
        auto m = ml->addMedia( "http://media" + std::to_string( i ) );
        ml->addToStreamHistory( m );
    }
    auto hList = ml->lastStreamsPlayed();
    ASSERT_EQ( History::MaxEntries, hList.size() );
    auto m = ml->addMedia("smb://new-media" );
    ml->addToStreamHistory( m );
    hList = ml->lastStreamsPlayed();
    ASSERT_EQ( History::MaxEntries, hList.size() );
}

TEST_F( HistoryTest, Ordering )
{
    auto m = ml->addMedia( "first-stream" );
    ml->addToStreamHistory( m );
    compat::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    auto m2 = ml->addMedia( "second-stream" );
    ml->addToStreamHistory( m2 );
    auto hList = ml->lastStreamsPlayed();
    ASSERT_EQ( 2u, hList.size() );
    ASSERT_EQ( hList[0]->media()->id(), m2->id() );
    ASSERT_EQ( hList[1]->media()->id(), m->id() );
}

TEST_F( HistoryTest, UpdateInsertionDate )
{
    auto m = ml->addMedia( "stream" );
    ml->addToStreamHistory( m );
    auto hList = ml->lastStreamsPlayed();
    ASSERT_EQ( 1u, hList.size() );
    auto date = hList[0]->insertionDate();
    compat::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    ml->addToStreamHistory( m );
    hList = ml->lastStreamsPlayed();
    ASSERT_EQ( 1u, hList.size() );
    ASSERT_NE( date, hList[0]->insertionDate() );
}

TEST_F( HistoryTest, ClearStreamHistory )
{
    auto m = ml->addMedia( "f00" );
    ml->addToStreamHistory( m );
    auto m2 = ml->addMedia( "bar" );
    ml->addToStreamHistory( m2 );
    auto history = ml->lastStreamsPlayed();
    ASSERT_EQ( 2u, history.size() );

    ml->clearHistory();
    history = ml->lastStreamsPlayed();
    ASSERT_EQ( 0u, history.size() );

    Reload();

    history = ml->lastStreamsPlayed();
    ASSERT_EQ( 0u, history.size() );
}
