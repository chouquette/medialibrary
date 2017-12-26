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

#include "ParserService.h"
#include "Parser.h"
#include "Media.h"

namespace medialibrary
{

ParserService::ParserService()
    : m_ml( nullptr )
    , m_cb( nullptr )
    , m_parserCb( nullptr )
    , m_stopParser( false )
    , m_paused( false )
    , m_idle( true )
{
}

void ParserService::start()
{
    // Ensure we don't start multiple times.
    assert( m_threads.size() == 0 );
    for ( auto i = 0u; i < nbThreads(); ++i )
        m_threads.emplace_back( &ParserService::mainloop, this );
}

void ParserService::pause()
{
    std::lock_guard<compat::Mutex> lock( m_lock );
    m_paused = true;
}

void ParserService::resume()
{
    std::lock_guard<compat::Mutex> lock( m_lock );
    m_paused = false;
    m_cond.notify_all();
}

void ParserService::signalStop()
{
    for ( auto& t : m_threads )
    {
        if ( t.joinable() )
        {
            {
                std::lock_guard<compat::Mutex> lock( m_lock );
                m_cond.notify_all();
                m_stopParser = true;
            }
        }
    }
}

void ParserService::stop()
{
    for ( auto& t : m_threads )
    {
        if ( t.joinable() )
            t.join();
    }
}

void ParserService::parse( std::unique_ptr<parser::Task> t )
{
    if ( m_threads.size() == 0 )
    {
        // Since the thread isn't started, no need to lock the mutex before pushing the task
        m_tasks.push( std::move( t ) );
        start();
    }
    else
    {
        std::lock_guard<compat::Mutex> lock( m_lock );
        m_tasks.push( std::move( t ) );
        m_cond.notify_all();
    }
}

void ParserService::initialize( MediaLibrary* ml, IParserCb* parserCb )
{
    m_ml = ml;
    m_cb = ml->getCb();
    m_notifier = ml->getNotifier();
    m_parserCb = parserCb;
    // Run the service specific initializer
    initialize();
}

bool ParserService::isIdle() const
{
    return m_idle;
}

void ParserService::flush()
{
    std::unique_lock<compat::Mutex> lock( m_lock );
    assert( m_paused == true || m_threads.empty() == true );
    m_idleCond.wait( lock, [this]() {
        return m_idle == true;
    });
    while ( m_tasks.empty() == false )
        m_tasks.pop();
}

uint8_t ParserService::nbNativeThreads() const
{
    auto nbProcs = static_cast<uint8_t>( compat::Thread::hardware_concurrency() );
    if ( nbProcs == 0 )
        return 1;
    return nbProcs;
}

bool ParserService::initialize()
{
    return true;
}

void ParserService::mainloop()
{
    // It would be unsafe to call name() at the end of this function, since
    // we might stop the thread during ParserService destruction. This implies
    // that the underlying service has been deleted already.
    std::string serviceName = name();
    LOG_INFO("Entering ParserService [", serviceName, "] thread");
    setIdle( false );

    while ( m_stopParser == false )
    {
        std::unique_ptr<parser::Task> task;
        {
            std::unique_lock<compat::Mutex> lock( m_lock );
            if ( m_tasks.empty() == true || m_paused == true )
            {
                LOG_INFO( "Halting ParserService [", serviceName, "] mainloop" );
                setIdle( true );
                m_idleCond.notify_all();
                m_cond.wait( lock, [this]() {
                    return ( m_tasks.empty() == false && m_paused == false )
                            || m_stopParser == true;
                });
                LOG_INFO( "Resuming ParserService [", serviceName, "] mainloop" );
                // We might have been woken up because the parser is being destroyed
                if ( m_stopParser  == true )
                    break;
                setIdle( false );
            }
            // Otherwise it's safe to assume we have at least one element.
            LOG_INFO('[', serviceName, "] has ", m_tasks.size(), " tasks remaining" );
            task = std::move( m_tasks.front() );
            m_tasks.pop();
        }
        if ( isCompleted( *task ) == true )
        {
            LOG_INFO( "Skipping completed task [", serviceName, "] on ", task->mrl );
            m_parserCb->done( std::move( task ), parser::Task::Status::Success );
            continue;
        }
        parser::Task::Status status;
        try
        {
            LOG_INFO( "Executing ", serviceName, " task on ", task->mrl );
            auto chrono = std::chrono::steady_clock::now();
            if ( ( task->file != nullptr && task->file->isDeleted() )
                 || ( task->media != nullptr && task->media->isDeleted() ) )
                status = parser::Task::Status::Fatal;
            else
            {
                if ( task->file != nullptr )
                    task->file->startParserStep(); // FIXME ?
                status = run( *task );
                auto duration = std::chrono::steady_clock::now() - chrono;
                LOG_INFO( "Done executing ", serviceName, " task on ", task->mrl, " in ",
                          std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count(), "ms" );
            }
        }
        catch ( const std::exception& ex )
        {
            LOG_ERROR( "Caught an exception during ", task->mrl, " [", serviceName, "] parsing: ", ex.what() );
            status = parser::Task::Status::Fatal;
        }
        m_parserCb->done( std::move( task ), status );
    }
    LOG_INFO("Exiting ParserService [", serviceName, "] thread");
    setIdle( true );
}

void ParserService::setIdle(bool isIdle)
{
    // Calling the idleChanged callback will trigger a call to isIdle, so set the value before
    // invoking it, otherwise we have an incoherent state.
    m_idle = isIdle;
    m_parserCb->onIdleChanged( isIdle );
}

}
