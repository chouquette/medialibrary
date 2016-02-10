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

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include "Types.h"

class IMediaLibraryCb;

class ModificationNotifier
{
public:
    ModificationNotifier( MediaLibraryPtr ml );
    ~ModificationNotifier();

    void start();
    void notifyMediaCreation( MediaPtr media );
    void notifyMediaModification( MediaPtr media );
    void notifyMediaRemoval( int64_t media );

    void notifyArtistCreation( ArtistPtr artist );
    void notifyArtistModification( ArtistPtr artist );
    void notifyArtistRemoval( int64_t artist );

private:
    void run();
    void notify();

private:
    template <typename T>
    struct Queue
    {
        std::vector<std::shared_ptr<T>> added;
        std::vector<std::shared_ptr<T>> modified;
        std::vector<int64_t> removed;
        std::chrono::time_point<std::chrono::steady_clock> timeout;
    };

    template <typename T, typename AddedCb, typename ModifiedCb, typename RemovedCb>
    void notify( Queue<T>&& queue, AddedCb addedCb, ModifiedCb modifiedCb, RemovedCb removedCb )
    {
        if ( queue.added.size() > 0 )
            (*m_cb.*addedCb)( std::move( queue.added ) );
        if ( queue.modified.size() > 0 )
            (*m_cb.*modifiedCb)( std::move( queue.modified ) );
        if ( queue.removed.size() > 0 )
            (*m_cb.*removedCb)( std::move( queue.removed ) );
    }

    template <typename T>
    void notifyCreation( std::shared_ptr<T> entity, Queue<T>& queue )
    {
        std::lock_guard<std::mutex> lock( m_lock );
        queue.added.push_back( std::move( entity ) );
        updateTimeout( queue );
    }

    template <typename T>
    void notifyModification( std::shared_ptr<T> entity, Queue<T>& queue )
    {
        std::lock_guard<std::mutex> lock( m_lock );
        queue.modified.push_back( std::move( entity ) );
        updateTimeout( queue );
    }

    template <typename T>
    void notifyRemoval( int64_t rowId, Queue<T>& queue )
    {
        std::lock_guard<std::mutex> lock( m_lock );
        queue.removed.push_back( rowId );
        updateTimeout( m_media );
    }

    template <typename T>
    void updateTimeout( Queue<T>& queue )
    {
        queue.timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds{ 500 };
        if ( m_timeout == std::chrono::time_point<std::chrono::steady_clock>{} )
        {
            // If no wake up has been scheduled, schedule one now
            m_timeout = queue.timeout;
            m_cond.notify_all();
        }
    }

    template <typename T>
    void checkQueue( Queue<T>& input, Queue<T>& output, std::chrono::time_point<std::chrono::steady_clock>& nextTimeout,
                     std::chrono::time_point<std::chrono::steady_clock> now )
    {
        constexpr auto ZeroTimeout = std::chrono::time_point<std::chrono::steady_clock>{};
        //    LOG_ERROR( "Input timeout: ", input.timeout.time_since_epoch(), " - Now: ", now.time_since_epoch() );
        if ( input.timeout <= now )
        {
            using std::swap;
            swap( input, output );
        }
        // Or is scheduled for timeout soon:
        else if ( input.timeout != ZeroTimeout && ( nextTimeout == ZeroTimeout || input.timeout < nextTimeout ) )
        {
            nextTimeout = input.timeout;
        }
    }

private:
    MediaLibraryPtr m_ml;
    IMediaLibraryCb* m_cb;

    // Queues
    Queue<IMedia> m_media;
    Queue<IArtist> m_artists;

    // Notifier thread
    std::mutex m_lock;
    std::condition_variable m_cond;
    std::thread m_notifierThread;
    std::atomic_bool m_stop;
    std::chrono::time_point<std::chrono::steady_clock> m_timeout;
};


