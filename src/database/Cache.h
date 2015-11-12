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

#ifndef CACHE_H
#define CACHE_H

#include <cassert>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "SqliteTools.h"

class PrimaryKeyCacheKeyPolicy
{
    public:
        typedef unsigned int KeyType;

        template <typename T>
        static unsigned int key( const T* self )
        {
            return self->id();
        }
        static unsigned int key( const sqlite::Row& row )
        {
            return row.load<unsigned int>( 0 );
        }
};

template <typename IMPL, typename CACHEPOLICY>
class Cache
{
    using lock_t = std::unique_lock<std::recursive_mutex>;

public:
    static lock_t lock()
    {
        return lock_t{ Mutex };
    }

    static std::shared_ptr<IMPL> load( const typename CACHEPOLICY::KeyType& key )
    {
        auto it = Store.find( key );
        if ( it == Store.end() )
            return nullptr;
        return it->second;
    }

    template <typename T>
    static std::shared_ptr<IMPL> load( const T& value )
    {
        auto key = CACHEPOLICY::key( value );
        return load( key );
    }

    static void store( const std::shared_ptr<IMPL> value )
    {
        auto key = CACHEPOLICY::key( value.get() );
        Store[key] = value;
    }

    /**
     * @brief discard Discard a record from the cache
     */
    static bool discard( const typename CACHEPOLICY::KeyType& key )
    {
        auto it = Store.find( key );
        if ( it == Store.end() )
            return false;
        Store.erase( it );
        return true;
    }

    static void clear()
    {
        Store.clear();
    }

private:
    static std::unordered_map<typename CACHEPOLICY::KeyType, std::shared_ptr<IMPL>> Store;
    static std::recursive_mutex Mutex;
};

template <typename IMPL, typename CACHEPOLICY>
std::unordered_map<typename CACHEPOLICY::KeyType, std::shared_ptr<IMPL>>
Cache<IMPL, CACHEPOLICY>::Store;

template <typename IMPL, typename CACHEPOLICY>
std::recursive_mutex Cache<IMPL, CACHEPOLICY>::Mutex;


#endif // CACHE_H
