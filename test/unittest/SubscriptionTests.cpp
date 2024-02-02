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

#include "Subscription.h"
#include "Media.h"
#include "File.h"

static void Create( Tests* T )
{
    auto c = Subscription::create( T->ml.get(), IService::Type::Podcast, "collection",
                                   "artwork",  0 );
    ASSERT_NON_NULL( c );
    ASSERT_EQ( c->name(), "collection" );
    ASSERT_EQ( c->artworkMRL(), "artwork" );
}

static void ListFromService( Tests* T )
{
    auto service = T->ml->service( IService::Type::Podcast );
    auto collectionQuery = service->subscriptions( nullptr );
    ASSERT_EQ( 0u, collectionQuery->count() );
    auto collections = collectionQuery->all();
    ASSERT_TRUE( collections.empty() );

    auto c = Subscription::create( T->ml.get(), IService::Type::Podcast, "Z collection",
                                   "artwork", 0 );
    ASSERT_NON_NULL( c );
    auto c2 = Subscription::create( T->ml.get(), IService::Type::Podcast, "A collection",
                                    "artwork", 0 );
    ASSERT_NON_NULL( c2 );

    collectionQuery = service->subscriptions( nullptr );
    ASSERT_EQ( 2u, collectionQuery->count() );
    collections = collectionQuery->all();
    ASSERT_EQ( 2u, collections.size() );

    ASSERT_EQ( c2->id(), collections[0]->id() );
    ASSERT_EQ( c->id(), collections[1]->id() );

    QueryParameters params{};
    params.desc = true;

    collectionQuery = service->subscriptions( &params );
    ASSERT_EQ( 2u, collectionQuery->count() );
    collections = collectionQuery->all();
    ASSERT_EQ( 2u, collections.size() );

    ASSERT_EQ( c->id(), collections[0]->id() );
    ASSERT_EQ( c2->id(), collections[1]->id() );
}

static void ListFromMedia( Tests* T )
{
    auto c =
        Subscription::create( T->ml.get(), IService::Type::Podcast, "A collection", "artwork", 0 );
    ASSERT_NON_NULL( c );

    auto m1 = std::static_pointer_cast<Media>(
        T->ml->addMedia( "file:///path/to/movie.mkv", IMedia::Type::Video ) );
    ASSERT_NON_NULL( m1 );

    auto res = m1->linkedSubscriptions( nullptr )->all();
    ASSERT_TRUE( res.empty() );

    c->addMedia( *m1 );

    res = m1->linkedSubscriptions( nullptr )->all();
    ASSERT_EQ( res.size(), 1u );
    ASSERT_EQ( res[0]->id(), c->id() );

    auto c2 =
        Subscription::create( T->ml.get(), IService::Type::Podcast, "Z collection", "artwork", 0 );
    ASSERT_NON_NULL( c2 );

    c2->addMedia( *m1 );
    res = m1->linkedSubscriptions( nullptr )->all();
    ASSERT_EQ( res.size(), 2u );
    ASSERT_EQ( res[0]->id(), c->id() );
    ASSERT_EQ( res[1]->id(), c2->id() );

    QueryParameters params{};
    params.desc = true;
    res = m1->linkedSubscriptions( &params )->all();
    ASSERT_EQ( res.size(), 2u );
    ASSERT_EQ( res[0]->id(), c2->id() );
    ASSERT_EQ( res[1]->id(), c->id() );

    c->removeMedia( m1->id() );
    c2->removeMedia( m1->id() );
    res = m1->linkedSubscriptions( nullptr )->all();
    ASSERT_TRUE( res.empty() );
}

