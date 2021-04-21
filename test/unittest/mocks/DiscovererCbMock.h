/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#pragma once

#include <atomic>
#include <condition_variable>

#include "medialibrary/IMediaLibrary.h"
#include "common/NoopCallback.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"

#include <cassert>

namespace mock
{

class WaitForDiscoveryComplete : public mock::NoopCallback
{
public:
    WaitForDiscoveryComplete()
        : m_discoveryDone( false )
        , m_initialDiscoveryDone( false )
        , m_banFolderDone( false )
        , m_unbanFolderDone( false )
        , m_entryPointRemoved( false )
    {
    }

    virtual void onDiscoveryCompleted( const std::string& entryPoint, bool ) override
    {
        assert( entryPoint.empty() == false );
        m_discoveryDone = true;
        m_cond.notify_all();
    }

    virtual void onEntryPointBanned( const std::string&, bool ) override
    {
        m_banFolderDone = true;
        m_cond.notify_all();
    }

    virtual void onEntryPointUnbanned( const std::string&, bool ) override
    {
        m_unbanFolderDone = true;
        m_cond.notify_all();
    }

    virtual void onEntryPointRemoved( const std::string&, bool ) override
    {
        m_entryPointRemoved = true;
        m_cond.notify_all();
    }

    bool waitDiscovery()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        auto res = m_cond.wait_for( lock, std::chrono::seconds{ 15 }, [this]() {
            return m_discoveryDone.load();
        } );
        m_discoveryDone = false;
        m_initialDiscoveryDone = true;
        return res;
    }

    bool waitReload()
    {
        // A reload() request when no discovery has run will never reload anything
        // and therefor won't invoke any callback
        if ( m_initialDiscoveryDone == false )
            return true;
        std::unique_lock<compat::Mutex> lock( m_mutex );
        auto res = m_cond.wait_for( lock, std::chrono::seconds{ 15 }, [this]() {
            return m_discoveryDone.load();
        } );
        m_discoveryDone = false;
        return res;
    }

    bool waitBanFolder()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        auto res = m_cond.wait_for( lock, std::chrono::seconds{ 15 }, [this]() {
            return m_banFolderDone.load();
        });
        m_banFolderDone = false;
        return res;
    }

    bool waitUnbanFolder()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        auto res = m_cond.wait_for( lock, std::chrono::seconds{ 15 }, [this]() {
            return m_unbanFolderDone.load();
        });
        m_unbanFolderDone = false;
        return res;
    }

    bool waitEntryPointRemoved()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        auto res =  m_cond.wait_for( lock, std::chrono::seconds{ 15 }, [this]() {
            return m_entryPointRemoved.load();
        });
        m_entryPointRemoved = false;
        return res;
    }

private:
    std::atomic_bool m_discoveryDone;
    std::atomic_bool m_initialDiscoveryDone;
    std::atomic_bool m_banFolderDone;
    std::atomic_bool m_unbanFolderDone;
    std::atomic_bool m_entryPointRemoved;
    compat::ConditionVariable m_cond;
    compat::Mutex m_mutex;
};

}
