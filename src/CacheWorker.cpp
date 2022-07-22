/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright © 2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "CacheWorker.h"
#include "Media.h"
#include "Subscription.h"
#include "utils/Url.h"
#include "utils/File.h"
#include "utils/Filename.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/ICacher.h"

#include <algorithm>

namespace medialibrary
{

CacheWorker::CacheWorker( MediaLibrary* ml )
    : m_ml( ml )
    , m_paused( false )
    , m_run( true )
{
}

CacheWorker::~CacheWorker()
{
    stop();
}

void CacheWorker::setCacher( std::shared_ptr<ICacher> cacher )
{
    assert( m_cacher == nullptr );
    m_cacher = std::move( cacher );
}

void CacheWorker::queueTask( std::shared_ptr<Media> m, bool cache )
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    m_tasks.emplace( std::move( m ), cache );
    if ( m_thread.joinable() == false )
        m_thread = compat::Thread{ &CacheWorker::run, this };
    else
        m_cond.notify_all();
}

bool CacheWorker::cacheMedia( std::shared_ptr<Media> m )
{
    auto f = m->mainFile();
    if ( f == nullptr )
        return false;
    if ( f->type() == IFile::Type::Cache )
    {
        LOG_DEBUG( "Media ", m->id(), " is already cached" );
        return true;
    }
    queueTask( std::move( m ), true );
    return true;
}

bool CacheWorker::removeCached( std::shared_ptr<Media> m )
{
    auto f = m->mainFile();
    if ( f == nullptr )
        return false;
    if ( f->type() != IFile::Type::Cache )
    {
        LOG_DEBUG( "Media ", m->id(), " is not cached" );
        return false;
    }
    queueTask( std::move( m ), false );
    return true;
}

void CacheWorker::cacheSubscriptions()
{
    queueTask( nullptr, true );
}

void CacheWorker::pause()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    m_paused = true;
    m_cond.notify_one();
}

void CacheWorker::resume()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    m_paused = false;
    m_cond.notify_one();
}

void CacheWorker::stop()
{
    m_cacher->interrupt();
    {
        std::lock_guard<compat::Mutex> lock{ m_mutex };
        if ( m_thread.joinable() == false )
            return;
        m_run = false;
        m_cond.notify_one();
    }
    m_thread.join();
}

uint64_t CacheWorker::cacheSize() const
{
    return m_cacheSize.load( std::memory_order_acquire );
}

uint64_t CacheWorker::availableSubscriptionCacheSize() const
{
    auto usedSize = cacheSize();
    auto totalSize = m_ml->settings().maxSubscriptionCacheSize();
    if ( usedSize > totalSize )
    {
        LOG_WARN( "Subscription cache is overused: ", usedSize, " / ", totalSize );
        return 0;
    }
    return totalSize - usedSize;
}

void CacheWorker::run()
{
    auto cb = m_ml->getCb();

    LOG_DEBUG( "Starting cache worker" );
    checkCache();
    while ( true )
    {
        Task t;
        {
            std::unique_lock<compat::Mutex> lock{ m_mutex };
            if ( m_run == false )
                break;
            if ( m_paused == true || m_tasks.empty() == true )
            {
                if ( cb != nullptr )
                    cb->onCacheIdleChanged( true );
                m_cond.wait( lock, [this]() {
                    return m_run == false ||
                           ( m_paused == false && m_tasks.empty() == false );
                });
                if ( m_run == false )
                    break;
                assert( m_paused == false );
                if ( cb != nullptr )
                    cb->onCacheIdleChanged( false );
            }

            t = std::move( m_tasks.front() );
            m_tasks.pop();
        }
        if ( t.m == nullptr )
            doSubscriptionCache();
        else if ( t.cache == true )
            doCache( std::move( t.m ), nullptr, File::CacheType::Manual );
        else
            doUncache( std::move( t.m ) );
    }
    LOG_DEBUG( "Terminating cache worker" );
}

uint64_t CacheWorker::doCache( std::shared_ptr<Media> m, Subscription* c,
                               IFile::CacheType cacheType )
{
    auto f = std::static_pointer_cast<File>( m->mainFile() );
    if ( f == nullptr )
    {
        assert( !"No main file for the media" );
        return 0;
    }
    if ( f->type() == IFile::Type::Cache )
    {
        assert( !"Media was already cached" );
        return 0;
    }
    LOG_DEBUG( "Attempting to ", ( cacheType == IFile::CacheType::Automatic ?
                 "automatically" : "manually" ), " cache file at ", f->mrl() );
    if ( evictIfNeeded( *f, c, cacheType ) == false )
    {
        LOG_DEBUG( "Failed to evict media from cache, can't cache ", f->mrl() );
        return 0;
    }
    auto cachedPath = m_ml->cachePath() + f->cachedFileName();
    if ( m_cacher->cache( f->mrl(), cachedPath ) == false )
        return 0;
    auto fileSize = f->size();
    if ( fileSize == 0 )
        fileSize = utils::fs::fileSize( cachedPath );
    m->cache( utils::file::toMrl( cachedPath ), cacheType, fileSize );
    return f->size();
}

bool CacheWorker::removeFromCache( const std::string& mrl )
{
    assert( utils::url::schemeIs( "file://", mrl ) );
    auto path = utils::url::toLocalPath( mrl );
    return utils::fs::remove( path );
}

void CacheWorker::doUncache( std::shared_ptr<Media> m )
{
    auto f = std::static_pointer_cast<File>( m->mainFile() );
    if ( f == nullptr || f->type() != IFile::Type::Cache )
    {
        assert( !"The media has no cached file" );
        return;
    }
    const auto& mrl = f->mrl();
    if ( removeFromCache( mrl ) == false )
        return;
    m->removeCached();
}

