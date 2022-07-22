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

#pragma once

#include <memory>

#include "compat/Thread.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"
#include "medialibrary/IFile.h"

#include <queue>
#include <atomic>

namespace medialibrary
{

class Media;
class File;
class MediaLibrary;
class Subscription;
class ICacher;

class CacheWorker
{
private:
    struct Task
    {
        Task() = default;
        Task( std::shared_ptr<Media> m, bool c );
        std::shared_ptr<Media> m; // If nullptr, we're caching all subscriptions
        bool cache; // true if we're caching, false if we're uncaching
    };

public:
    CacheWorker( MediaLibrary* ml );
    ~CacheWorker();
    void setCacher( std::shared_ptr<ICacher> cacher );
    bool cacheMedia( std::shared_ptr<Media> m );
    bool removeCached( std::shared_ptr<Media> m );
    void cacheSubscriptions();
    void pause();
    void resume();
    void stop();
    uint64_t cacheSize() const;

private:
    void run();
    uint64_t doCache( std::shared_ptr<Media> m, Subscription* s,
                      IFile::CacheType cacheType );
    void doUncache( std::shared_ptr<Media> m );
    void doSubscriptionCache();
    void checkCache();
    bool removeFromCache( const std::string& mrl );
    bool evictIfNeeded( const File& file, Subscription* s, IFile::CacheType cacheType );
    void queueTask( std::shared_ptr<Media> m, bool cache );
    uint64_t availableSubscriptionCacheSize() const;
    uint64_t availableCacheSize() const;

private:
    MediaLibrary* m_ml;
    compat::Mutex m_mutex;
    compat::Thread m_thread;
    compat::ConditionVariable m_cond;
    std::queue<Task> m_tasks;
    bool m_paused;
    bool m_run;
    std::shared_ptr<ICacher> m_cacher;

    std::atomic_uint64_t m_cacheSize;
};
}

