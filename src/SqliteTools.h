#ifndef SQLITETOOLS_H
#define SQLITETOOLS_H

#include <cassert>
#include <cstring>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>

#include "Types.h"

template <typename T, typename Enable = void>
struct Traits;

template <typename T>
struct Traits<T, typename std::enable_if<std::is_integral<T>::value>::type>
{
    static constexpr
    int (*Bind)(sqlite3_stmt *, int, int) = &sqlite3_bind_int;

    static constexpr
    int (*Load)(sqlite3_stmt *, int) = &sqlite3_column_int;
};

template <>
struct Traits<std::string>
{
    static int Bind(sqlite3_stmt* stmt, int pos, const std::string& value )
    {
        char* tmp = strdup( value.c_str() );
        return sqlite3_bind_text( stmt, pos, tmp, -1, free );
    }

    static std::string Load( sqlite3_stmt* stmt, int pos )
    {
        auto tmp = (const char*)sqlite3_column_text( stmt, pos );
        if ( tmp != nullptr )
            return std::string( tmp );
        return std::string();
    }
};

template <typename T>
struct Traits<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
{
        static constexpr int
        (*Bind)(sqlite3_stmt *, int, double) = &sqlite3_bind_double;

        static constexpr double
        (*Load)(sqlite3_stmt *, int) = &sqlite3_column_double;
};

template <>
struct Traits<std::nullptr_t>
{
    static int Bind(sqlite3_stmt* stmt, int idx, std::nullptr_t)
    {
        return sqlite3_bind_null( stmt, idx );
    }
};

class SqliteTools
{
    private:
        typedef std::unique_ptr<sqlite3_stmt, int (*)(sqlite3_stmt*)> StmtPtr;
    public:
        /**
         * Will fetch all records of type IMPL and return them as a shared_ptr to INTF
         * This WILL add all fetched records to the cache
         *
         * @param results   A reference to the result vector. All existing elements will
         *                  be discarded.
         */
        template <typename IMPL, typename INTF, typename... Args>
        static bool fetchAll( DBConnection dbConnectionWeak, const std::string& req, std::vector<std::shared_ptr<INTF> >& results, const Args&... args )
        {
            auto dbConnection = dbConnectionWeak.lock();
            if ( dbConnection == nullptr )
                return false;

            results.clear();
            auto stmt = prepareRequest( dbConnection, req, args...);
            if ( stmt == nullptr )
                return false;
            int res = sqlite3_step( stmt.get() );
            while ( res == SQLITE_ROW )
            {
                auto row = IMPL::load( dbConnection, stmt.get() );
                results.push_back( row );
                res = sqlite3_step( stmt.get() );
            }
            return true;
        }

        template <typename T, typename... Args>
        static std::shared_ptr<T> fetchOne( DBConnection dbConnectionWeak, const std::string& req, const Args&... args )
        {
            auto dbConnection = dbConnectionWeak.lock();
            if ( dbConnection == nullptr )
                return nullptr;

            std::shared_ptr<T> result;
            auto stmt = prepareRequest( dbConnection, req, args... );
            if ( stmt == nullptr )
                return nullptr;
            if ( sqlite3_step( stmt.get() ) != SQLITE_ROW )
                return result;
            result = T::load( dbConnection, stmt.get() );
            return result;
        }

        template <typename... Args>
        static bool executeDelete( DBConnection dbConnectionWeak, const std::string& req, const Args&... args )
        {
            auto dbConnection = dbConnectionWeak.lock();
            if ( dbConnection == nullptr )
                return false;
            if ( executeRequest( dbConnection, req, args... ) == false )
                return false;
            return sqlite3_changes( dbConnection.get() ) > 0;
        }

        template <typename... Args>
        static bool executeUpdate( DBConnection dbConnectionWeak, const std::string& req, const Args&... args )
        {
            // The code would be exactly the same, do not freak out because it call delete :)
            return executeDelete( dbConnectionWeak, req, args... );
        }

        template <typename... Args>
        static bool executeRequest( DBConnection dbConnectionWeak, const std::string& req, const Args&... args )
        {
            auto dbConnection = dbConnectionWeak.lock();
            if ( dbConnection == nullptr )
                return false;
            return executeRequest( dbConnection, req, args... );
        }

        /**
         * Inserts a record to the DB and return the newly created primary key.
         * Returns 0 (which is an invalid sqlite primary key) when insertion fails.
         */
        template <typename... Args>
        static unsigned int insert( DBConnection dbConnectionWeak, const std::string& req, const Args&... args )
        {
            auto dbConnection = dbConnectionWeak.lock();
            if ( dbConnection == nullptr )
                return false;

            if ( executeRequest( dbConnection, req, args... ) == false )
                return 0;
            return sqlite3_last_insert_rowid( dbConnection.get() );
        }

    private:
        template <typename... Args>
        static StmtPtr prepareRequest( std::shared_ptr<sqlite3> dbConnectionPtr, const std::string& req, const Args&... args )
        {
            return _prepareRequest<1>( dbConnectionPtr, req, args... );
        }

        template <unsigned int>
        static StmtPtr _prepareRequest( std::shared_ptr<sqlite3> dbConnection, const std::string& req )
        {
            assert( dbConnection != nullptr );
            sqlite3_stmt* stmt = nullptr;
            int res = sqlite3_prepare_v2( dbConnection.get(), req.c_str(), -1, &stmt, NULL );
            if ( res != SQLITE_OK )
            {
                std::cerr << "Failed to execute request: " << req << std::endl;
                std::cerr << sqlite3_errmsg( dbConnection.get() ) << std::endl;
            }
            return StmtPtr( stmt, &sqlite3_finalize );
        }

        template <unsigned int COLIDX, typename T, typename... Args>
        static StmtPtr _prepareRequest( std::shared_ptr<sqlite3> dbConnectionPtr, const std::string& req, const T& arg, const Args&... args )
        {
            auto stmt = _prepareRequest<COLIDX + 1>( dbConnectionPtr, req, args... );
            Traits<T>::Bind( stmt.get(), COLIDX, arg );
            return stmt;
        }

        template <typename... Args>
        static bool executeRequest( std::shared_ptr<sqlite3> dbConnection, const std::string& req, const Args&... args )
        {
            auto stmt = prepareRequest( dbConnection, req, args... );
            if ( stmt == nullptr )
                return false;
            int res;
            do
            {
                res = sqlite3_step( stmt.get() );
            } while ( res == SQLITE_ROW );
            if ( res != SQLITE_DONE )
            {
                std::cerr << "Invalid result: " <<
#if SQLITE_VERSION_NUMBER >= 3007015
                            sqlite3_errstr( res )
#else
                             res
#endif
                          << ": " << sqlite3_errmsg( dbConnection.get() ) << std::endl;
                return false;
            }
            return true;
        }
};

#endif // SQLITETOOLS_H