static void ChildSubscriptions( Tests* T )
{
    auto c = Subscription::create( T->ml.get(), IService::Type::Podcast, "collection",
                                   "artwork", 0 );
    ASSERT_NON_NULL( c );
    auto scQuery = c->childSubscriptions( nullptr );
    ASSERT_EQ( 0u, scQuery->count() );
    auto sc = scQuery->all();
    ASSERT_EQ( 0u, sc.size() );

    auto sc1 = c->addChildSubscription( "Z", "https://child1.png" );
    ASSERT_NON_NULL( sc1 );
    auto sc2 = c->addChildSubscription( "A", "https://child2.jpg" );
    ASSERT_NON_NULL( sc2 );

    ASSERT_EQ( 2u, scQuery->count() );
    sc = scQuery->all();
    ASSERT_EQ( 2u, sc.size() );
    ASSERT_EQ( sc2->id(), sc[0]->id() );
    ASSERT_EQ( sc1->id(), sc[1]->id() );

    auto parent = sc1->parent();
    ASSERT_NON_NULL( parent );
    ASSERT_EQ( parent->id(), c->id() );
    parent = sc2->parent();
    ASSERT_NON_NULL( parent );
    ASSERT_EQ( parent->id(), c->id() );

    /* Ensure sub collections aren't listed as 1st level collections */
    auto service = T->ml->service( IService::Type::Podcast );
    auto collections = service->subscriptions( nullptr )->all();
    ASSERT_EQ( 1u, collections.size() );
    ASSERT_EQ( c->id(), collections[0]->id() );

    sc2 = Subscription::fetch( T->ml.get(), sc2->id() );
    ASSERT_NON_NULL( sc2 );

    /* Ensure that sub collections are removed with their parent */
    auto res = T->ml->removeSubscription( c->id() );
    ASSERT_TRUE( res );
    sc2 = Subscription::fetch( T->ml.get(), sc2->id() );
    ASSERT_EQ( nullptr, sc2 );
    sc1 = Subscription::fetch( T->ml.get(), sc1->id() );
    ASSERT_EQ( nullptr, sc1 );
}

static void ListMedia( Tests* T )
{
    auto c1 = Subscription::create( T->ml.get(), IService::Type::Podcast, "collection",
                                    "artwork", 0 );
    ASSERT_NON_NULL( c1 );
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addExternalMedia( "http://youtu.be/media1", -1 ) );
    ASSERT_NON_NULL( m1 );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file:///path/to/movie.mkv", IMedia::Type::Video ) );
    ASSERT_NON_NULL( m2 );
    c1->addMedia( *m1 );
    c1->addMedia( *m2 );

    auto c2 = Subscription::create( T->ml.get(), IService::Type::Podcast, "another collection",
                                    "artwork", 0 );
    ASSERT_NON_NULL( c2 );
    auto m3 = T->ml->addExternalMedia( "http://podcast.io/something.mp3", -1 );
    ASSERT_NON_NULL( m3 );

    auto mediaQuery = c1->media( nullptr );
    ASSERT_EQ( 2u, mediaQuery->count() );
    auto media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );
}

static void SearchMedia( Tests* T )
{
    auto c1 =
        Subscription::create( T->ml.get(), IService::Type::Podcast, "collection", "artwork", 0 );
    ASSERT_NON_NULL( c1 );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addExternalMedia( "m1.mkv", -1 ) );
    ASSERT_NON_NULL( m1 );
    m1->setTitle( "title 1", true );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "m2.mkv", IMedia::Type::Video ) );
    ASSERT_NON_NULL( m2 );
    m2->setTitle( "title 2", true );

    c1->addMedia( *m1 );
    c1->addMedia( *m2 );

    auto c2 = Subscription::create( T->ml.get(), IService::Type::Podcast, "another collection",
                                    "artwork", 0 );
    ASSERT_NON_NULL( c2 );
    auto m3 = std::static_pointer_cast<Media>( T->ml->addExternalMedia( "m3.mp3", -1 ) );
    ASSERT_NON_NULL( m3 );
    m3->setTitle( "title 3", true );
    c2->addMedia( *m3 );

    auto r = c1->search( "1", nullptr )->all();
    ASSERT_EQ( 1u, r.size() );
    ASSERT_EQ( r[0]->fileName(), m1->fileName() );

    r = c1->search( "title", nullptr )->all();
    ASSERT_EQ( 2u, r.size() );
    ASSERT_EQ( r[0]->fileName(), m1->fileName() );
    ASSERT_EQ( r[1]->fileName(), m2->fileName() );

    r = c2->search( "title", nullptr )->all();
    ASSERT_EQ( 1u, r.size() );
    ASSERT_EQ( r[0]->fileName(), m3->fileName() );

    r = c2->search( "won't match", nullptr )->all();
    ASSERT_TRUE( r.empty() );
}

