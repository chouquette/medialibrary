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

DiscovererWorker::DiscovererWorker( MediaLibrary* ml, FsHolder* fsHolder )
    : m_fsHolder( fsHolder )
    , m_currentTask( nullptr )
    , m_run( false )
    , m_ml( ml )
    , m_discoveryNotified( false )
{
    /*
     * We can't use a reference as we need a default constructor to be valid for
     * the unit tests constructor
     */
    assert( fsHolder != nullptr );
}

DiscovererWorker::~DiscovererWorker()
{
    stop();
}

void DiscovererWorker::stop()
{
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        if ( m_run == false )
            return;
        m_run = false;
        /* Interrupt the current long running task if any */
        m_discoverer->interrupt();
    }

    /* Wake the thread in case it was waiting for more things to do */
    m_cond.notify_all();

    m_fsHolder->stopNetworkFsFactories();

    /* And wait for all short requests to be handled */
    m_thread.join();
}

void DiscovererWorker::pause()
{
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        if ( m_run == false )
            return;
    }
    m_discoverer->pause();
}

void DiscovererWorker::resume()
{
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        if ( m_run == false )
            return;
    }
    m_discoverer->resume();
}

bool DiscovererWorker::discover( const std::string& root )
{
    if ( root.length() == 0 )
        return false;
    auto path = utils::file::toFolderPath( root );
    LOG_INFO( "Adding ", path, " to the folder discovery list" );
    enqueue( root, Task::Type::AddRoot );
    enqueue( root, Task::Type::Reload );
    return true;
}

void DiscovererWorker::remove( const std::string& root )
{
    enqueue( root, Task::Type::Remove );
}

void DiscovererWorker::reload()
{
    enqueue( "", Task::Type::Reload );
}

void DiscovererWorker::reload( const std::string& root )
{
    enqueue( utils::file::toFolderPath( root ), Task::Type::Reload );
}

void DiscovererWorker::ban( const std::string& root )
{
    enqueue( utils::file::toFolderPath( root ), Task::Type::Ban );
}

void DiscovererWorker::unban( const std::string& root )
{
    enqueue( utils::file::toFolderPath( root ), Task::Type::Unban );
}

void DiscovererWorker::reloadDevice(int64_t deviceId)
{
    enqueue( deviceId, Task::Type::ReloadDevice );
}

