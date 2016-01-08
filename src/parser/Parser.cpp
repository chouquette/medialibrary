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

#include "IMediaLibrary.h"
#include "Media.h"
#include "File.h"

Parser::Parser( DBConnection dbConnection, MediaLibrary* ml, IMediaLibraryCb* cb )
    : m_dbConnection( dbConnection )
    , m_ml( ml )
    , m_callback( cb )
    , m_opToDo( 0 )
    , m_opDone( 0 )
    , m_percent( 0 )
{
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
    auto files = File::fetchAll<File>( m_dbConnection, req );

    for ( auto& f : files )
    {
        auto m = f->media();
        parse( m, f );
    }
}

void Parser::updateStats()
{
    if ( m_callback == nullptr )
        return;

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
    updateStats();

    if ( status == parser::Task::Status::TemporaryUnavailable ||
         status == parser::Task::Status::Fatal )
    {
        return;
    }
    if ( status == parser::Task::Status::Success )
    {
        if ( m_callback != nullptr )
            m_callback->onFileUpdated( t->media );
    }

    auto serviceIdx = ++t->currentService;
    if ( serviceIdx == m_services.size() )
    {
        t->file->markParsed();
        return;
    }
    m_services[serviceIdx]->parse( std::move( t ) );
}
