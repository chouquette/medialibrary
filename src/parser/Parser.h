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

#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include "medialibrary/parser/Parser.h"

namespace medialibrary
{

class MediaLibrary;
class IMediaLibraryCb;

namespace parser
{

class IParserService;
class Worker;
class Task;

// Use an interface to expose only the "done" method
class IParserCb
{
public:
    virtual ~IParserCb() = default;
    virtual void parse( std::shared_ptr<Task> task ) = 0;
    virtual void done( std::shared_ptr<Task> task, Status status ) = 0;
    virtual void onIdleChanged( bool isIdle ) = 0;
    /**
     * @brief refreshTaskList Will be invoked when the task list should be refreshed
     *
     * This is likely to happen when the discoverer is instructed to remove/ban
     * some folders, in which case we need to stop
     */
    virtual void refreshTaskList() = 0;
};

class Parser : public IParserCb
{
public:
    using ServicePtr = std::shared_ptr<IParserService>;

    Parser( MediaLibrary* ml );
    virtual ~Parser();
    void addService( ServicePtr service );
    virtual void parse( std::shared_ptr<Task> task ) override;
    void start();
    void pause();
    void resume();
    void stop();
    void flush();
    ///
    /// \brief restart Will instruct the parser services to prepare for restarting
    ///                after a flush operation. This is different from resume
    ///                as it can imply initialization side effects
    ///
    void restart();
    // Queues all unparsed files for parsing.
    void restore();

    virtual void refreshTaskList() override;

private:
    void updateStats();
    virtual void done( std::shared_ptr<Task> task,
                       Status status ) override;
    virtual void onIdleChanged( bool idle ) override;

private:
    std::vector<std::unique_ptr<Worker>> m_services;

    MediaLibrary* m_ml;
    IMediaLibraryCb* m_callback;
    std::atomic_uint m_opToDo;
    std::atomic_uint m_opDone;
    std::atomic_uint m_percent;
    std::chrono::time_point<std::chrono::steady_clock> m_chrono;
};

}

}
