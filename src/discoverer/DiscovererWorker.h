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

#pragma once

#include <atomic>
#include "compat/ConditionVariable.h"
#include <memory>
#include <list>
#include <string>
#include <vector>

#include "compat/Mutex.h"
#include "compat/Thread.h"
#include "FsDiscoverer.h"
#include "filesystem/FsHolder.h"

namespace medialibrary
{

class MediaLibrary;
class FsHolder;

class DiscovererWorker : public IFsHolderCb
{
protected:
    struct Task
    {
        enum class Type
        {
            Reload,
            Remove,
            Ban,
            Unban,
            ReloadDevice,
            AddRoot,
        };

        Task() = default;
        Task( const std::string& root, Type type )
            : root( root ), entityId( 0 ), type( type ) {}
        Task( int64_t entityId, Type type )
            : entityId( entityId ), type( type ) {}
        bool isLongRunning() const
        {
            return type == Type::Reload;
        }
        std::string root;
        int64_t entityId;
        Type type;
    };

    // This is reserved for the DiscovererTests and aims at not creating the
    // background thread when it's not required
    DiscovererWorker() = default;

public:
    DiscovererWorker( MediaLibrary* ml, FsHolder* fsHolder );
    virtual ~DiscovererWorker();
    void stop();
    void pause();
    void resume();

    bool discover( const std::string& root );
    void remove( const std::string& root );
    void reload();
    void reload( const std::string& root );
    void ban( const std::string& root );
    void unban( const std::string& root );
    void reloadDevice( int64_t deviceId );

private:
    void enqueue( Task t );
    void enqueue( const std::string& root, Task::Type type );
    void enqueue( int64_t entityId, Task::Type type );
    virtual void notify();
    void run();
    void runReload( const std::string& root );
    void runRemove( const std::string& root );
    void runBan( const std::string& root );
    void runUnban( const std::string& root );
    void runReloadDevice( int64_t deviceId );
    void runReloadAllDevices();
    void runAddRoot( const std::string& root );
    bool filter( const Task& newTask );
    virtual void onDeviceReappearing( int64_t deviceId ) override;
    virtual void onDeviceDisappearing( int64_t deviceId ) override;

protected:
    FsHolder* m_fsHolder;
    std::list<Task> m_tasks;
    Task* m_currentTask;
    compat::Mutex m_mutex;
    compat::ConditionVariable m_cond;
    // This will be set to false when the worker needs to be stopped
    bool m_run;
    std::unique_ptr<FsDiscoverer> m_discoverer;
    MediaLibrary* m_ml;
    compat::Thread m_thread;
    bool m_discoveryNotified;

    friend std::ostream& operator<<( std::ostream& s, DiscovererWorker::Task::Type& t );
};

std::ostream& operator<<( std::ostream& s, DiscovererWorker::Task::Type& t );


}
