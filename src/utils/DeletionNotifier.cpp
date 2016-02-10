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

#include <cassert>

#include "DeletionNotifier.h"
#include "MediaLibrary.h"

DeletionNotifier::DeletionNotifier( MediaLibraryPtr ml )
    : m_ml( ml )
    , m_cb( ml->getCb() )
{
}

DeletionNotifier::~DeletionNotifier()
{
    if ( m_notifierThread.joinable() == true )
    {
        m_stop = true;
        m_cond.notify_all();
        m_notifierThread.join();
    }
}

void DeletionNotifier::start()
{
    assert( m_notifierThread.get_id() == std::thread::id{} );
    m_stop = false;
    m_notifierThread = std::thread{ &DeletionNotifier::run, this };
}

void DeletionNotifier::notifyMediaRemoval( int64_t mediaId )
{
    notifyRemoval( mediaId, m_media );
}

void DeletionNotifier::notifyRemoval( int64_t rowId, DeletionNotifier::Queue& queue )
{
    std::lock_guard<std::mutex> lock( m_lock );
    queue.entities.push_back( rowId );
    queue.timeout = std::chrono::steady_clock::now() + std::chrono::seconds{ 5 };
    if ( m_timeout == std::chrono::time_point<std::chrono::steady_clock>{} )
    {
        LOG_ERROR( "Scheduling wakeup in 5s" );
        // If no wake up has been scheduled, schedule one now
        m_timeout = queue.timeout;
        m_cond.notify_all();
    }
    else if ( queue.entities.size() >= BatchSize )
    {
        LOG_ERROR("Batch size reached");
        m_cond.notify_all();
    }
}


void DeletionNotifier::run()
{
    constexpr auto ZeroTimeout = std::chrono::time_point<std::chrono::steady_clock>{};

    // Create some other queue to swap with the ones that are used
    // by other threads. That way we can release those early and allow
    // more insertions to proceed
    Queue media;

    while ( m_stop == false )
    {
        {
            std::unique_lock<std::mutex> lock( m_lock );
            if ( m_timeout == ZeroTimeout )
                m_cond.wait( lock, [this, ZeroTimeout](){ return m_timeout != ZeroTimeout || m_stop == true; } );
            LOG_ERROR("Waking up from endless cond" );
            m_cond.wait_until( lock, m_timeout, [this]() {
                LOG_ERROR("Checking pred");
                return m_stop == true ||
                    m_media.entities.size() == BatchSize;
            });
            LOG_ERROR("Notifier timedout");
            if ( m_stop == true )
                break;
            auto now = std::chrono::steady_clock::now();
            auto nextTimeout = ZeroTimeout;
            checkQueue( m_media, media, nextTimeout, now );
            m_timeout = nextTimeout;
        }
        notify( std::move( media ), &IMediaLibraryCb::onMediaDeleted );
    }
}

void DeletionNotifier::checkQueue( DeletionNotifier::Queue& input, DeletionNotifier::Queue& output,
                                   std::chrono::time_point<std::chrono::steady_clock>& nextTimeout,
                                   std::chrono::time_point<std::chrono::steady_clock> now)
{
    constexpr auto ZeroTimeout = std::chrono::time_point<std::chrono::steady_clock>{};
//    LOG_ERROR( "Input timeout: ", input.timeout.time_since_epoch(), " - Now: ", now.time_since_epoch() );
    if ( input.timeout <= now || input.entities.size() >= BatchSize )
    {
        LOG_ERROR("Swapping tmp & actual queues");
        using std::swap;
        swap( input, output );
    }
    // Or is scheduled for timeout soon:
    else if ( input.timeout != ZeroTimeout && ( nextTimeout == ZeroTimeout || input.timeout < nextTimeout ) )
    {
        LOG_ERROR("Refreshing timeout");
        nextTimeout = input.timeout;
    }
}
