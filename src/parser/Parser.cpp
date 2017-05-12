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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "Parser.h"

#include <algorithm>

#include "medialibrary/IMediaLibrary.h"
#include "Media.h"
#include "File.h"
#include "ParserService.h"

namespace medialibrary
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
    service->initialize( m_ml, this );
    m_services.push_back( std::move( service ) );
}

void Parser::parse( std::shared_ptr<Media> media, std::shared_ptr<File> file )
{
    if ( m_services.size() == 0 )
        return;
    m_services[0]->parse( std::unique_ptr<parser::Task>( new parser::Task( media, file ) ) );
    m_opToDo += m_services.size();
    updateStats();
}

void Parser::start()
{
    restore();
    for ( auto& s : m_services )
        s->start();
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

void Parser::restore()
{
    if ( m_services.empty() == true )
        return;

    auto files = File::fetchUnparsed( m_ml );
    LOG_INFO( "Resuming parsing on ", files.size(), " mrl" );
    for ( auto& f : files )
    {
        auto m = f->media();
        parse( m, f );
    }
}

void Parser::updateStats()
{
    if ( m_opDone == 0 && m_opToDo > 0 && m_chrono == decltype(m_chrono){})
        m_chrono = std::chrono::steady_clock::now();
    auto percent = m_opToDo > 0 ? ( m_opDone * 100 / m_opToDo ) : 0;
    if ( percent != m_percent )
    {
        m_percent = percent;
        m_callback->onParsingStatsUpdated( m_percent );
        if ( m_percent == 100 )
        {
            auto duration = std::chrono::steady_clock::now() - m_chrono;
            LOG_DEBUG( "Finished all parsing operations in ",
                       std::chrono::duration_cast<std::chrono::seconds>( duration ).count(), "s" );
            m_chrono = decltype(m_chrono){};
        }
    }
}

void Parser::done( std::unique_ptr<parser::Task> t, parser::Task::Status status )
{
    ++m_opDone;

    auto serviceIdx = ++t->currentService;

    if ( status == parser::Task::Status::TemporaryUnavailable ||
         status == parser::Task::Status::Fatal ||
         t->file->parserStep() == File::ParserStep::Completed )
    {
        if ( serviceIdx < m_services.size() )
        {
            // We won't process the next tasks, so we need to keep the number of "todo" operations coherent:
            m_opToDo -= m_services.size() - serviceIdx;
        }
        updateStats();
        return;
    }

    // If some services declined to parse the file, start over again.
    if ( serviceIdx == m_services.size() )
    {
        t->currentService = serviceIdx = 0;
        m_opToDo += m_services.size();
        LOG_INFO("Running parser chain again for ", t->file->mrl());
    }
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
