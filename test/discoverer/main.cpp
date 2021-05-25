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
        : m_nbDiscoveryToRun( 0 )
        , m_nbDiscoveryCompleted( 0 )
        , m_isParsingCompleted( false )
        , m_isIdle( false )
        , m_error( false )
    {
    }

    bool waitForCompletion()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        m_cond.wait( lock, [this](){
            return (m_nbDiscoveryToRun > 0 &&
                    m_nbDiscoveryToRun == m_nbDiscoveryCompleted &&
                    m_isParsingCompleted == true &&
                    m_isIdle == true) || m_error;
        });
        return m_error == false;
    }

private:
    virtual void onDiscoveryStarted( const std::string& ) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_nbDiscoveryToRun++;
            m_isParsingCompleted = false;
        }
        m_cond.notify_all();
    }
    virtual void onDiscoveryCompleted(const std::string&, bool success ) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            if ( success == true )
                m_nbDiscoveryCompleted++;
            else
                m_error = true;
        }
        m_cond.notify_all();
    }
    virtual void onParsingStatsUpdated(uint32_t percent) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isParsingCompleted = percent == 100;
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
    uint32_t m_nbDiscoveryToRun;
    uint32_t m_nbDiscoveryCompleted;
    bool m_isParsingCompleted;
    bool m_isIdle;
    bool m_error;
};

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        std::cerr << "usage: " << argv[0] << " <entrypoint>" << std::endl;
        return 1;
    }

    auto mlDir = getTempPath( "discoverer_test" );
    auto dbPath = mlDir + "/test.db";

    unlink( dbPath.c_str() );

    auto testCb = std::make_unique<TestCb>();
    std::unique_ptr<medialibrary::IMediaLibrary> ml{
        NewMediaLibrary( dbPath.c_str(), mlDir.c_str(), false )
    };

    ml->setVerbosity( medialibrary::LogLevel::Debug );
    ml->initialize( testCb.get() );
    auto res = ml->setDiscoverNetworkEnabled( true );
    assert( res );
    ml->discover( argv[1] );

    res = testCb->waitForCompletion();
    return res == true ? 0 : 1;
}
