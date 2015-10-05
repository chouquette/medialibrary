#include "DiscovererWorker.h"

#include "logging/Logger.h"

DiscovererWorker::DiscovererWorker()
    : m_run( true )
{
}

DiscovererWorker::~DiscovererWorker()
{
    {
        std::unique_lock<std::mutex> lock( m_mutex );
        while ( m_entryPoints.empty() == false )
            m_entryPoints.pop();
        m_run = false;
    }
    m_cond.notify_all();
    m_thread.join();
}

void DiscovererWorker::addDiscoverer( std::unique_ptr<IDiscoverer> discoverer )
{
    m_discoverers.push_back( std::move( discoverer ) );
}

void DiscovererWorker::setCallback(IMediaLibraryCb* cb)
{
    m_cb = cb;
}

bool DiscovererWorker::discover( const std::string& entryPoint )
{
    if ( entryPoint.length() == 0 )
        return false;
    enqueue( entryPoint );
    return true;
}

void DiscovererWorker::reload()
{
    enqueue( "" );
}

void DiscovererWorker::enqueue( const std::string& entryPoint )
{
    std::unique_lock<std::mutex> lock( m_mutex );

    m_entryPoints.emplace( entryPoint );
    if ( m_thread.get_id() == std::thread::id{} )
        m_thread = std::thread( &DiscovererWorker::run, this );
    // Since we just added an element, let's not check for size == 0 :)
    else if ( m_entryPoints.size() == 1 )
        m_cond.notify_all();
}

void DiscovererWorker::run()
{
    while ( m_run == true )
    {
        std::string entryPoint;
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            if ( m_entryPoints.size() == 0 )
            {
                m_cond.wait( lock, [this]() { return m_entryPoints.size() > 0 || m_run == false ; } );
                if ( m_run == false )
                    break;
            }
            entryPoint = m_entryPoints.front();
            m_entryPoints.pop();
        }
        if ( entryPoint.length() > 0 )
        {
            if ( m_cb != nullptr )
                m_cb->onDiscoveryStarted( entryPoint );
            for ( auto& d : m_discoverers )
            {
                // Assume only one discoverer can handle an entrypoint.
                if ( d->discover( entryPoint ) == true )
                    break;
                if ( m_run == false )
                    break;
            }
            if ( m_cb != nullptr )
                m_cb->onDiscoveryCompleted( entryPoint );
        }
        else
        {
            for ( auto& d : m_discoverers )
            {
                d->reload();
                if ( m_run == false )
                    break;
            }
        }
    }
    LOG_INFO( "Exiting DiscovererWorker thread" );
}

