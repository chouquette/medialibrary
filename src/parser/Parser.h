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

#include <memory>
#include <queue>

#include "File.h"
#include "Task.h"

namespace medialibrary
{

class ParserService;

// Use an interface to expose only the "done" method
class IParserCb
{
public:
    virtual ~IParserCb() = default;
    virtual void done( std::unique_ptr<parser::Task> task, parser::Task::Status status ) = 0;
    virtual void onIdleChanged( bool isIdle ) = 0;
};

class Parser : IParserCb
{
public:
    using ServicePtr = std::unique_ptr<ParserService>;

    Parser( MediaLibrary* ml );
    virtual ~Parser();
    void addService( ServicePtr service );
    void parse( std::shared_ptr<Media> media, std::shared_ptr<File> file );
    void start();
    void pause();
    void resume();
    void stop();

private:
    // Queues all unparsed files for parsing.
    void restore();
    void updateStats();
    virtual void done( std::unique_ptr<parser::Task> task, parser::Task::Status status ) override;
    virtual void onIdleChanged( bool idle ) override;

private:
    typedef std::vector<ServicePtr> ServiceList;

private:
    ServiceList m_services;

    MediaLibrary* m_ml;
    IMediaLibraryCb* m_callback;
    std::atomic_uint m_opToDo;
    std::atomic_uint m_opDone;
    std::atomic_uint m_percent;
    std::chrono::time_point<std::chrono::steady_clock> m_chrono;
};

}
