#ifndef CACHE_H
#define CACHE_H

#include <cassert>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "SqliteTools.h"

template <typename TYPE>
class PrimaryKeyCacheKeyPolicy
{
    public:
        typedef unsigned int KeyType;
        static unsigned int key( const std::shared_ptr<TYPE>& self )
        {
            return self->id();
        }
        static unsigned int key( sqlite3_stmt* stmt )
        {
            return Traits<unsigned int>::Load( stmt, 0 );
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
 *   One that takes a sqlite3_stmt and one that takes an instance of the type being cached
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
template <typename IMPL, typename INTF, typename TABLEPOLICY, typename CACHEPOLICY = PrimaryKeyCacheKeyPolicy<IMPL> >
class Cache
{
    public:
        static std::shared_ptr<IMPL> fetch( sqlite3* dbConnection, const typename CACHEPOLICY::KeyType& key )
        {
            std::lock_guard<std::mutex> lock( Mutex );
            auto it = Store.find( key );
            if ( it != Store.end() )
                return it->second;
            static const std::string req = "SELECT * FROM " + TABLEPOLICY::Name +
                            " WHERE " + TABLEPOLICY::CacheColumn + " = ?";
            auto res = SqliteTools::fetchOne<IMPL>( dbConnection, req.c_str(), key );
            Store[key] = res;
            return res;
        }

        /*
         * Will fetch all elements from the database & cache them.
         *
         * @param res   A reference to the result vector. All existing elements will
         *              be discarded.
         */
        static bool fetchAll( sqlite3* dbConnection, std::vector<std::shared_ptr<INTF> >& res )
        {
            static const std::string req = "SELECT * FROM " + TABLEPOLICY::Name;
            return SqliteTools::fetchAll<IMPL, INTF>( dbConnection, req.c_str(), res );
        }

        static std::shared_ptr<IMPL> load( sqlite3* dbConnection, sqlite3_stmt* stmt )
        {
            auto cacheKey = CACHEPOLICY::key( stmt );

            std::lock_guard<std::mutex> lock( Mutex );
            auto it = Store.find( cacheKey );
            if ( it != Store.end() )
                return it->second;
            auto inst = std::make_shared<IMPL>( dbConnection, stmt );
            Store[cacheKey] = inst;
            return inst;
        }

        static bool destroy( sqlite3* dbConnection, const typename CACHEPOLICY::KeyType& key )
        {
            std::lock_guard<std::mutex> lock( Mutex );
            auto it = Store.find( key );
            if ( it != Store.end() )
                Store.erase( it );
            static const std::string req = "DELETE FROM " + TABLEPOLICY::Name + " WHERE " +
                    TABLEPOLICY::CacheColumn + " = ?";
            return SqliteTools::executeDelete( dbConnection, req.c_str(), key );
        }

        static bool destroy( sqlite3* dbConnection, const std::shared_ptr<IMPL>& self )
        {
            const auto& key = CACHEPOLICY::key( self );
            return destroy( dbConnection, key );
        }

        static void clear()
        {
            std::lock_guard<std::mutex> lock( Mutex );
            Store.clear();
        }

    protected:
        /*
         * Create a new instance of the cache class.
         */
        template <typename... Args>
        static bool insert( sqlite3* dbConnection, std::shared_ptr<IMPL> self, const char* req, const Args&... args )
        {
            unsigned int pKey = SqliteTools::insert( dbConnection, req, args... );
            if ( pKey == 0 )
                return false;
            (self.get())->*TABLEPOLICY::PrimaryKey = pKey;
            auto cacheKey = CACHEPOLICY::key( self );

            std::lock_guard<std::mutex> lock( Mutex );
            // We expect the cache column to be PRIMARY KEY / UNIQUE, so an insertion with
            // a duplicated key should have been rejected by sqlite. This indicates an invalid state
            assert( Store.find( cacheKey ) == Store.end() );
            Store[cacheKey] = self;
            return true;
        }

    private:
        static std::unordered_map<typename CACHEPOLICY::KeyType, std::shared_ptr<IMPL> > Store;
        static std::mutex Mutex;
};

template <typename IMPL, typename INTF, typename TABLEPOLICY, typename CACHEPOLICY>
std::unordered_map<typename CACHEPOLICY::KeyType, std::shared_ptr<IMPL> >
Cache<IMPL, INTF, TABLEPOLICY, CACHEPOLICY>::Store;

template <typename IMPL, typename INTF, typename TABLEPOLICY, typename CACHEPOLICY>
std::mutex Cache<IMPL, INTF, TABLEPOLICY, CACHEPOLICY>::Mutex;

#endif // CACHE_H
