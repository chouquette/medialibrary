/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Service.h"

static void FetchCreate( Tests* T )
{
    auto s = T->ml->service( IService::Type::Podcast );
    ASSERT_NON_NULL( s );
    ASSERT_EQ( s->type(), IService::Type::Podcast );
    auto s2 = T->ml->service( IService::Type::Podcast );
    ASSERT_NON_NULL( s2 );
    ASSERT_EQ( s->type(), s->type() );
}

static void CheckDbModel( Tests* T )
{
    auto res = Service::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void AutoDownload( Tests* T )
{
    auto s = T->ml->service( IService::Type::Podcast );
    ASSERT_NON_NULL( s );
    ASSERT_TRUE( s->isAutoDownloadEnabled() );

    auto res = s->setAutoDownloadEnabled( false );
    ASSERT_TRUE( res );
    ASSERT_FALSE( s->isAutoDownloadEnabled() );
    s = T->ml->service( IService::Type::Podcast );
    ASSERT_FALSE( s->isAutoDownloadEnabled() );

    res = s->setAutoDownloadEnabled( true );
    ASSERT_TRUE( res );
    ASSERT_TRUE( s->isAutoDownloadEnabled() );
    s = T->ml->service( IService::Type::Podcast );
    ASSERT_TRUE( s->isAutoDownloadEnabled() );
}

static void NewMediaNotification( Tests* T )
{
    auto s = T->ml->service( IService::Type::Podcast );
    ASSERT_NON_NULL( s );
    ASSERT_TRUE( s->isNewMediaNotificationEnabled() );

    auto res = s->setNewMediaNotificationEnabled( false );
    ASSERT_TRUE( res );
    ASSERT_FALSE( s->isNewMediaNotificationEnabled() );
    s = T->ml->service( IService::Type::Podcast );
    ASSERT_FALSE( s->isNewMediaNotificationEnabled() );

    res = s->setNewMediaNotificationEnabled( true );
    ASSERT_TRUE( res );
    ASSERT_TRUE( s->isNewMediaNotificationEnabled() );
    s = T->ml->service( IService::Type::Podcast );
    ASSERT_TRUE( s->isNewMediaNotificationEnabled() );
}

static void MaxCachedSize( Tests* T )
{
    auto s = T->ml->service( IService::Type::Podcast );
    ASSERT_NON_NULL( s );
    ASSERT_EQ( -1, s->maxCachedSize() );

    auto res = s->setMaxCachedSize( -666 );
    ASSERT_TRUE( res );
    ASSERT_EQ( -1, s->maxCachedSize() );

    res = s->setMaxCachedSize( 12345 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 12345, s->maxCachedSize() );

    s = T->ml->service( IService::Type::Podcast );
    ASSERT_EQ( 12345, s->maxCachedSize() );

    res = s->setMaxCachedSize( -1 );
    ASSERT_TRUE( res );
    ASSERT_EQ( -1, s->maxCachedSize() );

    s = T->ml->service( IService::Type::Podcast );
    ASSERT_EQ( -1, s->maxCachedSize() );
}

int main( int ac, char** av )
{
    INIT_TESTS( Service )

    ADD_TEST( FetchCreate );
    ADD_TEST( CheckDbModel );
    ADD_TEST( AutoDownload );
    ADD_TEST( NewMediaNotification );
    ADD_TEST( MaxCachedSize );
    END_TESTS
}
