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

#include "compat/ConditionVariable.h"
#include "compat/Mutex.h"

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
    SWMRLock()
        : m_nbReader( 0 )
        , m_nbReaderWaiting( 0 )
        , m_writing( false )
        , m_nbWriterWaiting( 0 )
    {
    }

    void lock_read()
    {
        std::unique_lock<compat::Mutex> lock( m_lock );
        ++m_nbReaderWaiting;
        m_writeDoneCond.wait( lock, [this](){
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
            m_writeDoneCond.notify_one();
    }

    void lock_write()
    {
        std::unique_lock<compat::Mutex> lock( m_lock );
        ++m_nbWriterWaiting;
        m_writeDoneCond.wait( lock, [this](){
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
            m_writeDoneCond.notify_all();
    }

private:
    compat::ConditionVariable m_writeDoneCond;
    compat::Mutex m_lock;
    unsigned int m_nbReader;
    unsigned int m_nbReaderWaiting;
    bool m_writing;
    unsigned int m_nbWriterWaiting;
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

}

}
