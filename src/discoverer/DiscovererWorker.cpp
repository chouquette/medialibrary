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
#include "FsDiscoverer.h"

#include <cassert>
#include <algorithm>

namespace medialibrary
{

DiscovererWorker::DiscovererWorker( MediaLibrary* ml,
                                    std::unique_ptr<FsDiscoverer> discoverer )
    : m_currentTask( nullptr )
    , m_run( true )
    , m_taskInterrupted( false )
    , m_discoverer( std::move( discoverer ) )
    , m_ml( ml )
    , m_thread( &DiscovererWorker::run, this )
{
}

DiscovererWorker::~DiscovererWorker()
{
    stop();
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
    auto ep = utils::file::toFolderPath( entryPoint );
    LOG_INFO( "Adding ", ep, " to the folder discovery list" );
    enqueue( ep, Task::Type::AddEntryPoint );
    enqueue( ep, Task::Type::Reload );
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

void DiscovererWorker::addEntryPoint(std::string entryPoint)
{
    enqueue( utils::file::toFolderPath( std::move( entryPoint ) ),
             Task::Type::AddEntryPoint );
}

bool DiscovererWorker::filter( const DiscovererWorker::Task& newTask )
{
    /* This *must* be called with the mutex locked */
    auto filterOut = false;
    switch ( newTask.type )
    {
        case Task::Type::AddEntryPoint:
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
                    if ( (*it).type == Task::Type::AddEntryPoint )
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
             * If we see a task adding or reloading this entry point, we
             * can remove those.
             * If another remove task for the same entrypoint is scheduled, we
             * can filter this one out
             */
            for ( auto it = begin( m_tasks ); it != end( m_tasks ); )
            {
                if ( (*it).entryPoint == newTask.entryPoint )
                {
                    if ( (*it).type == Task::Type::AddEntryPoint ||
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
                    if ( (*it).type == Task::Type::AddEntryPoint ||
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
        case Task::Type::Reload:
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
        case Task::Type::AddEntryPoint:
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
                if ( m_currentTask->type != Task::Type::Reload ||
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
    assert( type != Task::Type::ReloadDevice );

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
    assert( type == Task::Type::ReloadDevice );

    LOG_INFO( "Queuing entity ", entityId, " of type ",
              static_cast<typename std::underlying_type<Task::Type>::type>( type ) );
    enqueue( Task{ entityId, type } );
}

void DiscovererWorker::notify()
{
    m_cond.notify_all();
}

void DiscovererWorker::run()
{
    LOG_INFO( "Entering DiscovererWorker thread" );
    m_ml->onDiscovererIdleChanged( false );
    runReloadAllDevices();
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
                if ( m_tasks.empty() == true )
                {
                    m_ml->onDiscovererIdleChanged( true );
                    m_cond.wait( lock, [this]() {
                        return m_tasks.empty() == false || m_run == false;
                    } );
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
            case Task::Type::AddEntryPoint:
                runAddEntryPoint( task.entryPoint );
                break;
            default:
                assert(false);
            }
            if ( needTaskRefresh == true )
            {
                auto parser = m_ml->tryGetParser();
                if ( parser != nullptr )
                    parser->refreshTaskList();
            }
        }
        ML_UNHANDLED_EXCEPTION_BODY( "DiscovererWorker" )
    }
    LOG_INFO( "Exiting DiscovererWorker thread" );
    m_ml->onDiscovererIdleChanged( true );
}

void DiscovererWorker::runReload( const std::string& entryPoint )
{
    try
    {
        if ( entryPoint.empty() == true )
        {
            // Let the discoverer invoke the callbacks for all its known folders
            m_discoverer->reload( *this );
        }
        else
        {
            m_ml->getCb()->onDiscoveryStarted( entryPoint );
            LOG_INFO( "Reloading folder ", entryPoint );
            auto res = m_discoverer->reload( entryPoint, *this );
            m_ml->getCb()->onDiscoveryCompleted( entryPoint, res );
        }
    }
    catch(std::exception& ex)
    {
        LOG_ERROR( "Fatal error while reloading: ", ex.what() );
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
    auto res = Folder::remove( m_ml, std::move( folder ),
                               Folder::RemovalBehavior::EntrypointRemoved );
    m_ml->getCb()->onEntryPointRemoved( ep, res );
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
    auto res = Folder::remove( m_ml, std::move( folder ),
                                Folder::RemovalBehavior::RemovedFromDisk );
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
    m_ml->startFsFactoriesAndRefresh();

    MediaLibrary::removeOldEntities( m_ml );
}

void DiscovererWorker::runAddEntryPoint( const std::string& entryPoint )
{
    auto res = m_discoverer->addEntryPoint( entryPoint );
    m_ml->getCb()->onEntryPointAdded( entryPoint, res );
}

bool DiscovererWorker::isInterrupted() const
{
    return m_run.load() == false ||
           m_taskInterrupted.load() == true;
}

}
