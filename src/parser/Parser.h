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
#include <vector>
#include "medialibrary/parser/Parser.h"
#include "filesystem/FsHolder.h"

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

class Parser : public IParserCb, public IFsHolderCb
{
public:
    using ServicePtr = std::shared_ptr<IParserService>;

    explicit Parser( MediaLibrary* ml );
    virtual ~Parser();
    void addService( ServicePtr service );
    virtual void parse( std::shared_ptr<Task> task ) override;
    void start();
    void pause();
    void resume();
    void stop();
    ///
    /// \brief prepareRescan Prepares the parser for a forced rescan
    ///
    /// This will pause all workers and flush the tasks list.
    ///
    void prepareRescan();
    ///
    /// \brief rescan Launch a rescan of all processed files.
    ///
    /// This will prepare the worker for restart, fetch all uncompleted tasks and
    /// start them.
    ///
    void rescan();

    virtual void refreshTaskList() override;

    void flush();

private:
    void updateStats();
    virtual void done( std::shared_ptr<Task> task,
                       Status status ) override;
    virtual void onIdleChanged( bool idle ) override;
    // Queues all unparsed files for parsing.
    void restore();
    virtual void onDeviceReappearing( int64_t deviceId ) override;
    virtual void onDeviceDisappearing( int64_t deviceId ) override;

private:
    std::vector<std::unique_ptr<Worker>> m_serviceWorkers;

    MediaLibrary* m_ml;
    IMediaLibraryCb* m_callback;
    std::atomic_uint m_opScheduled;
    std::atomic_uint m_opDone;
};

}

}
