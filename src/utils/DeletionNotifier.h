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

class DeletionNotifier
{
public:
    DeletionNotifier( MediaLibraryPtr ml );
    ~DeletionNotifier();

    void start();
    void notifyMediaRemoval( int64_t media );

private:
    void run();
    void notify();

private:
    struct Queue
    {
        std::vector<int64_t> entities;
        std::chrono::time_point<std::chrono::steady_clock> timeout;
    };

    template <typename Func>
    void notify( Queue&& queue, Func f )
    {
        if ( queue.entities.size() == 0 )
            return;
        (*m_cb.*f)( std::move( queue.entities ) );
    }

    void notifyRemoval( int64_t rowId, Queue& queue );

    void checkQueue( Queue& input, Queue& output, std::chrono::time_point<std::chrono::steady_clock>& nextTimeout,
                     std::chrono::time_point<std::chrono::steady_clock> now );

private:
    MediaLibraryPtr m_ml;
    IMediaLibraryCb* m_cb;

    // Queues
    Queue m_media;

    // Notifier thread
    std::mutex m_lock;
    std::condition_variable m_cond;
    std::thread m_notifierThread;
    std::atomic_bool m_stop;
    std::chrono::time_point<std::chrono::steady_clock> m_timeout;
};


