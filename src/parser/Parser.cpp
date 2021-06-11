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

Parser::Parser( MediaLibrary* ml )
    : m_ml( ml )
    , m_callback( ml->getCb() )
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
    if ( worker->initialize( m_ml, this, std::move( service ) ) == false )
        return;
    m_serviceWorkers.push_back( std::move( worker ) );
}

void Parser::parse( std::shared_ptr<Task> task )
{
    if ( m_serviceWorkers.empty() == true )
        return;
    assert( task != nullptr );
    m_serviceWorkers[0]->parse( std::move( task ) );
    m_opScheduled.fetch_add( 1, std::memory_order_relaxed );
    updateStats();
}

void Parser::start()
{
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
    m_opScheduled.store( 0, std::memory_order_relaxed );
    m_opDone.store( 0, std::memory_order_relaxed );
}

void Parser::prepareRescan()
{
    pause();
    flush();
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
    m_opScheduled.fetch_add( tasks.size(), std::memory_order_relaxed );
    updateStats();
    m_serviceWorkers[0]->parse( std::move( tasks ) );
}

void Parser::refreshTaskList()
{
    /*
     * We need to do this in various steps:
     * - Pausing the workers after their currently running task
     * - Flushing their task list
     * - Restoring the task list from DB
     * - Resuming the workers
     */
    pause();
    flush();
    restore();
    resume();
}

void Parser::updateStats()
{
    auto opScheduled = m_opScheduled.load( std::memory_order_relaxed );
    auto opDone = m_opDone.load( std::memory_order_relaxed );

    assert( opScheduled >= opDone );
    if ( opDone % 10 == 0 || opScheduled == opDone )
    {
        LOG_DEBUG( "Updating progress: operations scheduled ", opScheduled,
                   "; operations done: ", opDone );
        m_callback->onParsingStatsUpdated( opDone, opScheduled );
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
        m_opDone.fetch_add( 1, std::memory_order_relaxed );

        updateStats();
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
            m_opDone.fetch_add( 1, std::memory_order_relaxed );
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

void Parser::onIdleChanged( bool idle )
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
