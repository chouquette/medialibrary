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

#include "config.h"

#if CXX11_THREADS && !defined(__ANDROID__)
#include <thread>
namespace medialibrary
{
namespace compat
{
using Thread = std::thread;
namespace this_thread = std::this_thread;
}
}

#else

#include <functional>
#include <memory>
#include <pthread.h>
#include <system_error>
#include <unistd.h>

namespace medialibrary
{
namespace compat
{

// temporary forward declarations, so we can use thread_id from this_thread::get_id
// and have this_thread::get_id declared before thread_id itself, in order to friend it.
namespace details { class thread_id; }
namespace this_thread { details::thread_id get_id(); }
class Thread;

namespace details
{

class thread_id
{
public:
    constexpr thread_id() noexcept : m_id( 0 ) {}
    thread_id( const thread_id& ) = default;
    thread_id& operator=( const thread_id& ) = default;
    thread_id( thread_id&& r ) : m_id( r.m_id ) { r.m_id = 0; }
    thread_id& operator=( thread_id&& r ) { m_id = r.m_id; r.m_id = 0; return *this; }

    bool operator==(const thread_id& r) const noexcept { return m_id == r.m_id; }
    bool operator!=(const thread_id& r) const noexcept { return m_id != r.m_id; }
    bool operator< (const thread_id& r) const noexcept { return m_id <  r.m_id; }
    bool operator<=(const thread_id& r) const noexcept { return m_id <= r.m_id; }
    bool operator> (const thread_id& r) const noexcept { return m_id >  r.m_id; }
    bool operator>=(const thread_id& r) const noexcept { return m_id >= r.m_id; }

private:
    thread_id( pthread_t id ) : m_id( id ) {}

    pthread_t m_id;

    friend thread_id this_thread::get_id();
    friend Thread;
    friend std::hash<thread_id>;
};
}

// Compatibility thread class, for platforms that don't implement C++11 (or do it incorrectly)
// This handles the very basic usage we need for the medialibrary
class Thread
{
    template <typename T>
    struct Invoker
    {
        void (T::*func)();
        T* inst;
    };

public:
    using id = details::thread_id;

    template <typename T>
    Thread( void (T::*entryPoint)(), T* inst )
    {
        auto i = std::unique_ptr<Invoker<T>>( new Invoker<T>{
            entryPoint, inst
        });
        if ( pthread_create( &m_id.m_id, nullptr, []( void* opaque ) -> void* {
            auto invoker = std::unique_ptr<Invoker<T>>( reinterpret_cast<Invoker<T>*>( opaque ) );
            (invoker->inst->*(invoker->func))();
            return nullptr;
        }, i.get() ) != 0 )
            throw std::system_error{ std::make_error_code( std::errc::resource_unavailable_try_again ) };
        i.release();
    }

    static unsigned hardware_concurrency()
    {
        return sysconf( _SC_NPROCESSORS_ONLN );
    }

    void join()
    {
        if ( !joinable() )
            throw std::system_error{ std::make_error_code( std::errc::invalid_argument ) };
        if ( this_thread::get_id() == m_id )
            throw std::system_error{ std::make_error_code( std::errc::resource_deadlock_would_occur ) };
        pthread_join( m_id.m_id, nullptr );
    }

    // Platform agnostic methods:

    Thread() = default;
    Thread( Thread&& ) = default;
    Thread& operator=( Thread&& ) = default;
    Thread( const Thread& ) = delete;
    Thread& operator=( const Thread& ) = delete;

    bool joinable()
    {
        return m_id != id{};
    }

    id get_id()
    {
        return m_id;
    }

private:
    id m_id;
};

namespace this_thread
{
    inline details::thread_id get_id()
    {
        return { pthread_self() };
    }
}

}
}

namespace std
{
template <>
struct hash<medialibrary::compat::Thread::id>
{
    size_t operator()( const medialibrary::compat::Thread::id& id ) const noexcept
    {
        static_assert( sizeof( id.m_id <= sizeof(size_t) ), "pthread_t is too big" );
        return id.m_id;
    }
};
}

#endif
