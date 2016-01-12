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
#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>

#include "Task.h"

class MediaLibrary;
class IParserCb;

class ParserService
{
public:
    ParserService();
    virtual ~ParserService();

    void start();
    void pause();
    void resume();
    void parse( std::unique_ptr<parser::Task> t );
    void initialize( MediaLibrary* mediaLibrary, IParserCb* parserCb );

protected:
    MediaLibrary* mediaLibrary();
    uint8_t nbNativeThreads() const;
    /// Can be overriden to run service dependent initializations
    virtual bool initialize();
    virtual parser::Task::Status run( parser::Task& task ) = 0;
    virtual const char* name() const = 0;
    virtual uint8_t nbThreads() const = 0;

private:
    // Thread(s) entry point
    void mainloop();

private:
    MediaLibrary* m_ml;
    IParserCb* m_parserCb;
    bool m_stopParser;
    bool m_paused;
    std::condition_variable m_cond;
    std::queue<std::unique_ptr<parser::Task>> m_tasks;
    std::vector<std::thread> m_threads;
    std::mutex m_lock;
};
