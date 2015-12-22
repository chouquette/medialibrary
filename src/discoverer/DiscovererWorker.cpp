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

DiscovererWorker::DiscovererWorker()
    : m_run( false )
    , m_cb( nullptr )
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

void DiscovererWorker::setCallback(IMediaLibraryCb* cb)
{
    m_cb = cb;
}

void DiscovererWorker::stop()
{
    bool running = true;
    if ( m_run.compare_exchange_strong( running, false ) )
    {
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            while ( m_entryPoints.empty() == false )
                m_entryPoints.pop();
        }
        m_cond.notify_all();
        m_thread.join();
    }
}

bool DiscovererWorker::discover( const std::string& entryPoint )
{
    if ( entryPoint.length() == 0 )
        return false;
    enqueue( entryPoint );
    return true;
}

void DiscovererWorker::reload()
{
    enqueue( "" );
}

void DiscovererWorker::enqueue( const std::string& entryPoint )
{
    std::unique_lock<std::mutex> lock( m_mutex );

    m_entryPoints.emplace( entryPoint );
    if ( m_thread.get_id() == std::thread::id{} )
    {
        m_run = true;
        m_thread = std::thread( &DiscovererWorker::run, this );
    }
    // Since we just added an element, let's not check for size == 0 :)
    else if ( m_entryPoints.size() == 1 )
        m_cond.notify_all();
}

void DiscovererWorker::run()
{
    LOG_INFO( "Entering DiscovererWorker thread" );
    while ( m_run == true )
    {
        std::string entryPoint;
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            if ( m_entryPoints.size() == 0 )
            {
                m_cond.wait( lock, [this]() { return m_entryPoints.size() > 0 || m_run == false ; } );
                if ( m_run == false )
                    break;
            }
            entryPoint = m_entryPoints.front();
            m_entryPoints.pop();
        }
        if ( entryPoint.length() > 0 )
        {
            if ( m_cb != nullptr )
                m_cb->onDiscoveryStarted( entryPoint );
            for ( auto& d : m_discoverers )
            {
                // Assume only one discoverer can handle an entrypoint.
                try
                {
                    if ( d->discover( entryPoint ) == true )
                        break;
                }
                catch(std::exception& ex)
                {
                    LOG_ERROR( "Fatal error while discovering ", entryPoint, ": ", ex.what() );
                }

                if ( m_run == false )
                    break;
            }
            if ( m_cb != nullptr )
                m_cb->onDiscoveryCompleted( entryPoint );
        }
        else
        {
            if ( m_cb != nullptr )
                m_cb->onReloadStarted();
            for ( auto& d : m_discoverers )
            {
                try
                {
                    d->reload();
                }
                catch(std::exception& ex)
                {
                    LOG_ERROR( "Fatal error while reloading: ", ex.what() );
                }
                if ( m_run == false )
                    break;
            }
            if ( m_cb != nullptr )
                m_cb->onReloadCompleted();
        }
    }
    LOG_INFO( "Exiting DiscovererWorker thread" );
}

