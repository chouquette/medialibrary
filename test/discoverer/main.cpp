/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "medialibrary/IMediaLibrary.h"
#include "test/common/NoopCallback.h"
#include "test/common/util.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <unistd.h>
#include <cassert>

class TestCb : public mock::NoopCallback
{
public:
    TestCb()
        : m_isDiscoveryCompleted( false )
        , m_isParsingCompleted( false )
        , m_isIdle( false )
        , m_error( false )
    {
    }

    bool waitForCompletion()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        m_cond.wait( lock, [this](){
            return (m_isDiscoveryCompleted == true &&
                    m_isParsingCompleted == true &&
                    m_isIdle == true) || m_error;
        });
        return m_error == false;
    }

private:
    virtual void onDiscoveryStarted() override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isDiscoveryCompleted = false;
            m_isParsingCompleted = false;
        }
        m_cond.notify_all();
    }

    virtual void onDiscoveryCompleted() override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isDiscoveryCompleted = true;
        }
        m_cond.notify_all();
    }

    virtual void onDiscoveryFailed( const std::string& ) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_error = true;
        }
        m_cond.notify_all();
    }

    virtual void onParsingStatsUpdated( uint32_t done, uint32_t scheduled ) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isParsingCompleted = (done == scheduled);
        }
        m_cond.notify_all();
    }
    virtual void onBackgroundTasksIdleChanged(bool isIdle) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isIdle = isIdle;
        }
        m_cond.notify_all();
    }

private:
    compat::ConditionVariable m_cond;
    compat::Mutex m_mutex;
    bool m_isDiscoveryCompleted;
    bool m_isParsingCompleted;
    bool m_isIdle;
    bool m_error;
};

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        std::cerr << "usage: " << argv[0] << " <entrypoint> [nb_runs] [-q]" << std::endl;
        return 1;
    }

    auto mlDir = getTempPath( "discoverer_test" );
    auto dbPath = mlDir + "/test.db";
    auto nbRuns = 1;
    auto quiet = false;
    for ( auto i = 2; i < argc; ++i )
    {
        if ( !strcmp( argv[i], "-q" ) )
            quiet = true;
        else
            nbRuns = atoi( argv[i] );
    }


    unlink( dbPath.c_str() );

    auto testCb = std::make_unique<TestCb>();
    std::unique_ptr<medialibrary::IMediaLibrary> ml{
        NewMediaLibrary( dbPath.c_str(), mlDir.c_str(), false )
    };

    ml->setVerbosity( quiet == true ? medialibrary::LogLevel::Error :
                                      medialibrary::LogLevel::Debug );
    ml->initialize( testCb.get() );
    auto res = ml->setDiscoverNetworkEnabled( true );
    assert( res );
    for ( auto i = 0; i < nbRuns; ++i )
    {
        ml->discover( argv[1] );
        res = testCb->waitForCompletion();
        if ( res == false )
            return 1;
        if ( i < nbRuns - 1 )
            ml->forceRescan();
    }
    return 0;
}
