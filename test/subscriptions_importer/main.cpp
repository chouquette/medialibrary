/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "compat/ConditionVariable.h"
#include "compat/Mutex.h"

#include <iostream>
#include <unistd.h>
#include <cassert>

class TestCb : public mock::NoopCallback
{
public:
    TestCb()
        : m_isParsingCompleted( false )
    {
    }

    bool waitForCompletion()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        m_cond.wait( lock, [this](){
            return m_isParsingCompleted == true;
        });
        return true;
    }

    void prepareWaitForCache()
    {
        std::lock_guard<compat::Mutex> lock{ m_mutex };
        m_cacheUpdated = false;
        m_cacheWorkerIdle = false;
    }

    void waitForCacheUpdated()
    {
        std::unique_lock<compat::Mutex> lock{ m_mutex };
        m_cond.wait( lock, [this]() {
            return m_cacheWorkerIdle == true && m_cacheUpdated == true;
        });
    }

private:
    virtual void onParsingStatsUpdated( uint32_t done, uint32_t scheduled ) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isParsingCompleted = (done == scheduled);
        }
        m_cond.notify_all();
    }

    virtual void onCacheIdleChanged( bool idle ) override
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        m_cacheWorkerIdle = idle;
        m_cond.notify_all();
    }

    virtual void onSubscriptionCacheUpdated( int64_t ) override
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        assert( m_cacheWorkerIdle == false );
        m_cacheUpdated = true;
    }

private:
    compat::ConditionVariable m_cond;
    compat::Mutex m_mutex;
    bool m_isParsingCompleted;
    bool m_cacheUpdated;
    bool m_cacheWorkerIdle;
};

static void usage( const char* const* argv )
{
    std::cerr << "usage: " << argv[0] << "[-q] [-c] <RSS_MRL>\n"
                 "-q: Use Error log level. Default is Debug\n"
                 "-c: Automatically cache this subscription\n"
              << std::endl;
}

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        usage(argv);
        exit(1);
    }

    auto quiet = false;
    auto cache = false;
    int opt;
    while ( ( opt = getopt( argc, argv, "qc" ) ) != -1 )
    {
        switch ( opt )
        {
        case 'q':
            quiet = true;
            break;
        case 'c':
            cache = true;
            break;
        default:
            usage( argv );
            exit( 1 );
        }
    }
    if ( optind >= argc )
    {
        std::cerr << "Missing subscription RSS MRL" << std::endl;
        usage(argv);
        exit(2);
    }
    auto subMrl = argv[optind];

    auto mlDir = getTempPath( "subscriptions_importer" );
    auto dbPath = mlDir + "/test.db";
    unlink( dbPath.c_str() );

    auto testCb = std::make_unique<TestCb>();
    std::unique_ptr<medialibrary::IMediaLibrary> ml{
        NewMediaLibrary( dbPath.c_str(), mlDir.c_str(), false, nullptr )
    };

    ml->setVerbosity( quiet == true ? medialibrary::LogLevel::Error :
                                      medialibrary::LogLevel::Debug );
    auto initRes = ml->initialize( testCb.get() );
    assert( initRes == InitializeResult::Success );
    ml->setDiscoverNetworkEnabled( true );

    auto service = ml->service( IService::Type::Podcast );
    service->addSubscription( std::move( subMrl ) );

    testCb->waitForCompletion();
    if ( cache == true )
    {
        testCb->prepareWaitForCache();
        ml->cacheNewSubscriptionMedia();
        testCb->waitForCacheUpdated();
    }
}