static void CheckDbModel( Tests* T )
{
    auto res = Subscription::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void CachedSize( Tests* T )
{
    auto s1 = Subscription::create( T->ml.get(), IService::Type::Podcast, "collection",
                                    "artwork", 0 );
    ASSERT_EQ( s1->cachedSize(), 0u );

    ASSERT_NON_NULL( s1 );
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addExternalMedia( "http://youtu.be/media1", -1 ) );
    ASSERT_NON_NULL( m1 );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file:///path/to/movie.mkv", IMedia::Type::Video ) );
    ASSERT_NON_NULL( m2 );
    auto m3 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file:///path/to/episiode.mkv", IMedia::Type::Video ) );
    ASSERT_NON_NULL( m3 );

    /* Mock files have a default size of 321 */
    auto f1 = std::static_pointer_cast<File>( m1->mainFile() );
    ASSERT_NON_NULL( f1 );
    auto f2 = std::static_pointer_cast<File>( m2->mainFile() );
    ASSERT_NON_NULL( f2 );

    auto res = s1->addMedia( *m1 );
    ASSERT_TRUE( res );
    res = s1->addMedia( *m2 );
    ASSERT_TRUE( res );
    res = s1->addMedia( *m3 );
    ASSERT_TRUE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 0u );

    /* Since media1 is external, it doesn't have a size, let's fix this */
    res = f1->updateFsInfo( 0, 123 );
    ASSERT_TRUE( res );

    res = m1->cache( "file:///path/to/cache/media1.mkv", IFile::CacheType::Manual, f1->size() );
    ASSERT_TRUE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 123u );

    /* Ensure we reject caching a file without a size */
    res = f2->updateFsInfo( 0, 0 );
    ASSERT_TRUE( res );

    res = m2->cache( "file:///path/to/cache/media2.mkv", IFile::CacheType::Manual, f2->size() );
    ASSERT_FALSE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 123u );

    res = f2->updateFsInfo( 0, 987 );
    ASSERT_TRUE( res );

    res = m2->cache( "file:///path/to/cache/media2.mkv", IFile::CacheType::Manual, f2->size() );
    ASSERT_TRUE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 123u + 987u );

    res = s1->removeMedia( m1->id() );
    ASSERT_TRUE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 987u );

    T->ml->deleteMedia( m2->id() );
    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 0u );

    /* Ensure everything works fine when removing an uncached media */
    res = s1->removeMedia( m3->id() );
    ASSERT_TRUE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 0u );
}

static void FetchUncached( Tests* T )
{
    auto s = Subscription::create( T->ml.get(), IService::Type::Podcast, "collection",
                                   "artwork", 0 );
    ASSERT_NON_NULL( s );

    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file:///path/to.mkv", Media::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file:///path/to.avi", Media::Type::Video ) );
    auto m3 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file:///path/to.asf", Media::Type::Video ) );
    ASSERT_NON_NULL( m1 );
    ASSERT_NON_NULL( m2 );
    ASSERT_NON_NULL( m3 );

    s->addMedia( *m1 );
    s->addMedia( *m2 );

    // Cache m3 but don't add it to the collection
    auto res = m3->cache( "file:///path/to/somewhere/irrelevant.mkv", IFile::CacheType::Manual, 1 );
    ASSERT_TRUE( res );

    auto uncached = s->uncachedMedia( false );
    ASSERT_EQ( 2u, uncached.size() );

    res = m1->cache( "file:///path/to/cache.mkv", IFile::CacheType::Manual, 1 );
    ASSERT_TRUE( res );

    uncached = s->uncachedMedia( false );
    ASSERT_EQ( 1u, uncached.size() );
    ASSERT_EQ( m2->id(), uncached[0]->id() );

    res = m1->removeCached();
    ASSERT_TRUE( res );

    uncached = s->uncachedMedia( false );
    ASSERT_EQ( 2u, uncached.size() );
}