void CacheWorker::doSubscriptionCache()
{
    auto cb = m_ml->getCb();
    auto subscriptions = Subscription::fetchAll( m_ml );
    for ( auto& s : subscriptions )
    {
        auto uncachedMedia = s->uncachedMedia( true );

        if ( uncachedMedia.empty() == false )
        {
            for ( auto& m : uncachedMedia )
            {
                doCache( m, s.get(), File::CacheType::Automatic );
            }
            if ( cb != nullptr )
                cb->onSubscriptionCacheUpdated( s->id() );
        }
        s->markCacheAsHandled();
    }
}

bool CacheWorker::evictIfNeeded( const File& file, Subscription* c,
                                 IFile::CacheType cacheType )
{
    if ( cacheType == File::CacheType::Automatic )
    {
        /*
         * We only want to check the number of cached media for a subscription
         * when doing automatic caching.
         */
        auto maxMedia = c->maxCachedMedia();
        if ( maxMedia < 0 )
        {
            LOG_DEBUG( "No subscription settings, falling back to global settings" );
            maxMedia = m_ml->settings().nbCachedMediaPerSubscription();
        }
        auto nbCachedMediaInSub = c->cachedMedia( false )->count();
        LOG_DEBUG( "Subscription #", c->id(), " has ", nbCachedMediaInSub,
                   "/", maxMedia, " cached media" );
        if ( nbCachedMediaInSub >= static_cast<uint32_t>( maxMedia ) )
        {
            auto toEvict = c->cachedMedia( true )->items( 1, 0 );
            if ( toEvict.size() != 1 )
                return false;
            auto f = toEvict[0]->mainFile();
            if ( f->type() != File::Type::Cache )
            {
                assert( !"Invalid file type" );
                return false;
            }
            if ( removeFromCache( f->mrl() ) == false )
                return false;
            if ( toEvict[0]->removeCached() == false )
                return false;
            m_cacheSize.fetch_sub( f->size(), std::memory_order_acq_rel );
        }
    }
    auto fileSize = file.size();
    auto evictionNeeded = fileSize > availableSubscriptionCacheSize();
    if ( evictionNeeded == false )
        return true;

    auto cachedFiles = File::cachedFiles( m_ml );
    auto cachedFilesIdx = 0;
    assert( cachedFiles.empty() == false );

    while ( evictionNeeded == true )
    {
        auto f = std::move( cachedFiles[cachedFilesIdx] );
        const auto& mrl = f->mrl();
        if ( removeFromCache( mrl ) == false )
            return false;
        if ( cachedFiles[cachedFilesIdx]->destroy() == false )
            return false;
        m_cacheSize.fetch_sub( cachedFiles[cachedFilesIdx]->size(),
                               std::memory_order_acq_rel );
        evictionNeeded = fileSize > availableSubscriptionCacheSize();
    }
    return true;
}

void CacheWorker::checkCache()
{
    auto cachePath = m_ml->cachePath();
    auto cacheMrl = utils::file::toMrl( cachePath );
    auto fsFactory = m_ml->fsFactoryForMrl( cacheMrl );
    auto fsDir = fsFactory->createDirectory( cacheMrl );
    auto files = fsDir->files();

    struct Comp
    {
        bool operator()( const std::shared_ptr<fs::IFile>& f, const std::string& fileName ) const
        {
            return f->name() < fileName;
        }
        bool operator()( const std::string& fileName, const std::shared_ptr<fs::IFile>& f ) const
        {
            return fileName < f->name();
        }
        bool operator()( const std::shared_ptr<fs::IFile>& l, const std::shared_ptr<fs::IFile>& r ) const
        {
            return l->name() < r->name();
        }
    };

    // Sort the files from the cache folder to fasten up following lookups
    std::sort( begin( files ), end( files ), Comp{} );

    // Fetch the known cached files from the DB
    auto cachedFiles = File::cachedFiles( m_ml );

    m_cacheSize.store( 0, std::memory_order_release );
    for ( auto cachedIt = begin( cachedFiles ); cachedIt != end( cachedFiles ); )
    {
        auto wantedFileName = utils::url::encode( (*cachedIt)->cachedFileName() );

        auto it = std::lower_bound( begin( files ), end( files ), wantedFileName,
                                 Comp{} );
        if ( it != end( files ) )
        {
            if ( (*it)->name() == wantedFileName )
            {
                /* The cached file was indeed found it cache, all is well */
                auto fileSize = (*it)->size();
                assert( fileSize != 0 );
                m_cacheSize.fetch_add( fileSize, std::memory_order_acq_rel );

                files.erase( it );
                cachedIt = cachedFiles.erase( cachedIt );
                continue;
            }
        }
        /* A file flagged as cached in DB was not found on disk, unflag it */
        LOG_DEBUG( "File ", (*cachedIt)->rawMrl(), " was flagged as cached but "
                   "wasn't found on disk. Unflagging it." );
        (*cachedIt)->destroy();
        cachedIt = cachedFiles.erase( cachedIt );
    }
    /*
     * Whatever remains in the files vector are files that were not flagged as cached
     * in DB.
     * For now, take the easy way out and remove the cached version without touching
     * the database, but ideally we should flag the file as cached
     */
    for ( const auto& f : files )
    {
        auto path = utils::url::toLocalPath( f->mrl() );
        LOG_DEBUG( "Removing stale file from cache: ", path );
        utils::fs::remove( path );
    }
}

CacheWorker::Task::Task( std::shared_ptr<Media> m, bool c )
    : m( std::move( m ) )
    , cache( c )
{
}

} // namespace medialibrary
