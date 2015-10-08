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
        static unsigned int key( const TYPE* self )
        {
            return self->id();
        }
        static unsigned int key( sqlite3_stmt* stmt )
        {
            return sqlite::Traits<unsigned int>::Load( stmt, 0 );
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
         *
         */
        static std::vector<std::shared_ptr<INTF>> fetchAll( DBConnection dbConnection )
        {
            static const std::string req = "SELECT * FROM " + TABLEPOLICY::Name;
            return sqlite::Tools::fetchAll<IMPL, INTF>( dbConnection, req );
        }

        template <typename... Args>
        static std::vector<std::shared_ptr<INTF>> fetchAll( DBConnection dbConnection, const std::string &req, Args&&... args )
        {
            return sqlite::Tools::fetchAll<IMPL, INTF>( dbConnection, req, std::forward<Args>( args )... );
        }

        static std::shared_ptr<IMPL> load( DBConnection dbConnection, sqlite3_stmt* stmt )
        {
            auto cacheKey = CACHEPOLICY::key( stmt );

            Lock lock( Mutex );
            auto it = Store.find( cacheKey );
            if ( it != Store.end() )
                return it->second;
            auto inst = std::make_shared<IMPL>( dbConnection, stmt );
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

        static bool destroy( DBConnection dbConnection, const std::shared_ptr<IMPL>& self )
        {
            const auto& key = CACHEPOLICY::key( self );
            return destroy( dbConnection, key );
        }

        static bool destroy( DBConnection dbConnection, const IMPL* self )
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
        static bool discard( const std::shared_ptr<IMPL>& record )
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
            unsigned int pKey = sqlite::Tools::insert( dbConnection, req, std::forward<Args>( args )... );
            if ( pKey == 0 )
                return false;
            (self.get())->*TABLEPOLICY::PrimaryKey = pKey;
            auto cacheKey = CACHEPOLICY::key( self );

            Lock lock( Mutex );
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

template <typename IMPL, typename INTF, typename TABLEPOLICY, typename CACHEPOLICY>
std::unordered_map<typename CACHEPOLICY::KeyType, std::shared_ptr<IMPL> >
Cache<IMPL, INTF, TABLEPOLICY, CACHEPOLICY>::Store;

template <typename IMPL, typename INTF, typename TABLEPOLICY, typename CACHEPOLICY>
std::recursive_mutex Cache<IMPL, INTF, TABLEPOLICY, CACHEPOLICY>::Mutex;

#endif // CACHE_H
