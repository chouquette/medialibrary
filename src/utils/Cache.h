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

#include <mutex>
#include <cassert>

template <typename T>
class Cache
{
public:
    Cache() : m_cached( false ) {}

    bool isCached() const { return m_cached; }

    operator T() const
    {
        assert( m_cached );
        return m_value;
    }

    operator const T&() const
    {
        assert( m_cached );
        return m_value;
    }

    const T& get() const
    {
        assert( m_cached );
        return m_value;
    }

    T& get()
    {
        assert( m_cached );
        return m_value;
    }

    template <typename U>
    T& operator=( U&& value )
    {
        static_assert( std::is_convertible<typename std::decay<U>::type, typename std::decay<T>::type>::value, "Mismatching types" );
        m_value = std::forward<U>( value );
        m_cached = true;
        return m_value;
    }

    std::unique_lock<std::mutex> lock()
    {
        return std::unique_lock<std::mutex>( m_lock );
    }

private:
    T m_value;
    std::mutex m_lock;
    bool m_cached;
};
