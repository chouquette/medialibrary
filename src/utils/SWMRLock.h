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

#include "compat/ConditionVariable.h"
#include "compat/Mutex.h"
#include "compat/Thread.h"

#include <cassert>
#include <algorithm>
#include <vector>

namespace medialibrary
{

namespace utils
{

///
/// \brief Single Write Multiple Reader lock
///
class SWMRLock
{
public:
    void lock_read()
    {
        auto tid = compat::this_thread::get_id();

        std::unique_lock<compat::Mutex> lock( m_lock );
        ++m_nbReaderWaiting;
        m_cond.wait( lock, [this, tid](){
            if ( must_give_way( tid ) )
                /* There are other clients with priority access */
                return false;
            return m_writing == false;
        });
        --m_nbReaderWaiting;
        m_nbReader++;
    }

    void unlock_read()
    {
        std::unique_lock<compat::Mutex> lock( m_lock );
        --m_nbReader;
        if ( m_nbReader == 0 && m_nbWriterWaiting > 0 )
            m_cond.notify_one();
    }

    void lock_write()
    {
        auto tid = compat::this_thread::get_id();

        std::unique_lock<compat::Mutex> lock( m_lock );
        ++m_nbWriterWaiting;
        m_cond.wait( lock, [this, tid](){
            if ( must_give_way( tid ) )
                /* There are other clients with priority access */
                return false;
            return m_writing == false && m_nbReader == 0;
        });
        --m_nbWriterWaiting;
        m_writing = true;
    }

    void unlock_write()
    {
        std::unique_lock<compat::Mutex> lock( m_lock );
        m_writing = false;
        if ( m_nbReaderWaiting > 0 || m_nbWriterWaiting > 0 )
            m_cond.notify_all();
    }

    void acquire_priority_access()
    {
        auto &tids = m_priorityAccessOwners;
        auto tid = compat::this_thread::get_id();

        std::unique_lock<compat::Mutex> lock( m_lock );

        /* The priority access is not recursive */
        assert( !has_priority( tid ) );

        tids.push_back( tid );
    }

    void release_priority_access()
    {
        auto &tids = m_priorityAccessOwners;
        auto tid = compat::this_thread::get_id();

        std::unique_lock<compat::Mutex> lock( m_lock );

        auto end = std::remove( tids.begin(), tids.end(), tid );
        /* The tid must be in the list */
        assert( end != tids.end() );
        tids.erase( end, tids.end() );

        if ( tids.empty() )
            m_cond.notify_all();
    }

private:
    bool has_priority( compat::Thread::id tid ) const
    {
        auto &tids = m_priorityAccessOwners;
        return std::find( tids.cbegin(), tids.cend(), tid ) != tids.cend();
    }

    bool must_give_way( compat::Thread::id tid ) const
    {
        return !m_priorityAccessOwners.empty() && !has_priority( tid );
    }

    compat::ConditionVariable m_cond;
    compat::Mutex m_lock;
    unsigned int m_nbReader = 0;
    unsigned int m_nbReaderWaiting = 0;
    bool m_writing = false;
    unsigned int m_nbWriterWaiting = 0;

    /* If there is at least one thread with priority access, then all new lock
     * requests from threads without priority access are blocked. However, they
     * can continue executing their current requests following the traditional
     * RW-lock rules. */
    std::vector<compat::Thread::id> m_priorityAccessOwners;
};

class WriteLocker
{
public:
    WriteLocker( SWMRLock& l )
        : m_lock( l )
    {
    }

    void lock()
    {
        m_lock.lock_write();
    }

    void unlock()
    {
        m_lock.unlock_write();
    }

private:
    SWMRLock& m_lock;
};

class ReadLocker
{
public:
    ReadLocker( SWMRLock& l )
        : m_lock( l )
    {
    }

    void lock()
    {
        m_lock.lock_read();
    }

    void unlock()
    {
        m_lock.unlock_read();
    }

private:
    SWMRLock& m_lock;
};

class PriorityLocker
{
public:
    PriorityLocker( SWMRLock& l )
        : m_lock( l )
    {
    }

    void lock()
    {
        m_lock.acquire_priority_access();
    }

    void unlock()
    {
        m_lock.release_priority_access();
    }

private:
    SWMRLock& m_lock;
};

}

}
