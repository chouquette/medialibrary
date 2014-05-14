#ifndef SQLITETOOLS_H
#define SQLITETOOLS_H

#include <cstring>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>

template <typename T>
struct TypeHelper
{
    static const T Default = {};
};

// Have a base case for integral type only
// Use specialization to define other cases, and fail for the rest.
template <typename T>
struct Traits
{
    static constexpr
    typename std::enable_if<std::is_integral<T>::value, int>::type
    (*Bind)(sqlite3_stmt *, int, int) = &sqlite3_bind_int;
};

template <>
struct Traits<std::string>
{
    static int Bind(sqlite3_stmt* stmt, int pos, const std::string& value )
    {
        char* tmp = strdup( value.c_str() );
        sqlite3_bind_text( stmt, pos, tmp, -1, free );
    }
};

class SqliteTools
{
    public:
        static bool createTable(sqlite3* db, const char* request );

        template <typename T, typename U, typename KEYTYPE>
        static bool fetchAll( sqlite3* dbConnection, const char* req, const KEYTYPE& foreignKey, std::vector<U*>*& results)
        {
            results = new std::vector<U*>;
            sqlite3_stmt* stmt;
            int res = sqlite3_prepare_v2( dbConnection, req, -1, &stmt, NULL );
            if ( res != SQLITE_OK )
            {
                std::cerr << "Failed to execute request: " << req << std::endl;
                std::cerr << sqlite3_errmsg( dbConnection ) << std::endl;
                return false;
            }
            if ( foreignKey != TypeHelper<KEYTYPE>::Default )
                Traits<KEYTYPE>::Bind( stmt, 1, foreignKey );
            res = sqlite3_step( stmt );
            while ( res == SQLITE_ROW )
            {
                U* l = new T( dbConnection, stmt );
                results->push_back( l );
                res = sqlite3_step( stmt );
            }
            sqlite3_finalize( stmt );
            return true;
        }

        template <typename T, typename U>
        static bool fetchAll( sqlite3* dbConnection, const char* req, std::vector<U*>*& results)
        {
            return fetchAll<T, U>( dbConnection, req, 0, results );
        }

        template <typename T, typename KEYTYPE>
        static T* fetchOne( sqlite3* dbConnection, const char* req, const KEYTYPE& primaryKey )
        {
            T* result = NULL;
            sqlite3_stmt *stmt;
            int res = sqlite3_prepare_v2( dbConnection, req, -1, &stmt, NULL );
            if ( res != SQLITE_OK )
            {
                std::cerr << "Failed to execute request: " << req << std::endl;
                std::cerr << sqlite3_errmsg( dbConnection ) << std::endl;
                return result;
            }
            Traits<KEYTYPE>::Bind( stmt, 1, primaryKey );
            if ( sqlite3_step( stmt ) != SQLITE_ROW )
                return result;
            result = new T( dbConnection, stmt );
            sqlite3_finalize( stmt );
            return result;
        }


};

#endif // SQLITETOOLS_H
