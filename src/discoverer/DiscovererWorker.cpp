/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "DiscovererWorker.h"

#include "logging/Logger.h"
#include "Folder.h"
#include "MediaLibrary.h"
#include "Device.h"
#include "Media.h"
#include "utils/Filename.h"
#include "medialibrary/filesystem/Errors.h"
#include "utils/Defer.h"
#include "parser/Parser.h"

#include <cassert>
#include <algorithm>

namespace medialibrary
{

DiscovererWorker::DiscovererWorker( MediaLibrary* ml, parser::IParserCb* parserCb )
    : m_currentTask( nullptr )
    , m_run( false )
    , m_taskInterrupted( false )
    , m_ml( ml )
    , m_parserCb( parserCb )
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
            m_tasks.clear();
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

void DiscovererWorker::reloadDevice(int64_t deviceId)
{
    enqueue( deviceId, Task::Type::ReloadDevice );
}

void DiscovererWorker::reloadAllDevices()
{
    enqueue( 0, Task::Type::ReloadAllDevices );
}

bool DiscovererWorker::filter( const DiscovererWorker::Task& newTask )
{
    /* This *must* be called with the mutex locked */
    auto filterOut = false;
    switch ( newTask.type )
    {
        case Task::Type::Discover:
        {
            /*
             * We are required to discover an entry point.
             * If another discover task is queued for this entry point, we can
             * simply ignore it.
             * If we encounter a task which aims at removing this
             * entry point, we can delete it from the task list since this is its
             * inverse operation.
             * If we see a reload task for this entry point, we can also remove it
             * since the discover will also issue a reload.
             */
            for ( auto it = begin( m_tasks ); it != end( m_tasks ); )
            {
                if ( (*it).entryPoint == newTask.entryPoint )
                {
                    if ( (*it).type == Task::Type::Remove ||
                         (*it).type == Task::Type::Reload )
                    {
                        it = m_tasks.erase( it );
                        continue;
                    }
                    if ( (*it).type == Task::Type::Discover )
                        return true;
                }
                ++it;
            }
            break;
        }
        case Task::Type::Reload:
        {
            /*
             * We need to reload an entry point.
             * If another task reloading the same entry point is already scheduled
             * we can ignore it.
             * If another task banning/removing this entry point is scheduled,
             * we can ignore it as well.
             */
            for ( const auto& t : m_tasks )
            {
                if ( t.entryPoint == newTask.entryPoint &&
                     ( t.type == Task::Type::Reload ||
                       t.type == Task::Type::Remove ||
                       t.type == Task::Type::Ban ) )
                    return true;
            }
            break;
        }
        case Task::Type::Remove:
        {
            /*
             * We are about to remove an entry point.
             * If we see a task discovering or reloading this entry point, we
             * can remove those.
             * If another remove task for the same entrypoint is scheduled, we
             * can filter this one out
             */
            for ( auto it = begin( m_tasks ); it != end( m_tasks ); )
            {
                if ( (*it).entryPoint == newTask.entryPoint )
                {
                    if ( (*it).type == Task::Type::Discover ||
                         (*it).type == Task::Type::Reload )
                    {
                        it = m_tasks.erase( it );
                        filterOut = true;
                        continue;
                    }
                    if ( (*it).type == Task::Type::Remove )
                        return true;
                }
                ++it;
            }
            break;
        }
        case Task::Type::Ban:
        {
            /*
             * We are about to ban an entry point.
             * If it's scheduled to be discovered/reload/unbanned, we can remove
             * those tasks.
             * If an identical ban request is to be found, we can discard the new
             * one.
             */
            for ( auto it = begin( m_tasks ); it != end( m_tasks ); )
            {
                if ( (*it).entryPoint == newTask.entryPoint )
                {
                    if ( (*it).type == Task::Type::Discover ||
                         (*it).type == Task::Type::Reload ||
                         (*it).type == Task::Type::Unban )
                    {
                        it = m_tasks.erase( it );
                        continue;
                    }
                    if ( (*it).type == Task::Type::Ban )
                        return true;
                }
                ++it;
            }
            break;
        }
        case Task::Type::Unban:
        {
            /*
             * We are about to unban an entry point.
             * If we find a queue request for banning this folder, we can remove
             * it and not queue this request
             */
            for ( auto it = begin( m_tasks ); it != end( m_tasks ); ++it )
            {
                if ( (*it).entryPoint == newTask.entryPoint &&
                     (*it).type == Task::Type::Ban )
                {
                    m_tasks.erase( it );
                    return true;
                }
            }
            break;
        }
        case Task::Type::ReloadDevice:
        {
            for ( const auto& t : m_tasks )
            {
                if ( t.type == newTask.type && t.entityId == newTask.entityId )
                    return true;
            }
            break;
        }
        case Task::Type::ReloadAllDevices:
            break;
    }
    return filterOut;
}

void DiscovererWorker::enqueue( DiscovererWorker::Task t )
{
    std::unique_lock<compat::Mutex> lock( m_mutex );

    if ( filter( t ) == true )
        return;

    switch ( t.type )
    {
        case Task::Type::Discover:
        case Task::Type::Reload:
        case Task::Type::ReloadAllDevices:
            /*
             * These task types may just be queued after any currently
             * running tasks, no need to process them right now.
             */
            m_tasks.emplace_back( std::move( t ) );
            break;
        case Task::Type::Remove:
        case Task::Type::Ban:
        case Task::Type::Unban:
        case Task::Type::ReloadDevice:
        {
            /*
             * These types need to be processed as soon as possible, meaning we
             * will:
             * - Interrupt the currently running task if it's a long running one
             * - queue the new task before the previously running one
             * - start the previous task again
             */
            auto it = std::find_if( begin( m_tasks ), end( m_tasks ),
                                    []( const Task& t ) {
                return t.isLongRunning();
            });
            if ( m_currentTask != nullptr && m_currentTask->isLongRunning() )
            {
                /*
                 * We only care about interrupting long tasks, which are
                 * discover & reload tasks. Others are just a few requests and
                 * some bookkeeping, which we can wait for.
                 * We requeue a reload task for the currently processed
                 * entrypoint instead of requeuing the same task type, since we
                 * could end up requeuing a discovery task, which would
                 * effectively cancel any potential remove operation we might
                 * be queuing.
                 */
                m_taskInterrupted = true;
                /*
                 * If we are interrupting a discover or reload operation with a
                 * ban/remove operation on the same mountpoint, we might as well
                 * not reload this folder since it won't be found afterward
                 */
                if ( ( m_currentTask->type != Task::Type::Discover &&
                       m_currentTask->type != Task::Type::Reload ) ||
                     ( t.type != Task::Type::Ban &&
                       t.type != Task::Type::Remove ) ||
                     t.entryPoint != m_currentTask->entryPoint )
                {
                    it = m_tasks.emplace( it, m_currentTask->entryPoint,
                                          Task::Type::Reload );
                }
            }
            m_tasks.insert( it, std::move( t ) );
            break;
        }
        default:
            assert( !"Unexpected task type" );
    }

    notify();
}

void DiscovererWorker::enqueue( const std::string& entryPoint, Task::Type type )
{
    assert( type != Task::Type::ReloadDevice &&
            type != Task::Type::ReloadAllDevices );

    if ( entryPoint.empty() == false )
    {
        LOG_INFO( "Queuing entrypoint ", entryPoint, " of type ",
                  static_cast<typename std::underlying_type<Task::Type>::type>( type ) );
    }
    else
    {
        assert( type == Task::Type::Reload );
        LOG_INFO( "Queuing global reload request" );
    }
    enqueue( Task{ entryPoint, type } );
}

void DiscovererWorker::enqueue( int64_t entityId, Task::Type type )
{
    assert( type == Task::Type::ReloadDevice ||
            type == Task::Type::ReloadAllDevices );

    LOG_INFO( "Queuing entity ", entityId, " of type ",
              static_cast<typename std::underlying_type<Task::Type>::type>( type ) );
    enqueue( Task{ entityId, type } );
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
        ML_UNHANDLED_EXCEPTION_INIT
        {
            Task task;
            auto d = utils::make_defer( [this](){
                std::lock_guard<compat::Mutex> lock( m_mutex );
                m_taskInterrupted = false;
                m_currentTask = nullptr;
            });
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
                m_tasks.pop_front();
                m_currentTask = &task;
            }
            auto needTaskRefresh = false;
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
                needTaskRefresh = true;
                break;
            case Task::Type::Ban:
                runBan( task.entryPoint );
                needTaskRefresh = true;
                break;
            case Task::Type::Unban:
                runUnban( task.entryPoint );
                /*
                 * No need to refresh the parser tasks for this case, as this
                 * will only add new one which can be done on the fly
                 */
                break;
            case Task::Type::ReloadDevice:
                runReloadDevice( task.entityId );
                needTaskRefresh = true;
                break;
            case Task::Type::ReloadAllDevices:
                /*
                 * This is a special case which is run only during media library
                 * startup, we don't need to refresh the tasks list for this
                 * as the task list hasn't been fetched from database yet
                 */
                runReloadAllDevices();
                break;
            default:
                assert(false);
            }
            if ( needTaskRefresh == true && m_parserCb != nullptr )
                m_parserCb->refreshTaskList();
        }
        ML_UNHANDLED_EXCEPTION_BODY( "DiscovererWorker" )
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
                d->reload( *this );
            }
            else
            {
                m_ml->getCb()->onReloadStarted( entryPoint );
                LOG_INFO( "Reloading folder ", entryPoint );
                auto res = d->reload( entryPoint, *this );
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
    /*
     * If the parent folder was never added to the media library,
     * the discoverer will reject it.
     * We could check it from here, but that would mean fetching the folder
     * twice, which would be a waste.
     *
     * Re-enqueue the task to avoid blocking for too long while executing an
     * unban task.
     */
    enqueue( parentPath, Task::Type::Reload );
}

