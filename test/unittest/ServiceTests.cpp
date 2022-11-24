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
#include "Subscription.h"
#include "Media.h"

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

static void NbSubscriptions( Tests* T )
{
    auto s = T->ml->service( IService::Type::Podcast );
    ASSERT_NON_NULL( s );
    ASSERT_EQ( 0u, s->nbSubscriptions() );

    /*
     * We can't use Service::addSubscription since it relies on the parser
     * which isn't started for unit tests
    */
    auto sub = Subscription::create( T->ml.get(), s->type(), "test", 0 );
    ASSERT_NON_NULL( sub );

    s = T->ml->service( IService::Type::Podcast );
    ASSERT_EQ( 1u, s->nbSubscriptions() );

    auto subs = s->subscriptions( nullptr )->all();
    ASSERT_EQ( subs.size(), s->nbSubscriptions() );

    s = T->ml->service( IService::Type::Podcast );
    ASSERT_EQ( 1u, s->nbSubscriptions() );

    auto res = T->ml->removeSubscription( subs[0]->id() );
    ASSERT_TRUE( res );

    s = T->ml->service( IService::Type::Podcast );
    ASSERT_EQ( 0u, s->nbSubscriptions() );
}

static void NbUnplayedMedia( Tests* T )
{
    auto s = T->ml->service( IService::Type::Podcast );
    ASSERT_NON_NULL( s );
    ASSERT_EQ( 0u, s->nbUnplayedMedia() );
    ASSERT_EQ( 0u, s->nbMedia() );

    auto sub = Subscription::create( T->ml.get(), s->type(), "test", 0 );
    ASSERT_NON_NULL( sub );

    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "http://media.mk3", IMedia::Type::Audio ) );
    ASSERT_NON_NULL( m1 );

    auto sub2 = Subscription::create( T->ml.get(), s->type(), "test 2", 0 );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "http://media2.mk3", IMedia::Type::Audio ) );
    ASSERT_NON_NULL( m2 );

    auto res = sub->addMedia( *m1 );
    ASSERT_TRUE( res );

    s = T->ml->service( s->type() );
    ASSERT_EQ( 1u, s->nbUnplayedMedia() );
    ASSERT_EQ( 1u, s->nbMedia() );

    res = sub2->addMedia( *m2 );
    ASSERT_TRUE( res );

    s = T->ml->service( s->type() );
    ASSERT_EQ( 2u, s->nbUnplayedMedia() );
    ASSERT_EQ( 2u, s->nbMedia() );

    Subscription::destroy( T->ml.get(), sub2->id() );

    s = T->ml->service( s->type() );
    ASSERT_EQ( 1u, s->nbUnplayedMedia() );
    ASSERT_EQ( 1u, s->nbMedia() );

    m1->markAsPlayed();

    s = T->ml->service( s->type() );
    ASSERT_EQ( 0u, s->nbUnplayedMedia() );
    ASSERT_EQ( 1u, s->nbMedia() );

}

static void ListMedia( Tests* T )
{
    auto s = T->ml->service( IService::Type::Podcast );

    auto c1 = Subscription::create( T->ml.get(), s->type(), "collection", 0 );
    ASSERT_NON_NULL( c1 );
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addExternalMedia( "http://youtu.be/media1", -1 ) );
    ASSERT_NON_NULL( m1 );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file:///path/to/movie.mkv", IMedia::Type::Video ) );
    ASSERT_NON_NULL( m2 );
    c1->addMedia( *m1 );
    c1->addMedia( *m2 );

    auto c2 = Subscription::create( T->ml.get(), s->type(), "another collection", 0 );
    ASSERT_NON_NULL( c2 );
    auto m3 = std::static_pointer_cast<Media>(T->ml->addExternalMedia( "http://podcast.io/something.mp3", -1 ));
    ASSERT_NON_NULL( m3 );

    auto mediaQuery = s->media( nullptr );
    ASSERT_EQ( 2u, mediaQuery->count() );
    auto media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );

    c2->addMedia( *m3 );
    mediaQuery = s->media( nullptr );
    ASSERT_EQ( 3u, mediaQuery->count() );
    media = mediaQuery->all();
    ASSERT_EQ( 3u, media.size() );

    c1->removeMedia( m2->id() );
    mediaQuery = s->media( nullptr );
    ASSERT_EQ( 2u, mediaQuery->count() );
    media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );
}

static void Search( Tests* T )
{
    auto s = T->ml->service( IService::Type::Podcast );

    auto c1 = Subscription::create( T->ml.get(), s->type(), "collection 1", 0 );
    ASSERT_NON_NULL( c1 );
    auto c2 = Subscription::create( T->ml.get(), s->type(), "collection 2", 0 );
    ASSERT_NON_NULL( c2 );

    auto r = s->searchSubscription( "collection", nullptr )->all();
    ASSERT_EQ( r.size(), 2u );

    r = s->searchSubscription( "2", nullptr )->all();
    ASSERT_EQ( r.size(), 1u );
    ASSERT_EQ( r[0]->name(), c2->name() );

    r = s->searchSubscription( "nope", nullptr )->all();
    ASSERT_TRUE( r.empty() );
}

static void SearchMedia( Tests* T )
{
    auto s = T->ml->service( IService::Type::Podcast );

    auto c1 = Subscription::create( T->ml.get(), s->type(), "collection 1", 0 );
    ASSERT_NON_NULL( c1 );
    auto c2 = Subscription::create( T->ml.get(), s->type(), "collection 2", 0 );
    ASSERT_NON_NULL( c2 );

    ASSERT_NON_NULL( c1 );
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addExternalMedia( "m1", -1 ) );
    ASSERT_NON_NULL( m1 );
    m1->setTitle("media 1", true);
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "m2", IMedia::Type::Video ) );
    ASSERT_NON_NULL( m2 );
    m2->setTitle("media 2", true);

    c1->addMedia( *m1 );
    c2->addMedia( *m2 );

    auto r = s->searchMedia( "media", nullptr )->all();
    ASSERT_EQ( r.size(), 2u );
    ASSERT_EQ( r[0]->fileName(), "m1" );
    ASSERT_EQ( r[1]->fileName(), "m2" );

    r = s->searchMedia( "2", nullptr )->all();
    ASSERT_EQ( r.size(), 1u );
    ASSERT_EQ( r[0]->fileName(), "m2" );

    r = s->searchMedia( "nope", nullptr )->all();
    ASSERT_TRUE( r.empty() );
}

int main( int ac, char** av )
{
    INIT_TESTS( Service )

    ADD_TEST( FetchCreate );
    ADD_TEST( CheckDbModel );
    ADD_TEST( AutoDownload );
    ADD_TEST( NewMediaNotification );
    ADD_TEST( MaxCachedSize );
    ADD_TEST( NbSubscriptions );
    ADD_TEST( NbUnplayedMedia );
    ADD_TEST( ListMedia );
    ADD_TEST( Search );
    ADD_TEST( SearchMedia );

    END_TESTS
}
