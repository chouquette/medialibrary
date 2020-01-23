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

// Include mutex unconditionnaly for lock_gard & unique_lock
#include <mutex>

#if CXX11_MUTEX
#include <mutex>
namespace medialibrary
{
namespace compat
{
using Mutex = std::mutex;
}
}

#else

#include <windows.h>

namespace medialibrary
{
namespace compat
{

class Mutex
{
public:
    using native_handle_type = CRITICAL_SECTION*;

    Mutex()
    {
        InitializeCriticalSection( &m_lock );
    }

    ~Mutex()
    {
        DeleteCriticalSection( &m_lock );
    }

    void lock()
    {
        EnterCriticalSection( &m_lock );
    }

    bool try_lock()
    {
        return TryEnterCriticalSection( &m_lock );
    }

    void unlock()
    {
        LeaveCriticalSection( &m_lock );
    }

    native_handle_type native_handle() noexcept
    {
        return &m_lock;
    }

private:
    CRITICAL_SECTION m_lock;
};

}
}

#endif