void DiscovererWorker::runReloadDevice( int64_t deviceId )
{
    auto device = Device::fetch( m_ml, deviceId );
    if ( device == nullptr )
    {
        LOG_ERROR( "Can't fetch device ", deviceId, " to reload it" );
        return;
    }
    auto entryPoints = Folder::entryPoints( m_ml, false, device->id() );
    if ( entryPoints == nullptr )
        return;
    for ( const auto& ep : entryPoints->all() )
    {
        try
        {
            auto mrl = ep->mrl();
            LOG_INFO( "Reloading entrypoint on mounted device: ", mrl );
            runReload( mrl );
        }
        catch ( const fs::errors::DeviceRemoved& )
        {
            LOG_INFO( "Can't reload device ", device->uuid(), " as it was removed" );
        }
    }
}

void DiscovererWorker::runReloadAllDevices()
{
    m_ml->refreshDevices();

    MediaLibrary::removeOldEntities( m_ml );
}

bool DiscovererWorker::isInterrupted() const
{
    return m_run.load() == false ||
           m_taskInterrupted.load() == true;
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
            if ( d->discover( entryPoint, *this ) == true )
            {
                auto duration = std::chrono::steady_clock::now() - chrono;
                LOG_VERBOSE( "Discovered ", entryPoint, " in ",
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
