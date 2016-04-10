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

#include "Parser.h"

#include <algorithm>

#include "medialibrary/IMediaLibrary.h"
#include "Media.h"
#include "File.h"
#include "utils/ModificationsNotifier.h"

Parser::Parser( MediaLibrary* ml )
    : m_ml( ml )
    , m_callback( ml->getCb() )
    , m_notifier( ml->getNotifier() )
    , m_opToDo( 0 )
    , m_opDone( 0 )
    , m_percent( 0 )
{
}

Parser::~Parser()
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

void Parser::restore()
{
    if ( m_services.empty() == true )
        return;

    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE parsed = 0 AND is_present = 1";
    auto files = File::fetchAll<File>( m_ml, req );

    for ( auto& f : files )
    {
        auto m = f->media();
        parse( m, f );
    }
}

void Parser::updateStats()
{
    auto percent = m_opToDo > 0 ? ( m_opDone * 100 / m_opToDo ) : 0;
    if ( percent != m_percent )
    {
        m_percent = percent;
        m_callback->onParsingStatsUpdated( m_percent );
    }
}

void Parser::done( std::unique_ptr<parser::Task> t, parser::Task::Status status )
{
    ++m_opDone;

    auto serviceIdx = ++t->currentService;

    if ( status == parser::Task::Status::TemporaryUnavailable ||
         status == parser::Task::Status::Fatal )
    {
        if ( serviceIdx < m_services.size() )
        {
            // We won't process the next tasks, so we need to keep the number of "todo" operations coherent:
            m_opToDo -= m_services.size() - serviceIdx;
            updateStats();
        }
        return;
    }
    updateStats();

    if ( status == parser::Task::Status::Success )
    {
        m_notifier->notifyMediaModification( t->media );
    }

    if ( serviceIdx == m_services.size() )
    {
        t->file->markParsed();
        return;
    }
    m_services[serviceIdx]->parse( std::move( t ) );
}
