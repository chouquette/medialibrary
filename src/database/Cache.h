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
        static unsigned int key( sqlite::Row& row )
        {
            return row.load<unsigned int>( 0 );
        }
};

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
class Cache
{
    public:
        static std::shared_ptr<IMPL> fetch( DBConnection dbConnection, const typename CACHEPOLICY::KeyType& key )
        {
            Lock lock( Mutex );
            auto it = Store.find( key );
            if ( it != Store.end() )
                return it->second;
            static const std::string req = "SELECT * FROM " + TABLEPOLICY::Name +
                            " WHERE " + TABLEPOLICY::CacheColumn + " = ?";
            auto res = sqlite::Tools::fetchOne<IMPL>( dbConnection, req, key );
            if ( res == nullptr )
                return nullptr;
            Store[key] = res;
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
            Lock lock( Mutex );
            return sqlite::Tools::fetchAll<IMPL, INTF>( dbConnection, req );
        }

        template <typename INTF, typename... Args>
        static std::vector<std::shared_ptr<INTF>> fetchAll( DBConnection dbConnection, const std::string &req, Args&&... args )
        {
            Lock lock( Mutex );
            return sqlite::Tools::fetchAll<IMPL, INTF>( dbConnection, req, std::forward<Args>( args )... );
        }

        static std::shared_ptr<IMPL> load( DBConnection dbConnection, sqlite::Row& row )
        {
            auto cacheKey = CACHEPOLICY::key( row );

            Lock lock( Mutex );
            auto it = Store.find( cacheKey );
            if ( it != Store.end() )
                return it->second;
            auto inst = std::make_shared<IMPL>( dbConnection, row );
            Store[cacheKey] = inst;
            return inst;
        }

        static bool destroy( DBConnection dbConnection, const typename CACHEPOLICY::KeyType& key )
        {
            Lock lock( Mutex );
            auto it = Store.find( key );
            if ( it != Store.end() )
                Store.erase( it );
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
            Lock lock( Mutex );
            Store.clear();
        }

        /**
         * @brief discard Discard a record from the cache
         * @param key The key used for cache
         * @return
         */
        static bool discard( const IMPL* record )
        {
            auto key = CACHEPOLICY::key( record );
            Lock lock( Mutex );
            auto it = Store.find( key );
            if ( it == Store.end() )
                return false;
            Store.erase( it );
            return true;
        }

    protected:
        /*
         * Create a new instance of the cache class.
         */
        template <typename... Args>
        static bool insert( DBConnection dbConnection, std::shared_ptr<IMPL> self, const std::string& req, Args&&... args )
        {
            Lock lock( Mutex );

            unsigned int pKey = sqlite::Tools::insert( dbConnection, req, std::forward<Args>( args )... );
            if ( pKey == 0 )
                return false;
            (self.get())->*TABLEPOLICY::PrimaryKey = pKey;
            auto cacheKey = CACHEPOLICY::key( self.get() );

            // We expect the cache column to be PRIMARY KEY / UNIQUE, so an insertion with
            // a duplicated key should have been rejected by sqlite. This indicates an invalid state
            assert( Store.find( cacheKey ) == Store.end() );
            Store[cacheKey] = self;
            return true;
        }

    private:
        static std::unordered_map<typename CACHEPOLICY::KeyType, std::shared_ptr<IMPL> > Store;
        static std::recursive_mutex Mutex;
        typedef std::lock_guard<std::recursive_mutex> Lock;
};

template <typename IMPL, typename TABLEPOLICY, typename CACHEPOLICY>
std::unordered_map<typename CACHEPOLICY::KeyType, std::shared_ptr<IMPL> >
Cache<IMPL, TABLEPOLICY, CACHEPOLICY>::Store;

template <typename IMPL, typename TABLEPOLICY, typename CACHEPOLICY>
std::recursive_mutex Cache<IMPL, TABLEPOLICY, CACHEPOLICY>::Mutex;

#endif // CACHE_H
