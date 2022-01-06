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

#include "Parser.h"

#include <utility>
#include <cassert>

#include "MediaLibrary.h"
#include "ParserWorker.h"
#include "parser/Task.h"
#include "logging/Logger.h"

namespace medialibrary
{
namespace parser
{

Parser::Parser( MediaLibrary* ml, FsHolder* fsHolder )
    : m_ml( ml )
    , m_fsHolder( fsHolder )
    , m_callback( nullptr )
    , m_opScheduled( 0 )
    , m_opDone( 0 )
{
}

Parser::~Parser()
{
    stop();
}

void Parser::addService( ServicePtr service )
{
    auto worker = std::unique_ptr<Worker>( new Worker );
    worker->initialize( m_ml, this, std::move( service ) );
    m_serviceWorkers.push_back( std::move( worker ) );
}

void Parser::parse( std::shared_ptr<Task> task )
{
    if ( m_serviceWorkers.empty() == true )
        return;
    assert( task != nullptr );
    m_serviceWorkers[0]->parse( std::move( task ) );
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    m_opScheduled += 1;
    updateStats();
}

void Parser::start()
{
    {
        std::lock_guard<compat::Mutex> lock{ m_mutex };
        assert( m_callback == nullptr );
        m_callback = m_ml->getCb();
    }
    m_fsHolder->registerCallback( this );
    assert( m_serviceWorkers.size() == 3 );
    restore();
}

void Parser::pause()
{
    for ( auto& s : m_serviceWorkers )
        s->pause();
}

void Parser::resume()
{
    for ( auto& s : m_serviceWorkers )
        s->resume();
}

void Parser::stop()
{
    {
        std::lock_guard<compat::Mutex> lock{ m_mutex };
        if ( m_callback == nullptr )
            return;
        m_callback = nullptr;
    }
    m_fsHolder->unregisterCallback( this );

    for ( auto& s : m_serviceWorkers )
    {
        s->signalStop();
    }
    for ( auto& s : m_serviceWorkers )
    {
        s->stop();
    }
}

void Parser::flush()
{
    for ( auto& s : m_serviceWorkers )
        s->flush();
    /*
     * The services are now paused so we are ensured we won't have a concurrent
     * update for the task counters
     */
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    m_opDone = 0;
    m_opScheduled = 0;
}

void Parser::rescan()
{
    for ( auto& s : m_serviceWorkers )
        s->restart();
    restore();
    resume();
}

void Parser::restore()
{
    if ( m_serviceWorkers.empty() == true )
        return;
    auto tasks = Task::fetchUncompleted( m_ml );
    if ( tasks.empty() == true )
    {
        LOG_DEBUG( "No task to resume." );
        return;
    }
    LOG_INFO( "Resuming parsing on ", tasks.size(), " tasks" );
    {
        std::lock_guard<compat::Mutex> lock{ m_mutex };
        m_opScheduled = tasks.size();
        updateStats();
    }
    m_serviceWorkers[0]->parse( std::move( tasks ) );
}

void Parser::onDeviceReappearing( int64_t )
{
    refreshTaskList();
}

void Parser::onDeviceDisappearing( int64_t )
{
    /*
     * If a device went away, let's ensure we're not still trying to
     * analyze its content.
     * By the time this callback is called, the database has been updated so
     * the tasks associated with it will be filtered out due to `is_present = 0`
     */
    refreshTaskList();
}

void Parser::refreshTaskList()
{
    /*
     * Ideally we should assert that the callback is != nullptr here but we can't
     * as things stand:
     * - Checking the callback means locking the parser mutex
     * - While in this scope we're being called from the FsHolder with its mutex held
     * - While stopping, we can't lock the parser mutex then unregister the fs holder
     *   callback as it would create a lock inversion
     * - Not unregistering as part of the parser lock means there's a small window
     *   in which the callback is nullptr but the FsHolder still has its callback
     *   registered
     */
    flush();
    restore();
}

void Parser::updateStats()
{
    if ( m_callback == nullptr )
        return;
    assert( m_opScheduled >= m_opDone );
    if ( m_opDone % 10 == 0 || m_opScheduled == m_opDone )
    {
        LOG_DEBUG( "Updating progress: operations scheduled ", m_opScheduled,
                   "; operations done: ", m_opDone );
        m_callback->onParsingStatsUpdated( m_opDone, m_opScheduled );
    }
}

void Parser::done( std::shared_ptr<Task> t, Status status )
{
    auto serviceIdx = t->goToNextService();

    if ( status == Status::TemporaryUnavailable ||
         status == Status::Fatal ||
         status == Status::Discarded ||
         t->isCompleted() )
    {
        {
            std::lock_guard<compat::Mutex> lock{ m_mutex };
            ++m_opDone;
            updateStats();
        }
        // We create a separate task for refresh, which doesn't count toward
        // (mrl,parent_playlist) uniqueness. In order to allow for a subsequent
        // refresh of the same file, we remove it once the refresh is complete.
        // In case the status was `Discarded`, the task was already deleted from
        // the database.
        if ( t->isRefresh() == true )
            Task::destroy( m_ml, t->id() );
        return;
    }
    if ( status == Status::Requeue )
    {
        // The retry_count is mostly handled when fetching the remaining tasks
        // from the database. However, when requeuing, it all happens at runtime
        // in the C++ code, so we also need to ensure we're not requeuing tasks
        // forever.
        if ( t->attemptsRemaining() == 0 )
        {
            std::lock_guard<compat::Mutex> lock{ m_mutex };
            ++m_opDone;
            updateStats();
            return;
        }
        t->resetCurrentService();
        serviceIdx = 0;
    }

    // If some services declined to parse the file, start over again.
    assert( serviceIdx < m_serviceWorkers.size() );
    m_serviceWorkers[serviceIdx]->parse( std::move( t ) );
}

void Parser::onIdleChanged( bool idle ) const
{
    // If any parser service is not idle, then the global parser state is active
    if ( idle == false )
    {
        m_ml->onParserIdleChanged( false );
        return;
    }
    // Otherwise the parser is idle when all services are idle
    for ( const auto& s : m_serviceWorkers )
    {
        // We're switching a service from "not idle" to "idle" here, so as far as the medialibrary
        // is concerned the parser is still "not idle". In case a single parser service isn't
        // idle, no need to trigger a change to the medialibrary.
        if ( s->isIdle() == false )
            return;
    }
    m_ml->onParserIdleChanged( true );
}

}
}
