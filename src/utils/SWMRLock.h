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
    void lock_read();
    void unlock_read();

    void lock_write();
    void unlock_write();

    void acquire_priority_access();
    void release_priority_access();

private:
    bool has_priority( compat::Thread::id tid ) const;
    bool must_give_way( compat::Thread::id tid ) const;

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
