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

#include "IMedia.h"

Parser::Parser(DBConnection dbConnection , IMediaLibraryCb* cb)
    : m_stopParser( false )
    , m_dbConnection( dbConnection )
    , m_callback( cb )
    , m_opToDo( 0 )
    , m_opDone( 0 )
    , m_percent( 0 )
    , m_paused( false )
{
}

Parser::~Parser()
{
    if ( m_thread.joinable() )
    {
        {
            std::lock_guard<std::mutex> lock( m_lock );
            if ( m_tasks.empty() == true )
                m_cond.notify_all();
            m_stopParser = true;
        }
        m_thread.join();
    }
    while ( m_tasks.empty() == false )
    {
        delete m_tasks.front();
        m_tasks.pop();
    }
}

void Parser::addService(std::unique_ptr<IMetadataService> service)
{
    m_services.emplace_back( std::move( service ) );
    std::push_heap( m_services.begin(), m_services.end(), []( const ServicePtr& a, const ServicePtr& b )
    {
        // We want higher priority first
        return a->priority() < b->priority();
    });
}

void Parser::parse( std::shared_ptr<Media> file )
{
    std::lock_guard<std::mutex> lock( m_lock );

    if ( m_services.size() == 0 )
        return;
    m_tasks.push( new Task( file, m_services, m_callback ) );
    ++m_opToDo;
    updateStats();
    if ( m_paused == false )
        m_cond.notify_all();
}

void Parser::start()
{
    // Ensure we don't start multiple times.
    assert( m_thread.joinable() == false );

    m_thread = std::thread{ &Parser::run, this };
}

void Parser::pause()
{
    std::lock_guard<std::mutex> lock( m_lock );
    m_paused = true;
}

void Parser::resume()
{
    std::lock_guard<std::mutex> lock( m_lock );
    m_paused = false;
    m_cond.notify_all();
}

void Parser::run()
{
    LOG_INFO("Starting Parser thread");
    restore();

    while ( m_stopParser == false )
    {
        Task* task = nullptr;
        {
            std::unique_lock<std::mutex> lock( m_lock );
            if ( m_tasks.empty() == true || m_paused == true )
            {
                m_cond.wait( lock, [this]() {
                    return ( m_tasks.empty() == false && m_paused == false )
                            || m_stopParser == true;
                });
                // We might have been woken up because the parser is being destroyed
                if ( m_stopParser  == true )
                    break;
            }
            // Otherwise it's safe to assume we have at least one element.
            task = m_tasks.front();
            m_tasks.pop();
        }
        try
        {
            (*task->it)->run( task->file, task );
            // Consider the task invalid starting from this point. If it completed
            // it cleared itself afterward.
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR( "Caught an exception during ", task->file->mrl(), " parsing: ", ex.what() );
            done( task->file, IMetadataService::Status::Fatal, task );
        }
    }
    LOG_INFO("Exiting Parser thread");
}

void Parser::restore()
{
    if ( m_services.empty() == true )
        return;

    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name
            + " WHERE parsed = 0 AND is_present = 1";
    auto media = Media::fetchAll<Media>( m_dbConnection, req );

    std::lock_guard<std::mutex> lock( m_lock );
    for ( auto& m : media )
    {
        m_tasks.push( new Task( m, m_services, m_callback ) );
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


Parser::Task::Task(std::shared_ptr<Media> file, Parser::ServiceList& serviceList, IMediaLibraryCb* metadataCb )
    : file(file)
    , it( serviceList.begin() )
    , end( serviceList.end() )
    , cb( metadataCb )
{
}


void Parser::done(std::shared_ptr<Media> file, IMetadataService::Status status, void* data )
{
    Task *t = reinterpret_cast<Task*>( data );
    if ( status == IMetadataService::Status::TemporaryUnavailable ||
         status == IMetadataService::Status::Fatal )
    {
        ++m_opDone;
        updateStats();
        delete t;
        return;
    }
    else if ( status == IMetadataService::Status::Success )
    {
        if ( t->cb != nullptr )
            t->cb->onFileUpdated( file );
    }

    ++t->it;
    if (t->it == t->end)
    {
        ++m_opDone;
        updateStats();
        file->markParsed();
        delete t;
        return;
    }
    std::lock_guard<std::mutex> lock( m_lock );
    m_tasks.push( t );
    m_cond.notify_all();
}