static void MaxCachedMedia( Tests* T )
{
    auto s = Subscription::create( T->ml.get(), IService::Type::Podcast, "collection",
                                   "artwork", 0 );
    ASSERT_NON_NULL( s );

    ASSERT_EQ( -1, s->maxCachedMedia() );

    auto res = s->setMaxCachedMedia( 123 );
    ASSERT_TRUE( res );

    ASSERT_EQ( 123, s->maxCachedMedia() );

    s = Subscription::fetch( T->ml.get(), s->id() );
    ASSERT_EQ( 123, s->maxCachedMedia() );

    res = s->setMaxCachedMedia( -123 );
    ASSERT_TRUE( res );
    ASSERT_EQ( -1, s->maxCachedMedia() );

    s = Subscription::fetch( T->ml.get(), s->id() );
    ASSERT_EQ( -1, s->maxCachedMedia() );
}

static void MaxCachedSize( Tests* T )
{
    auto s = Subscription::create( T->ml.get(), IService::Type::Podcast, "collection",
                                    "artwork", 0 );
    ASSERT_NON_NULL( s );

    ASSERT_EQ( -1, s->maxCacheSize() );

    auto res = s->setMaxCacheSize( 123 );
    ASSERT_TRUE( res );

    ASSERT_EQ( 123, s->maxCacheSize() );

    s = Subscription::fetch( T->ml.get(), s->id() );
    ASSERT_EQ( 123, s->maxCacheSize() );

    res = s->setMaxCacheSize( -123 );
    ASSERT_TRUE( res );
    ASSERT_EQ( -1, s->maxCacheSize() );

    s = Subscription::fetch( T->ml.get(), s->id() );
    ASSERT_EQ( -1, s->maxCacheSize() );
}

static void NewMediaNotify( Tests* T )
{
    auto s = Subscription::create( T->ml.get(), IService::Type::Podcast, "collection",
                                   "artwork", 0 );
    ASSERT_NON_NULL( s );

    ASSERT_EQ( -1, s->newMediaNotification() );

    auto res = s->setNewMediaNotification( 124 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 1, s->newMediaNotification() );

    s = Subscription::fetch( T->ml.get(), s->id() );
    ASSERT_EQ( 1, s->newMediaNotification() );

    res = s->setNewMediaNotification( 0 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 0, s->newMediaNotification() );

    s = Subscription::fetch( T->ml.get(), s->id() );
    ASSERT_EQ( 0, s->newMediaNotification() );

    res = s->setNewMediaNotification( -123 );
    ASSERT_TRUE( res );
    ASSERT_EQ( -1, s->newMediaNotification() );

    s = Subscription::fetch( T->ml.get(), s->id() );
    ASSERT_EQ( -1, s->newMediaNotification() );
}

