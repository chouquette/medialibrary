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
#include "compat/ConditionVariable.h"
#include <queue>

#include "Task.h"
#include "medialibrary/parser/IParserService.h"
#include "medialibrary/Types.h"
#include "compat/Mutex.h"
#include "compat/Thread.h"
#include "File.h"

namespace medialibrary
{

class ModificationNotifier;
class MediaLibrary;

namespace parser
{
class IParserCb;

class Worker
{
public:
    Worker();

    void pause();
    void resume();
    ///
    /// \brief signalStop Will trigger the threads for termination.
    /// This doesn't wait for the threads to be done, but ensure they won't
    /// queue another operation.
    /// This is usefull to ask all the threads to terminate asynchronously, before
    /// waiting for them to actually stop in the stop() method.
    ///
    void signalStop();
    ///
    /// \brief stop Effectively wait the the underlying threads to join.
    ///
    void stop();
    void parse( std::shared_ptr<Task> t );
    bool initialize( MediaLibrary* ml, IParserCb* parserCb, std::unique_ptr<IParserService> service );
    bool isIdle() const;
    ///
    /// \brief flush flush every currently scheduled tasks
    ///
    /// The service needs to be previously paused or unstarted
    ///
    void flush();

    ///
    /// \brief restart Prepare the parser services for a restart.
    /// This assumes a flush was triggered before
    ///
    void restart();

private:
    // Thread(s) entry point
    void start();
    void mainloop();
    void setIdle( bool isIdle );
    bool handleServiceResult( Task& task, Status status );

private:
    MediaLibrary* m_ml;
    std::unique_ptr<IParserService> m_service;
    IParserCb* m_parserCb;
    bool m_stopParser;
    bool m_paused;
    std::atomic_bool m_idle;
    compat::ConditionVariable m_cond;
    compat::ConditionVariable m_idleCond;
    std::queue<std::shared_ptr<Task>> m_tasks;
    std::vector<compat::Thread> m_threads;
    compat::Mutex m_lock;
};

}
}