bool DiscovererWorker::filter( const DiscovererWorker::Task& newTask )
{
    /* This *must* be called with the mutex locked */
    auto filterOut = false;
    switch ( newTask.type )
    {
        case Task::Type::AddRoot:
        {
            /*
             * We are required to discover a root folder.
             * If another discover task is queued for this root folder, we can
             * simply ignore it.
             * If we encounter a task which aims at removing this
             * root folder, we can delete it from the task list since this is its
             * inverse operation.
             * If we see a reload task for this root folder, we can also remove it
             * since the discover will also issue a reload.
             */
            for ( auto it = begin( m_tasks ); it != end( m_tasks ); )
            {
                if ( (*it).root == newTask.root )
                {
                    if ( (*it).type == Task::Type::Remove ||
                         (*it).type == Task::Type::Reload )
                    {
                        it = m_tasks.erase( it );
                        continue;
                    }
                    if ( (*it).type == Task::Type::AddRoot )
                        return true;
                }
                ++it;
            }
            break;
        }
        case Task::Type::Reload:
        {
            /*
             * We need to reload a root folder.
             * If another task reloading the same root folder is already scheduled
             * we can ignore it.
             * If another task banning/removing this root folder is scheduled,
             * we can ignore it as well.
             */
            for ( const auto& t : m_tasks )
            {
                if ( t.root == newTask.root &&
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
             * We are about to remove a root folder.
             * If we see a task adding or reloading this root folder, we
             * can remove those.
             * If another remove task for the same root folder is scheduled, we
             * can filter this one out
             */
            for ( auto it = begin( m_tasks ); it != end( m_tasks ); )
            {
                if ( (*it).root == newTask.root )
                {
                    if ( (*it).type == Task::Type::AddRoot ||
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
             * We are about to ban a root folder.
             * If it's scheduled to be discovered/reload/unbanned, we can remove
             * those tasks.
             * If an identical ban request is to be found, we can discard the new
             * one.
             */
            for ( auto it = begin( m_tasks ); it != end( m_tasks ); )
            {
                if ( (*it).root == newTask.root )
                {
                    if ( (*it).type == Task::Type::AddRoot ||
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
             * We are about to unban a root folder.
             * If we find a queue request for banning this folder, we can remove
             * it and not queue this request
             */
            for ( auto it = begin( m_tasks ); it != end( m_tasks ); ++it )
            {
                if ( (*it).root == newTask.root &&
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

void DiscovererWorker::onDeviceReappearing( int64_t deviceId )
{
    reloadDevice( deviceId );
}

void DiscovererWorker::onDeviceDisappearing( int64_t )
{
    /*
     * The FsDiscoverer will abort the discovery itself if the device can't be
     * read from anymore
     */
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
        case Task::Type::AddRoot:
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
                 * root folder instead of requeuing the same task type, since we
                 * could end up requeuing a discovery task, which would
                 * effectively cancel any potential remove operation we might
                 * be queuing.
                 */
                if ( m_discoverer != nullptr )
                {
                    LOG_DEBUG( "Interrupting long running discovery task on "
                        "root folder ", m_currentTask->root );
                    m_discoverer->interrupt();
                }
                /*
                 * If we are interrupting a discover or reload operation with a
                 * ban/remove operation on the same mountpoint, we might as well
                 * not reload this folder since it won't be found afterward
                 */
                if ( m_currentTask->type != Task::Type::Reload ||
                     ( t.type != Task::Type::Ban &&
                       t.type != Task::Type::Remove ) ||
                     t.root != m_currentTask->root )
                {
                    LOG_DEBUG( "Requeuing reload task of root folder ",
                               m_currentTask->root );
                    it = m_tasks.emplace( it, m_currentTask->root,
                                          Task::Type::Reload );
                }
            }
            m_tasks.insert( it, std::move( t ) );
            break;
        }
        default:
            assert( !"Unexpected task type" );
    }
    if ( m_thread.joinable() == false )
    {
        /*
         * This check is only relevant in discoverer tests configuration.
         * Otherwise we always have a valid fsHolder instance
         */
        if ( m_fsHolder != nullptr )
        {
            m_discoverer = std::make_unique<FsDiscoverer>( m_ml, *m_fsHolder, m_ml->getCb() );
            m_thread = compat::Thread{ &DiscovererWorker::run, this };
            m_run = true;
        }
    }
    else
        notify();
}

void DiscovererWorker::enqueue( const std::string& root, Task::Type type )
{
    assert( type != Task::Type::ReloadDevice );

    if ( root.empty() == false )
    {
        LOG_INFO( "Queuing root folder ", root, " of type ", type );
    }
    else
    {
        assert( type == Task::Type::Reload );
        LOG_INFO( "Queuing global reload request" );
    }
    enqueue( Task{ root, type } );
}

void DiscovererWorker::enqueue( int64_t entityId, Task::Type type )
{
    assert( type == Task::Type::ReloadDevice );

    LOG_INFO( "Queuing entity ", entityId, " of type ", type );
    enqueue( Task{ entityId, type } );
}

void DiscovererWorker::notify()
{
    m_cond.notify_all();
}

void DiscovererWorker::run()
{
    LOG_INFO( "Entering DiscovererWorker thread" );
    m_fsHolder->registerCallback( this );
    m_ml->onDiscovererIdleChanged( false );
    runReloadAllDevices();

    auto needTaskRefresh = false;

    while ( true )
    {
        ML_UNHANDLED_EXCEPTION_INIT
        {
            Task task;
            auto d = utils::make_defer( [this](){
                std::lock_guard<compat::Mutex> lock( m_mutex );
                m_currentTask = nullptr;
            });
            {
                std::unique_lock<compat::Mutex> lock( m_mutex );
                if ( m_tasks.empty() == true )
                {
                    if ( m_discoveryNotified == true )
                    {
                        m_ml->getCb()->onDiscoveryCompleted();
                        m_discoveryNotified = false;
                    }
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
                /*
                 * If we are stopping, we only want to execute the short tasks
                 * to update the database to reflect the user's actions.
                 * The actual reload will be run next time.
                 */
                if ( task.isLongRunning() == true && m_run == false )
                    continue;
                m_currentTask = &task;
            }
            /*
             * In the event we have multiple ban/remove tasks in a row, those
             * would trigger multiple task refresh, which is a waste.
             * Instead, we postpone the task refresh to right before the next
             * long running task, once the task list is expected to be stable
             */
            if ( needTaskRefresh == true && task.isLongRunning() == true )
            {
                {
                    std::unique_lock<compat::Mutex> lock( m_mutex );
                    if ( m_run == false )
                        continue;
                }
                auto parser = m_ml->getParser();
                if ( parser != nullptr )
                    parser->refreshTaskList();
                needTaskRefresh = false;
            }
            LOG_DEBUG( "Running task of type ", task.type );
            switch ( task.type )
            {
            case Task::Type::Reload:
                if ( m_discoveryNotified == false )
                {
                    m_ml->getCb()->onDiscoveryStarted();
                    m_discoveryNotified = true;
                }
                runReload( task.root );
                break;
            case Task::Type::Remove:
                runRemove( task.root );
                needTaskRefresh = true;
                break;
            case Task::Type::Ban:
                runBan( task.root );
                needTaskRefresh = true;
                break;
            case Task::Type::Unban:
                runUnban( task.root );
                /*
                 * No need to refresh the parser tasks for this case, as this
                 * will only add new one which can be done on the fly
                 */
                break;
            case Task::Type::ReloadDevice:
                runReloadDevice( task.entityId );
                needTaskRefresh = true;
                break;
            case Task::Type::AddRoot:
                runAddRoot( task.root );
                break;
            default:
                assert(false);
            }
        }
        ML_UNHANDLED_EXCEPTION_BODY( "DiscovererWorker" )
    }
    LOG_INFO( "Exiting DiscovererWorker thread" );
    m_ml->onDiscovererIdleChanged( true );
    m_fsHolder->unregisterCallback( this );
}

void DiscovererWorker::runReload( const std::string& root )
{
    try
    {
        if ( root.empty() == true )
        {
            // Let the discoverer invoke the callbacks for all its known folders
            m_discoverer->reload();
        }
        else
        {
            LOG_INFO( "Reloading folder ", root );
            auto res = m_discoverer->reload( root );
            if ( res == false )
                m_ml->getCb()->onDiscoveryFailed( root );
        }
    }
    catch(std::exception& ex)
    {
        LOG_ERROR( "Fatal error while reloading: ", ex.what() );
    }
}

void DiscovererWorker::runRemove( const std::string& path )
{
    auto root = utils::file::toFolderPath( path );
    auto folder = Folder::fromMrl( m_ml, root );
    if ( folder == nullptr )
    {
        LOG_WARN( "Can't remove unknown root folder: ", root );
        m_ml->getCb()->onRootRemoved( path, false );
        return;
    }
    auto res = Folder::remove( m_ml, std::move( folder ),
                               Folder::RemovalBehavior::RootRemoved );
    m_ml->getCb()->onRootRemoved( path, res );
}

void DiscovererWorker::runBan( const std::string& root )
{
    auto res = Folder::ban( m_ml, root );
    m_ml->getCb()->onRootBanned( root, res );
}

void DiscovererWorker::runUnban( const std::string& root )
{
    auto folder = Folder::bannedFolder( m_ml, root );
    if ( folder == nullptr )
    {
        LOG_WARN( "Can't unban ", root, " as it wasn't banned" );
        m_ml->getCb()->onRootUnbanned( root, false );
        return;
    }
    auto res = Folder::remove( m_ml, std::move( folder ),
                                Folder::RemovalBehavior::RemovedFromDisk );
    m_ml->getCb()->onRootUnbanned( root, res );

    auto parentPath = utils::file::parentDirectory( root );
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
    auto roots = Folder::roots( m_ml, false, device->id(), nullptr );
    if ( roots == nullptr )
        return;
    for ( const auto& root : roots->all() )
    {
        try
        {
            auto mrl = root->mrl();
            LOG_INFO( "Reloading root folder on mounted device: ", mrl );
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
    m_fsHolder->startFsFactoriesAndRefresh();

    MediaLibrary::removeOldEntities( m_ml );
}

void DiscovererWorker::runAddRoot( const std::string& root )
{
    auto res = m_discoverer->addRoot( root );
    m_ml->getCb()->onRootAdded( root, res );
}

std::ostream& operator<<( std::ostream& s, DiscovererWorker::Task::Type& t )
{
    switch ( t )
    {
        case DiscovererWorker::Task::Type::Reload:
            s << "Reload";
            break;
        case DiscovererWorker::Task::Type::Remove:
            s << "Remove";
            break;
        case DiscovererWorker::Task::Type::Ban:
            s << "Ban";
            break;
        case DiscovererWorker::Task::Type::Unban:
            s << "Unban";
            break;
        case DiscovererWorker::Task::Type::ReloadDevice:
            s << "ReloadDevice";
            break;
        case DiscovererWorker::Task::Type::AddRoot:
            s << "AddRoot";
            break;
        default:
            assert( !"Invalid task type" );
            break;
    }
    return s;
}

}
