/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2020 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "MediaLibrary.h"
#include "mocks/NoopCallback.h"
#include "utils/Filename.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"

#include <iostream>
#include <condition_variable>
#include <set>
#include <cassert>
#include <unistd.h>

class FastDiscoverCancelCb : public mock::NoopCallback
{
public:
    FastDiscoverCancelCb()
        : m_doneQueuing( false )
        , m_discovererRunning( true )
    {
    }
    virtual void onDiscoveryStarted( const std::string& ep ) override
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        assert( m_discoveryStarted.find( ep ) == cend( m_discoveryStarted ) );
        m_discoveryStarted.insert( ep );
    }
    virtual void onDiscoveryCompleted( const std::string& ep, bool ) override
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        auto it = m_discoveryStarted.find( ep );
        assert( it != cend( m_discoveryStarted ) );
        m_discoveryStarted.erase( it );
        if ( m_discoveryStarted.empty() == true )
            m_cond.notify_all();
    }
    virtual void onReloadStarted( const std::string& ep ) override
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        assert( m_reloadStarted.find( ep ) == cend( m_reloadStarted ) );
        m_reloadStarted.insert( ep );
    }
    virtual void onReloadCompleted( const std::string& ep, bool ) override
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        auto it = m_reloadStarted.find( ep );
        assert( it != cend( m_reloadStarted ) );
        m_reloadStarted.erase( it );
    }
    virtual void onBackgroundTasksIdleChanged( bool idle ) override
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        m_discovererRunning = !idle;
        m_cond.notify_all();
    }

    void check()
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        assert( m_discoveryStarted.empty() == true );
        assert( m_reloadStarted.empty() == true );
    }

    void markDoneQueuing()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        m_doneQueuing = true;
    }

    void waitForDiscoveryCompleted()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        auto res = m_cond.wait_for( lock, std::chrono::minutes{ 10 }, [this](){
            return m_doneQueuing == true && m_discovererRunning == false;
        });
        assert( res == true );
    }

private:
    std::set<std::string> m_discoveryStarted;
    std::set<std::string> m_reloadStarted;
    compat::Mutex m_mutex;
    compat::ConditionVariable m_cond;
    bool m_doneQueuing;
    bool m_discovererRunning;
};

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        std::cerr << "usage: " << argv[0] << " <entrypoint path>" << std::endl;
        return 1;
    }
    auto entrypoint = utils::file::toMrl( argv[1] );

    auto testCb = std::make_unique<FastDiscoverCancelCb>();
    auto ml = std::make_unique<medialibrary::MediaLibrary>();
//    ml->setVerbosity( LogLevel::Debug );
    unlink( "/tmp/test.db" );
    ml->initialize( "/tmp/test.db", "/tmp/ml_folder", testCb.get() );
    ml->start();

    ml->discover( entrypoint );

    for ( auto i = 0; i < 1000; ++i )
    {
        auto fsFactory = ml->fsFactoryForMrl( entrypoint );
        auto fsDir = fsFactory->createDirectory( entrypoint );
        auto j = 0u;
        for ( const auto& d : fsDir->dirs() )
        {
            switch ( j % 2 )
            {
            case 0:
                ml->banFolder( d->mrl() );
                ml->unbanFolder( d->mrl() );
                break;
            case 1:
                ml->removeEntryPoint( d->mrl() );
                ml->discover( d->mrl() );
                break;
            }
            ++j;
        }
    }
    ml->reload( entrypoint );

    testCb->markDoneQueuing();
    testCb->waitForDiscoveryCompleted();
    return 0;
}
