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

#include "medialibrary/IMediaLibrary.h"
#include "mocks/NoopCallback.h"

namespace mock
{

class WaitForDiscoveryComplete : public mock::NoopCallback
{
public:
    WaitForDiscoveryComplete()
        : m_waitingReload( false )
    {
    }

    virtual void onDiscoveryCompleted( const std::string& entryPoint ) override
    {
        if ( entryPoint.empty() == true && m_waitingReload == false )
            return;
        m_done = true;
        m_cond.notify_all();
    }

    bool wait()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        return m_cond.wait_for( lock, std::chrono::seconds( 5 ), [this]() {
            return m_done.load();
        } );
    }

    // We don't synchronously trigger the discovery, so we can't rely on started/completed being called
    // Instead, we manually tell this mock how much discovery we expect. This sucks, but the alternative
    // would probably be to have an extra IMediaLibraryCb member to signal that a discovery has been queue.
    // however, in practice, this is a callback that says "yep, you've called IMediaLibrary::discover()"
    // which is probably lame.
    void prepareForWait()
    {
        m_done = false;
        m_waitingReload = false;
    }

    void prepareForReload()
    {
        m_done = false;
        m_waitingReload = true;
    }

private:
    std::atomic_bool m_done;
    std::atomic_bool m_waitingReload;
    std::condition_variable m_cond;
    compat::Mutex m_mutex;
};

}
