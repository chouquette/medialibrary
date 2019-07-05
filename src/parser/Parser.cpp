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

#include <algorithm>
#include <utility>

#include "medialibrary/IMediaLibrary.h"
#include "Media.h"
#include "File.h"
#include "ParserWorker.h"

namespace medialibrary
{
namespace parser
{

Parser::Parser( MediaLibrary* ml )
    : m_ml( ml )
    , m_callback( ml->getCb() )
    , m_opToDo( 0 )
    , m_opDone( 0 )
    , m_percent( 0 )
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
    m_services.push_back( std::move( worker ) );
}

void Parser::parse( std::shared_ptr<Task> task )
{
    if ( m_services.empty() == true )
        return;
    auto isRestoreTask = task == nullptr;
    m_services[0]->parse( std::move( task ) );
    if ( isRestoreTask == false )
    {
        ++m_opToDo;
        updateStats();
    }
}

void Parser::start()
{
    restore();
}

void Parser::pause()
{
    for ( auto& s : m_services )
        s->pause();
}

void Parser::resume()
{
    for ( auto& s : m_services )
        s->resume();
}

void Parser::stop()
{
    for ( auto& s : m_services )
    {
        s->signalStop();
    }
    for ( auto& s : m_services )
    {
        s->stop();
    }
}

void Parser::flush()
{
    for ( auto& s : m_services )
        s->flush();
}

void Parser::restart()
{
    for ( auto& s : m_services )
        s->restart();
}

void Parser::restore()
{
    if ( m_services.empty() == true )
        return;
    parse( nullptr );
}

void Parser::updateStats()
{
    if ( m_opDone == 0 && m_opToDo > 0 && m_chrono == decltype(m_chrono){})
        m_chrono = std::chrono::steady_clock::now();
    auto percent = m_opToDo > 0 ? ( m_opDone * 100 / m_opToDo ) : 0;
    if ( percent != m_percent )
    {
        m_percent = percent;
        LOG_DEBUG( "Updating progress: ", percent );
        m_callback->onParsingStatsUpdated( m_percent );
        if ( m_percent == 100 )
        {
            auto duration = std::chrono::steady_clock::now() - m_chrono;
            LOG_VERBOSE( "Finished all parsing operations in ",
                       std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count(), "ms" );
            m_chrono = decltype(m_chrono){};
        }
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
        ++m_opDone;

        updateStats();
        // We create a separate task for refresh, which doesn't count toward
        // (mrl,parent_playlist) uniqueness. In order to allow for a subsequent
        // refresh of the same file, we remove it once the refresh is complete.
        if ( t->isRefresh() == true )
            Task::destroy( m_ml, t->id() );
        return;
    }
    if ( status == Status::Requeue )
        serviceIdx = 0;

    // If some services declined to parse the file, start over again.
    assert( serviceIdx < m_services.size() );
    updateStats();
    m_services[serviceIdx]->parse( std::move( t ) );
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
    for ( const auto& s : m_services )
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
