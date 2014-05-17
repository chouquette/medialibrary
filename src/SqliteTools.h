#ifndef SQLITETOOLS_H
#define SQLITETOOLS_H

#include <cstring>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>

// Have a base case for integral type only
// Use specialization to define other cases, and fail for the rest.
template <typename T>
struct Traits
{
    static constexpr
    typename std::enable_if<std::is_integral<T>::value, int>::type
    (*Bind)(sqlite3_stmt *, int, int) = &sqlite3_bind_int;

    static constexpr
    typename std::enable_if<std::is_integral<T>::value, int>::type
    (*Load)(sqlite3_stmt *, int) = &sqlite3_column_int;
};

template <>
struct Traits<std::string>
{
    static int Bind(sqlite3_stmt* stmt, int pos, const std::string& value )
    {
        char* tmp = strdup( value.c_str() );
        return sqlite3_bind_text( stmt, pos, tmp, -1, free );
    }

    static const char* Load( sqlite3_stmt* stmt, int pos )
    {
        return (const char*)sqlite3_column_text( stmt, pos );
    }
};

class SqliteTools
{
    private:
        typedef std::unique_ptr<sqlite3_stmt, int (*)(sqlite3_stmt*)> StmtPtr;
    public:
        static bool createTable(sqlite3* db, const char* request );

        /**
         * Will fetch all records of type IMPL and return them as a shared_ptr to INTF
         * This WILL add all fetched records to the cache
         *
         * @param results   A reference to the result vector. All existing elements will
         *                  be discarded.
         */
        template <typename IMPL, typename INTF, typename... Args>
        static bool fetchAll( sqlite3* dbConnection, const char* req, std::vector<std::shared_ptr<INTF> >& results, const Args&... args )
        {
            results.clear();
            auto stmt = executeRequest( dbConnection, req, args...);
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
        static std::shared_ptr<T> fetchOne( sqlite3* dbConnection, const char* req, const Args&... args )
        {
            std::shared_ptr<T> result;
            auto stmt = executeRequest( dbConnection, req, args... );
            if ( stmt == nullptr )
                return nullptr;
            if ( sqlite3_step( stmt.get() ) != SQLITE_ROW )
                return result;
            result = T::load( dbConnection, stmt.get() );
            return result;
        }

        template <typename... Args>
        static bool destroy( sqlite3* dbConnection, const char* req, const Args&... args )
        {
            auto stmt = executeRequest( dbConnection, req, args... );
            if ( stmt == nullptr )
                return false;
            sqlite3_step( stmt.get() );
            return sqlite3_changes( dbConnection ) > 0;
        }

    private:
        template <typename... Args>
        static StmtPtr executeRequest( sqlite3* dbConnection, const char* req, const Args&... args )
        {
            return _executeRequest<1>( dbConnection, req, args... );
        }

        template <unsigned int>
        static StmtPtr _executeRequest( sqlite3* dbConnection, const char* req )
        {
            sqlite3_stmt* stmt = nullptr;
            int res = sqlite3_prepare_v2( dbConnection, req, -1, &stmt, NULL );
            if ( res != SQLITE_OK )
            {
                std::cerr << "Failed to execute request: " << req << std::endl;
                std::cerr << sqlite3_errmsg( dbConnection ) << std::endl;
            }
            return StmtPtr( stmt, &sqlite3_finalize );
        }

        template <unsigned int COLIDX, typename T, typename... Args>
        static StmtPtr _executeRequest( sqlite3* dbConnection, const char* req, const T& arg, const Args&... args )
        {
            auto stmt = _executeRequest<COLIDX + 1>( dbConnection, req, args... );
            Traits<T>::Bind( stmt.get(), COLIDX, arg );
            return stmt;
        }
};

#endif // SQLITETOOLS_H
