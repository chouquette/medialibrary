#include <algorithm>

#include "Parser.h"

Parser::Parser()
    : m_stopParser( false )
{
}

Parser::~Parser()
{
    if ( m_thread == nullptr )
        return;

    {
        std::lock_guard<std::mutex> lock( m_lock );
        if ( m_tasks.empty() == true )
            m_cond.notify_all();
        m_stopParser = true;
    }
    m_thread->join();
}

void Parser::addService(std::unique_ptr<IMetadataService> service)
{
    m_services.push_back( std::move( service ) );
    std::push_heap( m_services.begin(), m_services.end(), []( const ServicePtr& a, const ServicePtr& b )
    {
        // We want higher priority first
        return a->priority() < b->priority();
    });
}

void Parser::parse(FilePtr file, IParserCb* cb)
{
    std::lock_guard<std::mutex> lock( m_lock );

    m_tasks.push_back( new Task( file, m_services, cb ) );
    if ( m_thread == nullptr )
        m_thread.reset( new std::thread( &Parser::run, this ) );
}

void Parser::run()
{
    while ( m_stopParser == false )
    {
        Task* task = nullptr;
        {
            std::unique_lock<std::mutex> lock( m_lock );
            if ( m_tasks.empty() == true )
            {
                m_cond.wait( lock, [this]() { return m_tasks.empty() == false || m_stopParser == true; });
                // We might have been woken up because the parser is being destroyed
                if ( m_stopParser  == true )
                    return;
            }
            // Otherwise it's safe to assume we have at least one element.
            task = m_tasks.front();
            m_tasks.erase(m_tasks.begin());
        }
        (*task->it)->run( task->file, task );
    }
}


Parser::Task::Task( FilePtr file, Parser::ServiceList& serviceList, IParserCb* parserCb )
    : file(file)
    , it( serviceList.begin() )
    , end( serviceList.end() )
    , cb( parserCb )
{
}


void Parser::done( FilePtr file, ServiceStatus status, void* data )
{
    Task *t = reinterpret_cast<Task*>( data );
    t->cb->onServiceDone( file, status );
    if ( status == StatusTemporaryUnavailable || status == StatusFatal )
    {
        delete t;
        return ;
    }
    ++t->it;
    if (t->it == t->end)
    {
        t->cb->onFileDone( file );
        delete t;
        return;
    }
    std::lock_guard<std::mutex> lock( m_lock );
    m_tasks.push_back( t );
}
