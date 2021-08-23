/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "compat/Thread.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"
#include "common/NoopCallback.h"
#include "medialibrary/IMediaLibrary.h"

#include <atomic>
#include <iostream>
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <unistd.h>

namespace
{

constexpr auto NbIterations = 10u;

class MockCallback : public mock::NoopCallback
{
public:
    MockCallback();
    virtual bool waitForParsingComplete();
    void initWait();
    void signalEnd();
    bool isTestComplete();
protected:
    virtual void onDiscoveryCompleted() override;
    virtual void onParsingStatsUpdated( uint32_t done, uint32_t scheduled ) override;

    compat::ConditionVariable m_parsingCompleteVar;
    compat::Mutex m_parsingMutex;
    bool m_done;
    bool m_discoveryCompleted;

    std::atomic_bool m_testDone;
};

MockCallback::MockCallback()
    : m_done( false )
    , m_discoveryCompleted( false )
    , m_testDone( false )
{
}

bool MockCallback::waitForParsingComplete()
{
    std::unique_lock<compat::Mutex> lock( m_parsingMutex, std::adopt_lock );
    return m_parsingCompleteVar.wait_for( lock, std::chrono::minutes{ 10 }, [this]() {
        return m_done;
    });
}

void MockCallback::initWait()
{
    m_parsingMutex.lock();
    m_done = false;
    // Don't reset m_discoveryCompleted since we will discover only once
}

void MockCallback::signalEnd()
{
    m_testDone = true;
}

bool MockCallback::isTestComplete()
{
    return m_testDone == true;
}

void MockCallback::onDiscoveryCompleted()
{
    std::lock_guard<compat::Mutex> lock( m_parsingMutex );
    m_discoveryCompleted = true;
}

void MockCallback::onParsingStatsUpdated( uint32_t done, uint32_t scheduled )
{
    if ( done == scheduled )
    {
        std::lock_guard<compat::Mutex> lock( m_parsingMutex );
        if ( m_discoveryCompleted == false )
            return;
        m_done = true;
        m_parsingCompleteVar.notify_all();
    }
}

struct Tester
{
    IMediaLibrary* ml;
    std::shared_ptr<MockCallback> cbMock;
    std::string samplesFolder;

    void discovererMainLoop()
    {
        for ( auto i = 0u; i < NbIterations; ++i )
        {
            cbMock->initWait();
            if ( i == 0 )
                ml->discover( samplesFolder );
            else
                ml->forceRescan();
            auto res = cbMock->waitForParsingComplete();
            assert( res == true );
            std::cout << "Parsing #" << i << " completed." << std::endl;
        }
    }

    void readerMainLoop()
    {
        while ( cbMock->isTestComplete() == false )
        {
            auto mode = rand() % 7;
            switch ( mode )
            {
                case 0:
                {
                    auto audio = ml->audioFiles( nullptr )->all();
                    break;
                }
                case 1:
                {
                    auto video = ml->videoFiles( nullptr )->all();
                    break;
                }
                case 2:
                {
                    auto artists = ml->artists( ArtistIncluded::All, nullptr )->all();
                    break;
                }
                case 3:
                {
                    auto albums = ml->albums( nullptr );
                    break;
                }
                case 4:
                {
                    auto genres = ml->genres( nullptr )->all();
                    break;
                }
                case 5:
                {
                    auto pl = ml->playlists( PlaylistType::All, nullptr )->all();
                    break;
                }
                case 6:
                {
                    auto folders = ml->folders( IMedia::Type::Unknown, nullptr )->all();
                    break;
                }
                default:
                    abort();
            }
        }
    }
};

}

int main( int argc, char** argv)
{
    if ( argc < 2 )
    {
        std::cerr << "usage: " << argv[0] << " <samples folder>" << std::endl;
        return 1;
    }

    srand(time(nullptr));

    auto cbMock = std::make_shared<MockCallback>();
    std::unique_ptr<IMediaLibrary> ml{
        NewMediaLibrary( "sqliteload.db", "/tmp/ml/", false )
    };
    ml->setVerbosity( medialibrary::LogLevel::Info );
    unlink( "sqliteload.db" );
    ml->initialize( cbMock.get() );

    Tester T{ ml.get(), cbMock, argv[1] };

    compat::Thread discoverer( &Tester::discovererMainLoop, &T );
    compat::Thread reader1( &Tester::readerMainLoop, &T );
    compat::Thread reader2( &Tester::readerMainLoop, &T );

    discoverer.join();
    cbMock->signalEnd();

    reader1.join();
    reader2.join();

    return 0;
}
