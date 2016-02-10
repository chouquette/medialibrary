/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#pragma once

#include <atomic>
#include <condition_variable>

#include "IMediaLibrary.h"
#include "mocks/NoopCallback.h"

namespace mock
{

class WaitForDiscoveryComplete : public mock::NoopCallback
{
public:
    virtual void onDiscoveryCompleted( const std::string& ) override
    {
        if ( --m_nbDiscoveryToWait == 0 )
            m_cond.notify_all();
    }

    virtual void onReloadCompleted( const std::string& ) override
    {
        if ( --m_nbReloadExpected == 0 )
            m_reloadCond.notify_all();
    }

    bool wait()
    {
        std::unique_lock<std::mutex> lock( m_mutex );
        return m_cond.wait_for( lock, std::chrono::seconds( 5 ), [this]() {
            return m_nbDiscoveryToWait == 0;
        } );
    }

    // We don't synchronously trigger the discovery, so we can't rely on started/completed being called
    // Instead, we manually tell this mock how much discovery we expect. This sucks, but the alternative
    // would probably be to have an extra IMediaLibraryCb member to signal that a discovery has been queue.
    // however, in practice, this is a callback that says "yep, you've called IMediaLibrary::discover()"
    // which is probably lame.
    void prepareForWait(int nbExpected)
    {
        m_nbDiscoveryToWait = nbExpected;
    }

    void prepareForReload()
    {
        m_nbReloadExpected = 1;
    }

    bool waitForReload()
    {
        std::unique_lock<std::mutex> lock( m_mutex );
        return m_reloadCond.wait_for( lock, std::chrono::seconds( 5 ), [this](){
            return m_nbReloadExpected == 0;
        });
    }

private:
    std::atomic_int m_nbDiscoveryToWait;
    std::condition_variable m_cond;
    std::mutex m_mutex;

    std::atomic_int m_nbReloadExpected;
    std::condition_variable m_reloadCond;
    std::mutex m_reloadMutex;
};

}
