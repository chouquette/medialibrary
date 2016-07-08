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

#include "DiscovererWorker.h"

#include "logging/Logger.h"
#include "MediaLibrary.h"

namespace medialibrary
{

DiscovererWorker::DiscovererWorker( MediaLibraryPtr ml )
    : m_run( false )
    , m_cb( ml->getCb() )
{
}

DiscovererWorker::~DiscovererWorker()
{
    stop();
}

void DiscovererWorker::addDiscoverer( std::unique_ptr<IDiscoverer> discoverer )
{
    m_discoverers.push_back( std::move( discoverer ) );
}

void DiscovererWorker::stop()
{
    bool running = true;
    if ( m_run.compare_exchange_strong( running, false ) )
    {
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            while ( m_tasks.empty() == false )
                m_tasks.pop();
        }
        m_cond.notify_all();
        m_thread.join();
    }
}

bool DiscovererWorker::discover( const std::string& entryPoint )
{
    if ( entryPoint.length() == 0 )
        return false;
    enqueue( entryPoint, false );
    return true;
}

void DiscovererWorker::reload()
{
    enqueue( "", true );
}

void DiscovererWorker::reload( const std::string& entryPoint )
{
    enqueue( entryPoint, true );
}

void DiscovererWorker::enqueue( const std::string& entryPoint, bool reload )
{
    std::unique_lock<std::mutex> lock( m_mutex );

    m_tasks.emplace( entryPoint, reload );
    if ( m_thread.get_id() == std::thread::id{} )
    {
        m_run = true;
        m_thread = std::thread( &DiscovererWorker::run, this );
    }
    // Since we just added an element, let's not check for size == 0 :)
    else if ( m_tasks.size() == 1 )
        m_cond.notify_all();
}

void DiscovererWorker::run()
{
    LOG_INFO( "Entering DiscovererWorker thread" );
    while ( m_run == true )
    {
        Task task;
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            if ( m_tasks.size() == 0 )
            {
                m_cond.wait( lock, [this]() { return m_tasks.size() > 0 || m_run == false; } );
                if ( m_run == false )
                    break;
            }
            task = m_tasks.front();
            m_tasks.pop();
        }
        m_cb->onDiscoveryStarted( task.entryPoint );
        if ( task.reload == false )
        {
            for ( auto& d : m_discoverers )
            {
                // Assume only one discoverer can handle an entrypoint.
                try
                {
                    if ( d->discover( task.entryPoint ) == true )
                        break;
                }
                catch(std::exception& ex)
                {
                    LOG_ERROR( "Fatal error while discovering ", task.entryPoint, ": ", ex.what() );
                }

                if ( m_run == false )
                    break;
            }
        }
        else
        {
            for ( auto& d : m_discoverers )
            {
                try
                {
                    if ( task.entryPoint.empty() == true )
                        d->reload();
                    else
                        d->reload( task.entryPoint );
                }
                catch(std::exception& ex)
                {
                    LOG_ERROR( "Fatal error while reloading: ", ex.what() );
                }
                if ( m_run == false )
                    break;
            }
        }
        m_cb->onDiscoveryCompleted( task.entryPoint );
    }
    LOG_INFO( "Exiting DiscovererWorker thread" );
}

}
