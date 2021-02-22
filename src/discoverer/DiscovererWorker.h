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
#include "discoverer/IDiscoverer.h"
#include "medialibrary/IInterruptProbe.h"

namespace medialibrary
{

class MediaLibrary;

namespace parser
{
    class IParserCb;
}

class DiscovererWorker : public IInterruptProbe
{
protected:
    struct Task
    {
        enum class Type
        {
            Discover,
            Reload,
            Remove,
            Ban,
            Unban,
            ReloadDevice,
        };

        Task() = default;
        Task( const std::string& entryPoint, Type type )
            : entryPoint( entryPoint ), entityId( 0 ), type( type ) {}
        Task( int64_t entityId, Type type )
            : entityId( entityId ), type( type ) {}
        bool isLongRunning() const
        {
            return type == Type::Discover || type == Type::Reload;
        }
        std::string entryPoint;
        int64_t entityId;
        Type type;
    };

    // This is reserved for the DiscovererTests and aims at not creating the
    // background thread when it's not required
    DiscovererWorker() = default;

public:
    DiscovererWorker( MediaLibrary* ml, std::unique_ptr<IDiscoverer> discoverer );
    ~DiscovererWorker();
    void stop();

    bool discover( const std::string& entryPoint );
    void remove( const std::string& entryPoint );
    void reload();
    void reload( const std::string& entryPoint );
    void ban( const std::string& entryPoint );
    void unban( const std::string& entryPoint );
    void reloadDevice( int64_t deviceId );

private:
    void enqueue( Task t );
    void enqueue( const std::string& entryPoint, Task::Type type );
    void enqueue( int64_t entityId, Task::Type type );
    virtual void notify();
    void run();
    void runDiscover( const std::string& entryPoint );
    void runReload( const std::string& entryPoint );
    void runRemove( const std::string& entryPoint );
    void runBan( const std::string& entryPoint );
    void runUnban( const std::string& entryPoint );
    void runReloadDevice( int64_t deviceId );
    void runReloadAllDevices();
    bool filter( const Task& newTask );

private:
    virtual bool isInterrupted() const override;

protected:
    std::list<Task> m_tasks;
    Task* m_currentTask;
    compat::Mutex m_mutex;
    compat::ConditionVariable m_cond;
    // This will be set to false when the worker needs to be stopped
    std::atomic_bool m_run;
    // This will be true when a single task needs to be interrupted
    std::atomic_bool m_taskInterrupted;
    std::unique_ptr<IDiscoverer> m_discoverer;
    MediaLibrary* m_ml;
    compat::Thread m_thread;
};

}
