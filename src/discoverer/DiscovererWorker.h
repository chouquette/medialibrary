#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "IDiscoverer.h"

class DiscovererWorker : public IDiscoverer
{
public:
    DiscovererWorker();
    virtual ~DiscovererWorker();
    void addDiscoverer( std::unique_ptr<IDiscoverer> discoverer );
    void setCallback( IMediaLibraryCb* cb );
    void stop();

    virtual bool discover( const std::string& entryPoint ) override;
    virtual void reload() override;

private:
    void enqueue( const std::string& entryPoint );
    void run();

private:
    std::thread m_thread;
    std::queue<std::string> m_entryPoints;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::atomic_bool m_run;
    std::vector<std::unique_ptr<IDiscoverer>> m_discoverers;
    IMediaLibraryCb* m_cb;
};

