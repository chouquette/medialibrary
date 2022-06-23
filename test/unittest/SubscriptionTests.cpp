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
    auto c = Subscription::create( T->ml.get(), Service::Podcast, "collection", 0 );
    ASSERT_NON_NULL( c );
    ASSERT_EQ( c->name(), "collection" );
}

static void ListFromService( Tests* T )
{
    auto collectionQuery = T->ml->subscriptions( Service::Podcast, nullptr );
    ASSERT_EQ( 0u, collectionQuery->count() );
    auto collections = collectionQuery->all();
    ASSERT_TRUE( collections.empty() );

    auto c = Subscription::create( T->ml.get(), Service::Podcast, "Z collection", 0 );
    ASSERT_NON_NULL( c );
    auto c2 = Subscription::create( T->ml.get(), Service::Podcast, "A collection", 0 );
    ASSERT_NON_NULL( c2 );

    collectionQuery = T->ml->subscriptions( Service::Podcast, nullptr );
    ASSERT_EQ( 2u, collectionQuery->count() );
    collections = collectionQuery->all();
    ASSERT_EQ( 2u, collections.size() );

    ASSERT_EQ( c2->id(), collections[0]->id() );
    ASSERT_EQ( c->id(), collections[1]->id() );

    QueryParameters params{};
    params.desc = true;

    collectionQuery = T->ml->subscriptions( Service::Podcast, &params );
    ASSERT_EQ( 2u, collectionQuery->count() );
    collections = collectionQuery->all();
    ASSERT_EQ( 2u, collections.size() );

    ASSERT_EQ( c->id(), collections[0]->id() );
    ASSERT_EQ( c2->id(), collections[1]->id() );
}

static void ChildSubscriptions( Tests* T )
{
    auto c = Subscription::create( T->ml.get(), Service::Podcast, "collection", 0 );
    ASSERT_NON_NULL( c );
    auto scQuery = c->childSubscriptions( nullptr );
    ASSERT_EQ( 0u, scQuery->count() );
    auto sc = scQuery->all();
    ASSERT_EQ( 0u, sc.size() );

    auto sc1 = c->addChildSubscription( "Z" );
    ASSERT_NON_NULL( sc1 );
    auto sc2 = c->addChildSubscription( "A" );
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
    auto collections = T->ml->subscriptions( Service::Podcast, nullptr )->all();
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
    auto c1 = Subscription::create( T->ml.get(), Service::Podcast, "collection", 0 );
    ASSERT_NON_NULL( c1 );
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addExternalMedia( "http://youtu.be/media1", -1 ) );
    ASSERT_NON_NULL( m1 );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file:///path/to/movie.mkv", IMedia::Type::Video ) );
    ASSERT_NON_NULL( m2 );
    c1->addMedia( *m1 );
    c1->addMedia( *m2 );

    auto c2 = Subscription::create( T->ml.get(), Service::Podcast, "another collection", 0 );
    ASSERT_NON_NULL( c2 );
    auto m3 = T->ml->addExternalMedia( "http://podcast.io/something.mp3", -1 );
    ASSERT_NON_NULL( m3 );

    auto mediaQuery = c1->media( nullptr );
    ASSERT_EQ( 2u, mediaQuery->count() );
    auto media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );
}

static void CheckDbModel( Tests* T )
{
    auto res = Subscription::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void CachedSize( Tests* T )
{
    auto s1 = Subscription::create( T->ml.get(), Service::Podcast, "collection", 0 );
    ASSERT_EQ( s1->cachedSize(), 0 );

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
    ASSERT_EQ( s1->cachedSize(), 0 );

    /* Since media1 is external, it doesn't have a size, let's fix this */
    res = f1->updateFsInfo( 0, 123 );
    ASSERT_TRUE( res );

    res = m1->cache( "file:///path/to/cache/media1.mkv", IFile::CacheType::Manual, f1->size() );
    ASSERT_TRUE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 123 );

    /* Ensure we reject caching a file without a size */
    res = f2->updateFsInfo( 0, 0 );
    ASSERT_TRUE( res );

    res = m2->cache( "file:///path/to/cache/media2.mkv", IFile::CacheType::Manual, f2->size() );
    ASSERT_FALSE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 123 );

    res = f2->updateFsInfo( 0, 987 );
    ASSERT_TRUE( res );

    res = m2->cache( "file:///path/to/cache/media2.mkv", IFile::CacheType::Manual, f2->size() );
    ASSERT_TRUE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 123 + 987 );

    res = s1->removeMedia( m1->id() );
    ASSERT_TRUE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 987 );

    T->ml->deleteMedia( m2->id() );
    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 0 );

    /* Ensure everything works fine when removing an uncached media */
    res = s1->removeMedia( m3->id() );
    ASSERT_TRUE( res );

    s1 = Subscription::fetch( T->ml.get(), s1->id() );
    ASSERT_EQ( s1->cachedSize(), 0 );
}

static void FetchUncached( Tests* T )
{
    auto s = Subscription::create( T->ml.get(), Service::Podcast, "collection", 0 );
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
    auto s = Subscription::create( T->ml.get(), Service::Podcast, "collection", 0 );
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
    auto s = Subscription::create( T->ml.get(), Service::Podcast, "collection", 0 );
    ASSERT_NON_NULL( s );

    ASSERT_EQ( -1, s->maxCachedSize() );

    auto res = s->setMaxCachedSize( 123 );
    ASSERT_TRUE( res );

    ASSERT_EQ( 123, s->maxCachedSize() );

    s = Subscription::fetch( T->ml.get(), s->id() );
    ASSERT_EQ( 123, s->maxCachedSize() );

    res = s->setMaxCachedSize( -123 );
    ASSERT_TRUE( res );
    ASSERT_EQ( -1, s->maxCachedSize() );

    s = Subscription::fetch( T->ml.get(), s->id() );
    ASSERT_EQ( -1, s->maxCachedSize() );
}

int main( int ac, char** av )
{
    INIT_TESTS( Subscription )

    ADD_TEST( Create );
    ADD_TEST( ListFromService );
    ADD_TEST( ChildSubscriptions );
    ADD_TEST( ListMedia );
    ADD_TEST( CheckDbModel );
    ADD_TEST( CachedSize );
    ADD_TEST( FetchUncached );
    ADD_TEST( MaxCachedMedia );
    ADD_TEST( MaxCachedSize );

    END_TESTS
}