static void NbMedia( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "http://pod.ca/st/episode1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "http://pod.ca/st/episode2.mp3", IMedia::Type::Audio ) );
    ASSERT_NON_NULL( m1 );
    ASSERT_NON_NULL( m2 );

    auto sub = Subscription::create( T->ml.get(), IService::Type::Podcast, "sub",
                                     "artwork", 0 );
    ASSERT_NON_NULL( sub );
    ASSERT_EQ( 0u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 0u, sub->nbMedia() );

    // Add a simple unplayed media
    auto res = sub->addMedia( *m1 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 1u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 1u, sub->nbMedia() );
    sub = Subscription::fetch( T->ml.get(), sub->id() );
    ASSERT_EQ( 1u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 1u, sub->nbMedia() );

    // Add an already played media and remove it
    res = m2->markAsPlayed();
    ASSERT_TRUE( res );
    res = sub->addMedia( *m2 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 1u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 2u, sub->nbMedia() );
    sub = Subscription::fetch( T->ml.get(), sub->id() );
    ASSERT_EQ( 1u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 2u, sub->nbMedia() );

    res = sub->removeMedia( m2->id() );
    ASSERT_TRUE( res );
    ASSERT_EQ( 1u, sub->nbMedia() );

    res = m2->removeFromHistory();
    ASSERT_TRUE( res );

    // Now insert it as unplayed
    res = sub->addMedia( *m2 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 2u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 2u, sub->nbMedia() );
    sub = Subscription::fetch( T->ml.get(), sub->id() );
    ASSERT_EQ( 2u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 2u, sub->nbMedia() );

    // Check that updating the media play_count update the subscription
    res = m1->markAsPlayed();
    ASSERT_TRUE( res );

    sub = Subscription::fetch( T->ml.get(), sub->id() );
    ASSERT_EQ( 1u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 2u, sub->nbMedia() );

    res = m1->removeFromHistory();
    ASSERT_TRUE( res );

    sub = Subscription::fetch( T->ml.get(), sub->id() );
    ASSERT_EQ( 2u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 2u, sub->nbMedia() );

    res = Media::destroy( T->ml.get(), m1->id() );
    ASSERT_TRUE( res );

    sub = Subscription::fetch( T->ml.get(), sub->id() );
    ASSERT_EQ( 1u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 1u, sub->nbMedia() );

    res = sub->removeMedia( m2->id() );
    ASSERT_TRUE( res );

    sub = Subscription::fetch( T->ml.get(), sub->id() );
    ASSERT_EQ( 0u, sub->nbUnplayedMedia() );
    ASSERT_EQ( 0u, sub->nbMedia() );
}

static void SearchAllMedia( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>(
        T->ml->addMedia( "http://pod.ca/st/episode1.mp3", IMedia::Type::Audio ) );
    auto m2 = std::static_pointer_cast<Media>(
        T->ml->addMedia( "http://pod.ca/st/episode2.mp3", IMedia::Type::Audio ) );
    ASSERT_NON_NULL( m1 );
    ASSERT_NON_NULL( m2 );

    auto sub = Subscription::create( T->ml.get(), IService::Type::Podcast, "sub", "artwork", 0 );
    ASSERT_NON_NULL( sub );
    auto sub2 = Subscription::create( T->ml.get(), IService::Type::Podcast, "sub2", "artwork", 0 );
    ASSERT_NON_NULL( sub );

    auto res = T->ml->searchSubscriptionMedia( "epi" )->all();
    ASSERT_TRUE( res.empty() );

    sub->addMedia( *m1 );
    sub->addMedia( *m2 );

    res = T->ml->searchSubscriptionMedia( "epi" )->all();
    ASSERT_EQ( res.size(), 2u );
    ASSERT_EQ( res[0]->id(), m1->id() );
    ASSERT_EQ( res[1]->id(), m2->id() );
}

int main( int ac, char** av )
{
    INIT_TESTS( Subscription )

    ADD_TEST( Create );
    ADD_TEST( ListFromService );
    ADD_TEST( ListFromMedia );
    ADD_TEST( ChildSubscriptions );
    ADD_TEST( ListMedia );
    ADD_TEST( SearchMedia );
    ADD_TEST( CheckDbModel );
    ADD_TEST( CachedSize );
    ADD_TEST( FetchUncached );
    ADD_TEST( MaxCachedMedia );
    ADD_TEST( MaxCachedSize );
    ADD_TEST( NewMediaNotify );
    ADD_TEST( NbMedia );
    ADD_TEST( SearchAllMedia );

    END_TESTS
}
