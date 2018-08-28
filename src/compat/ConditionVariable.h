/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2016 Hugo Beauzée-Luyssen, Videolabs
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

// Unconditionaly include <mutex> for std::unique_lock
#include <mutex>
#include "compat/Mutex.h"

#if CXX11_THREADS

#include <condition_variable>

namespace medialibrary
{
namespace compat
{
    using ConditionVariable = std::condition_variable;
}
}

#else

#include <windows.h>

namespace medialibrary
{
namespace compat
{
class ConditionVariable
{
public:
    using native_handle_type = PCONDITION_VARIABLE;
    ConditionVariable()
    {
        InitializeConditionVariable( &m_cond );
    }

    void notify_one() noexcept
    {
        WakeConditionVariable( &m_cond );
    }

    void notify_all() noexcept
    {
        WakeAllConditionVariable( &m_cond );
    }

    void wait( std::unique_lock<Mutex>& lock )
    {
        SleepConditionVariableCS( &m_cond, lock.mutex()->native_handle(), INFINITE );
    }

    template <typename Pred>
    void wait( std::unique_lock<Mutex>& lock, Pred pred )
    {
        while ( pred() == false )
            wait( lock );
    }

    template <typename Rep, typename Period, typename Pred>
    bool wait_for( std::unique_lock<Mutex>& lock, const std::chrono::duration<Rep, Period>& relTime, Pred pred )
    {
        auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>( relTime );
        while ( pred() == false )
        {
            auto now = std::chrono::system_clock::now();
            if ( SleepConditionVariableCS( &m_cond, lock.mutex()->native_handle(), timeout.count() ) == 0 )
            {
                auto res = GetLastError();
                if ( res == ERROR_TIMEOUT )
                    return false;
                throw std::system_error{ std::make_error_code( std::errc::resource_unavailable_try_again ) };
            }
            timeout -= std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() - now );
        }
        return true;
    }

    template <typename Clock, typename Period, typename Pred>
    bool wait_until( std::unique_lock<Mutex>& lock, const std::chrono::time_point<Clock, Period>& relTime, Pred&& pred )
    {
        auto timeout = relTime - std::chrono::steady_clock::now();
        return wait_for( lock, timeout, std::forward<Pred>( pred ) );
    }

    native_handle_type native_handle()
    {
        return &m_cond;
    }

private:
    CONDITION_VARIABLE m_cond;
};
}
}

#endif
