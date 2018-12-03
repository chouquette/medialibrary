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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "DiscovererWorker.h"

#include "logging/Logger.h"
#include "Folder.h"
#include "Media.h"
#include "MediaLibrary.h"
#include "utils/Filename.h"
#include <cassert>

namespace medialibrary
{

DiscovererWorker::DiscovererWorker(MediaLibrary* ml )
    : m_run( false )
    , m_ml( ml )
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
            std::unique_lock<compat::Mutex> lock( m_mutex );
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
    LOG_INFO( "Adding ", entryPoint, " to the folder discovery list" );
    enqueue( utils::file::toFolderPath( entryPoint ), Task::Type::Discover );
    return true;
}

void DiscovererWorker::remove( const std::string& entryPoint )
{
    enqueue( entryPoint, Task::Type::Remove );
}

void DiscovererWorker::reload()
{
    enqueue( "", Task::Type::Reload );
}

void DiscovererWorker::reload( const std::string& entryPoint )
{
    enqueue( utils::file::toFolderPath( entryPoint ), Task::Type::Reload );
}

void DiscovererWorker::ban( const std::string& entryPoint )
{
    enqueue( utils::file::toFolderPath( entryPoint ), Task::Type::Ban );
}

void DiscovererWorker::unban( const std::string& entryPoint )
{
    enqueue( utils::file::toFolderPath( entryPoint ), Task::Type::Unban );
}

void DiscovererWorker::enqueue( const std::string& entryPoint, Task::Type type )
{
    std::unique_lock<compat::Mutex> lock( m_mutex );

    LOG_INFO( "Queuing entrypoint ", entryPoint, " of type ",
              static_cast<typename std::underlying_type<Task::Type>::type>( type ) );
    m_tasks.emplace( entryPoint, type );
    notify();
}

void DiscovererWorker::notify()
{
    if ( m_thread.get_id() == compat::Thread::id{} )
    {
        m_run = true;
        m_thread = compat::Thread( &DiscovererWorker::run, this );
    }
    // Since we just added an element, let's not check for size == 0 :)
    else if ( m_tasks.size() == 1 )
        m_cond.notify_all();
}

void DiscovererWorker::run()
{
    LOG_INFO( "Entering DiscovererWorker thread" );
    m_ml->onDiscovererIdleChanged( false );
    while ( m_run == true )
    {
        Task task;
        {
            std::unique_lock<compat::Mutex> lock( m_mutex );
            if ( m_tasks.size() == 0 )
            {
                m_ml->onDiscovererIdleChanged( true );
                m_cond.wait( lock, [this]() { return m_tasks.size() > 0 || m_run == false; } );
                if ( m_run == false )
                    break;
                m_ml->onDiscovererIdleChanged( false );
            }
            task = m_tasks.front();
            m_tasks.pop();
        }
        switch ( task.type )
        {
        case Task::Type::Discover:
            runDiscover( task.entryPoint );
            break;
        case Task::Type::Reload:
            runReload( task.entryPoint );
            break;
        case Task::Type::Remove:
            runRemove( task.entryPoint );
            break;
        case Task::Type::Ban:
            runBan( task.entryPoint );
            break;
        case Task::Type::Unban:
            runUnban( task.entryPoint );
            break;
        default:
            assert(false);
        }
    }
    LOG_INFO( "Exiting DiscovererWorker thread" );
    m_ml->onDiscovererIdleChanged( true );
}

void DiscovererWorker::runReload( const std::string& entryPoint )
{
    for ( auto& d : m_discoverers )
    {
        try
        {
            if ( entryPoint.empty() == true )
            {
                // Let the discoverer invoke the callbacks for all its known folders
                d->reload();
            }
            else
            {
                m_ml->getCb()->onReloadStarted( entryPoint );
                auto res = d->reload( entryPoint );
                m_ml->getCb()->onReloadCompleted( entryPoint, res );
            }
        }
        catch(std::exception& ex)
        {
            LOG_ERROR( "Fatal error while reloading: ", ex.what() );
        }
        if ( m_run == false )
            break;
    }
}

void DiscovererWorker::runRemove( const std::string& ep )
{
    auto entryPoint = utils::file::toFolderPath( ep );
    auto folder = Folder::fromMrl( m_ml, entryPoint );
    if ( folder == nullptr )
    {
        LOG_WARN( "Can't remove unknown entrypoint: ", entryPoint );
        m_ml->getCb()->onEntryPointRemoved( ep, false );
        return;
    }
    // The easy case is that this folder was directly discovered. In which case, we just
    // have to delete it and it won't be discovered again.
    // If it isn't, we also have to ban it to prevent it from reappearing. The Folder::banFolder
    // method already handles the prior deletion
    bool res;
    if ( folder->isRootFolder() == false )
        res = Folder::ban( m_ml, entryPoint );
    else
        res = m_ml->deleteFolder( *folder );
    if ( res == false )
    {
        m_ml->getCb()->onEntryPointRemoved( ep, false );
        return;
    }
    m_ml->getCb()->onEntryPointRemoved( ep, true );
}

void DiscovererWorker::runBan( const std::string& entryPoint )
{
    auto res = Folder::ban( m_ml, entryPoint );
    m_ml->getCb()->onEntryPointBanned( entryPoint, res );
}

void DiscovererWorker::runUnban( const std::string& entryPoint )
{
    auto folder = Folder::bannedFolder( m_ml, entryPoint );
    if ( folder == nullptr )
    {
        LOG_WARN( "Can't unban ", entryPoint, " as it wasn't banned" );
        m_ml->getCb()->onEntryPointUnbanned( entryPoint, false );
        return;
    }
    auto res = m_ml->deleteFolder( *folder );
    m_ml->getCb()->onEntryPointUnbanned( entryPoint, res );

    auto parentPath = utils::file::parentDirectory( entryPoint );
    // If the parent folder was never added to the media library, the discoverer will reject it.
    // We could check it from here, but that would mean fetching the folder twice, which would be a waste.
    runReload( parentPath );
}

void DiscovererWorker::runDiscover( const std::string& entryPoint )
{
    m_ml->getCb()->onDiscoveryStarted( entryPoint );
    auto discovered = false;
    LOG_INFO( "Running discover on: ", entryPoint );
    for ( auto& d : m_discoverers )
    {
        // Assume only one discoverer can handle an entrypoint.
        try
        {
            auto chrono = std::chrono::steady_clock::now();
            if ( d->discover( entryPoint ) == true )
            {
                auto duration = std::chrono::steady_clock::now() - chrono;
                LOG_DEBUG( "Discovered ", entryPoint, " in ",
                           std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
                discovered = true;
                break;
            }
        }
        catch(std::exception& ex)
        {
            LOG_ERROR( "Fatal error while discovering ", entryPoint, ": ", ex.what() );
        }

        if ( m_run == false )
            break;
    }
    if ( discovered == false )
        LOG_WARN( "No IDiscoverer found to discover ", entryPoint );
    m_ml->getCb()->onDiscoveryCompleted( entryPoint, discovered );
}

}
