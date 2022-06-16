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
#include "common/NoopCallback.h"
#include "common/util.h"
#include "utils/Filename.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"
#include "logging/Logger.h"
#include "logging/IostreamLogger.h"

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
        , m_discovereryCompleted( false )
    {
    }
    virtual void onDiscoveryStarted() override
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        m_discovereryCompleted = false;
    }

    virtual void onDiscoveryCompleted() override
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        m_discovereryCompleted = true;
        m_cond.notify_all();
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
            return m_doneQueuing == true && m_discovereryCompleted == true;
        });
        assert( res == true );
    }

private:
    compat::Mutex m_mutex;
    compat::ConditionVariable m_cond;
    bool m_doneQueuing;
    bool m_discovereryCompleted;
};

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        std::cerr << "usage: " << argv[0] << " <entrypoint path>" << std::endl;
        return 1;
    }
    auto entrypoint = utils::file::toMrl( argv[1] );

    auto mlDir = getTempPath( "fast_discoverer_cancel_test" );
    auto dbPath = mlDir + "test.db";

    auto testCb = std::make_unique<FastDiscoverCancelCb>();

    Log::SetLogger( std::make_shared<IostreamLogger>() );
    auto ml = std::make_unique<medialibrary::MediaLibrary>( dbPath, mlDir, nullptr );

    unlink( dbPath.c_str() );
    ml->initialize( testCb.get() );

    ml->discover( entrypoint );

    for ( auto i = 0; i < 500; ++i )
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
