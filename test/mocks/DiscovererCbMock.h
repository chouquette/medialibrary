#pragma once

#include <atomic>
#include <condition_variable>

#include "IMediaLibrary.h"

namespace mock
{

class WaitForDiscoveryComplete : public IMediaLibraryCb
{
public:
    virtual void onFileAdded( FilePtr ) override
    {
    }

    virtual void onFileUpdated( FilePtr ) override
    {
    }

    virtual void onDiscoveryStarted( const std::string& )
    {
    }

    virtual void onDiscoveryCompleted( const std::string& )
    {
        --m_nbDiscoveryToWait;
        m_cond.notify_all();
    }

    bool wait()
    {
        // If m_nbDiscoveryToWait is 0 when entering this function, it means the discovery
        // hasn't started yet. This is only used for tests, and so far, with a single discovery module.
        // Obviously, this is a hack and won't work if (when) we add another discovery module...
        // Hello future self, enjoy.
        auto v = 0;
        m_nbDiscoveryToWait.compare_exchange_strong(v, 1);
        std::unique_lock<std::mutex> lock( m_mutex );
        return m_cond.wait_for( lock, std::chrono::seconds( 5 ), [this]() {
            return m_nbDiscoveryToWait == 0;
        } );
    }

private:
    std::atomic_int m_nbDiscoveryToWait;
    std::condition_variable m_cond;
    std::mutex m_mutex;
};

}
