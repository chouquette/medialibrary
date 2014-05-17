#ifndef CACHE_H
#define CACHE_H

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
 * It has a few assumptions about class that will use it:
 * - The class is stored in database and has a dedicated type.
 * - The class has a static std::string member named TableName, that can be used to generate
 *   requests
 * - The class has a static std::string member named CacheColumn, that holds the name of
 *   the column that will act as a primary key in the cache.
 * - The class has a public method of signature "unsigned int id()" that can be used to
 *   fetch the primary key
 * - The class has 2 contructors:
 *      - One that takes a sqlite3* representing the active DB connection, and
 *        a sqlite3_stmt* that holds the row currently being fetched. This CTOR is used by
 *        the load method, when fetching an existing record which isn't cached yet.
 *      - One that can be used to instantiate a new record, without parameter restriction
 *
 * Template parameter:
 * - IMPL: The actual implementation. Typically the type that inherits this class
 * - INTF: An interface that express the type. This is used when fetching containers, as they
 *         are not polymorphic.
 * - CACHECOLUMNTYPE: The type of the column representing a record in a unique way. Quite
 *                    likely the primary key type
 * - CACHECOLUMNINDEX: The index of the column used by the cache, to be able to fetch it
 *                     from a sqlite3_stmt containing a row of the stored entity type.
 *
 * How to use it:
 * - Inherit this class and specify the template parameter accordingly
 * - Make this class a friend class of the class you inherit from
 * - Implement the 2 static strings defined above.
 */
template <typename IMPL, typename INTF, typename TABLEPOLICY, typename CACHEPOLICY = PrimaryKeyCacheKeyPolicy<IMPL>>
class Cache
{
    public:
        static std::shared_ptr<IMPL> fetch( sqlite3* dbConnection, const typename CACHEPOLICY::KeyType& key )
        {
            std::lock_guard<std::mutex> lock( Mutex );
            auto it = Store.find( key );
            if ( it != Store.end() )
                return it->second;
            std::string req = "SELECT * FROM" + TABLEPOLICY::Name +
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
        static bool fetchAll( sqlite3* dbConnection, std::vector<std::shared_ptr<INTF>>& res )
        {
            std::string req = "SELECT * FROM " + TABLEPOLICY::Name;
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

        /*
         * Create a new instance of the cache class.
         * This doesn't check for the existence of a similar entity already cached.
         */
        template <typename... Args>
        static std::shared_ptr<IMPL> create( Args&&... args )
        {
            auto res = std::make_shared<IMPL>( std::forward<Args>( args )... );

            typename CACHEPOLICY::KeyType cacheKey = CACHEPOLICY::key( res );

            std::lock_guard<std::mutex> lock( Mutex );
            Store[cacheKey] = res;
            return res;
        }

    private:
        static std::unordered_map<typename CACHEPOLICY::KeyType, std::shared_ptr<IMPL>> Store;
        static std::mutex Mutex;
};

template <typename IMPL, typename INTF, typename TABLEPOLICY, typename CACHEPOLICY>
std::unordered_map<typename CACHEPOLICY::KeyType, std::shared_ptr<IMPL>>
Cache<IMPL, INTF, TABLEPOLICY, CACHEPOLICY>::Store;

template <typename IMPL, typename INTF, typename TABLEPOLICY, typename CACHEPOLICY>
std::mutex Cache<IMPL, INTF, TABLEPOLICY, CACHEPOLICY>::Mutex;

#endif // CACHE_H
