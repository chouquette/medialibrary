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


/**
 * This utility class eases up the implementation of caching.
 *
 * It is driven by 2 policy classes:
 * - TABLEPOLICY describes the basics required to fetch a record. It has 2 static fields:
 *      - Name: the table name
 *      - CacheColumn: The column used to cache records.
 * - CACHEPOLICY describes which column to use for caching by providing two "key" methods.
 *   One that takes a sqlite::Row and one that takes an instance of the type being cached
 *
 * The default CACHEPOLICY implementation bases itself on an unsigned int column, assumed
 * to be the primary key, at index 0 of a fetch statement.
 *
 * Other template parameter:
 * - IMPL: The actual implementation. Typically the type that inherits this class
 * - INTF: An interface that express the type. This is used when fetching containers, as they
 *         are not polymorphic.
 *
 * How to use it:
 * - Inherit this class and specify the template parameter & policies accordingly
 * - Make this class a friend class of the class you inherit from
 */
template <typename IMPL, typename TABLEPOLICY, typename CACHEPOLICY = PrimaryKeyCacheKeyPolicy >
class Table
{
    using _Cache = Cache<IMPL, CACHEPOLICY>;

    public:
        static std::shared_ptr<IMPL> fetch( DBConnection dbConnection, const typename CACHEPOLICY::KeyType& key )
        {
            auto l = _Cache::lock();

            auto res = _Cache::load( key );
            if ( res != nullptr )
                return res;
            static const std::string req = "SELECT * FROM " + TABLEPOLICY::Name +
                            " WHERE " + TABLEPOLICY::CacheColumn + " = ?";
            res = sqlite::Tools::fetchOne<IMPL>( dbConnection, req, key );
            if ( res == nullptr )
                return nullptr;
            _Cache::store( res );
            return res;
        }

        template <typename... Args>
        static std::shared_ptr<IMPL> fetchOne( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            return sqlite::Tools::fetchOne<IMPL>( dbConnection, req, std::forward<Args>( args )... );
        }

        /*
         * Will fetch all elements from the database & cache them.
         */
        template <typename INTF = IMPL>
        static std::vector<std::shared_ptr<INTF>> fetchAll( DBConnection dbConnection )
        {
            static const std::string req = "SELECT * FROM " + TABLEPOLICY::Name;
            // Lock the cache mutex before attempting to acquire a context, otherwise
            // we could have a thread locking cache then DB, and a thread locking DB then cache
            auto l = _Cache::lock();
            return sqlite::Tools::fetchAll<IMPL, INTF>( dbConnection, req );
        }

        template <typename INTF, typename... Args>
        static std::vector<std::shared_ptr<INTF>> fetchAll( DBConnection dbConnection, const std::string &req, Args&&... args )
        {
            auto l = _Cache::lock();
            return sqlite::Tools::fetchAll<IMPL, INTF>( dbConnection, req, std::forward<Args>( args )... );
        }

        static std::shared_ptr<IMPL> load( DBConnection dbConnection, sqlite::Row& row )
        {
            auto l = _Cache::lock();

            auto res = _Cache::load( row );
            if ( res != nullptr )
                return res;
            res = std::make_shared<IMPL>( dbConnection, row );
            _Cache::store( res );
            return res;
        }

        static bool destroy( DBConnection dbConnection, const typename CACHEPOLICY::KeyType& key )
        {
            auto l = _Cache::lock();
            _Cache::discard( key );
            static const std::string req = "DELETE FROM " + TABLEPOLICY::Name + " WHERE " +
                    TABLEPOLICY::CacheColumn + " = ?";
            return sqlite::Tools::executeDelete( dbConnection, req, key );
        }

        template <typename T>
        static bool destroy( DBConnection dbConnection, const T* self )
        {
            const auto& key = CACHEPOLICY::key( self );
            return destroy( dbConnection, key );
        }

        static void clear()
        {
            auto l = _Cache::lock();
            _Cache::clear();
        }

    protected:
        /*
         * Create a new instance of the cache class.
         */
        template <typename... Args>
        static bool insert( DBConnection dbConnection, std::shared_ptr<IMPL> self, const std::string& req, Args&&... args )
        {
            auto l = _Cache::lock();

            unsigned int pKey = sqlite::Tools::insert( dbConnection, req, std::forward<Args>( args )... );
            if ( pKey == 0 )
                return false;
            (self.get())->*TABLEPOLICY::PrimaryKey = pKey;
            _Cache::store( self );
            return true;
        }
};

#endif // CACHE_H
