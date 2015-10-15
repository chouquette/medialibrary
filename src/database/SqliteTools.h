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

#ifndef SQLITETOOLS_H
#define SQLITETOOLS_H

#include <cassert>
#include <cstring>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>
#include <thread>

#include "Types.h"
#include "database/SqliteConnection.h"
#include "logging/Logger.h"

namespace sqlite
{

struct ForeignKey
{
    constexpr ForeignKey(unsigned int v) : value(v) {}
    unsigned int value;
};

template <typename ToCheck, typename T>
using IsSameDecay = std::is_same<typename std::decay<ToCheck>::type, T>;

template <typename T, typename Enable = void>
struct Traits;

template <typename T>
struct Traits<T, typename std::enable_if<
        std::is_integral<typename std::decay<T>::type>::value
        && ! IsSameDecay<T, int64_t>::value
    >::type>
{
    static constexpr
    int (*Bind)(sqlite3_stmt *, int, int) = &sqlite3_bind_int;

    static constexpr
    int (*Load)(sqlite3_stmt *, int) = &sqlite3_column_int;
};

template <typename T>
struct Traits<T, typename std::enable_if<IsSameDecay<T, ForeignKey>::value>::type>
{
    static int Bind( sqlite3_stmt *stmt, int pos, ForeignKey fk)
    {
        if ( fk.value != 0 )
            return Traits<unsigned int>::Bind( stmt, pos, fk.value );
        return sqlite3_bind_null( stmt, pos );
    }
};

template <typename T>
struct Traits<T, typename std::enable_if<IsSameDecay<T, std::string>::value>::type>
{
    static int Bind(sqlite3_stmt* stmt, int pos, const std::string& value )
    {
        return sqlite3_bind_text( stmt, pos, value.c_str(), -1, SQLITE_STATIC );
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
struct Traits<T, typename std::enable_if<std::is_floating_point<
        typename std::decay<T>::type
    >::value>::type>
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

template <typename T>
struct Traits<T, typename std::enable_if<IsSameDecay<T, int64_t>::value>::type>
{
    static constexpr int
    (*Bind)(sqlite3_stmt *, int, sqlite_int64) = &sqlite3_bind_int64;

    static constexpr sqlite_int64
    (*Load)(sqlite3_stmt *, int) = &sqlite3_column_int64;
};

class Tools
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
        static std::vector<std::shared_ptr<INTF> > fetchAll( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            auto ctx = dbConnection->acquireContext();

            std::vector<std::shared_ptr<INTF>> results;
            auto stmt = prepareRequest( dbConnection, req, std::forward<Args>( args )...);
            if ( stmt == nullptr )
                throw std::runtime_error("Failed to execute SQlite request");
            int res = sqlite3_step( stmt.get() );
            while ( res == SQLITE_ROW )
            {
                auto row = IMPL::load( dbConnection, stmt.get() );
                results.push_back( row );
                res = sqlite3_step( stmt.get() );
            }
            return results;
        }

        template <typename T, typename... Args>
        static std::shared_ptr<T> fetchOne( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            auto ctx = dbConnection->acquireContext();

            auto stmt = prepareRequest( dbConnection, req, std::forward<Args>( args )... );
            if ( stmt == nullptr )
                return nullptr;
            int res = sqlite3_step( stmt.get() );
            if ( res != SQLITE_ROW )
                return nullptr;
            return T::load( dbConnection, stmt.get() );
        }

        template <typename... Args>
        static bool hasResults( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            auto ctx = dbConnection->acquireContext();

            auto stmt = prepareRequest( dbConnection, req, std::forward<Args>( args )... );
            if ( stmt == nullptr )
                return false;
            int res = sqlite3_step( stmt.get() );
            if ( res != SQLITE_ROW )
                return false;
            return true;
        }

        template <typename... Args>
        static bool executeRequest( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            auto ctx = dbConnection->acquireContext();
            return executeRequestLocked( dbConnection, req, std::forward<Args>( args )... );
        }

        template <typename... Args>
        static bool executeDelete( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            auto ctx = dbConnection->acquireContext();
            if ( executeRequestLocked( dbConnection, req, std::forward<Args>( args )... ) == false )
                return false;
            return sqlite3_changes( dbConnection->getConn() ) > 0;
        }

        template <typename... Args>
        static bool executeUpdate( DBConnection dbConnectionWeak, const std::string& req, Args&&... args )
        {
            // The code would be exactly the same, do not freak out because it calls executeDelete :)
            return executeDelete( dbConnectionWeak, req, std::forward<Args>( args )... );
        }

        /**
         * Inserts a record to the DB and return the newly created primary key.
         * Returns 0 (which is an invalid sqlite primary key) when insertion fails.
         */
        template <typename... Args>
        static unsigned int insert( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            auto ctx = dbConnection->acquireContext();
            if ( executeRequestLocked( dbConnection, req, std::forward<Args>( args )... ) == false )
                return 0;
            return sqlite3_last_insert_rowid( dbConnection->getConn() );
        }

    private:
        template <typename... Args>
        static bool executeRequestLocked( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            auto stmt = prepareRequest( dbConnection, req, std::forward<Args>( args )... );
            if ( stmt == nullptr )
                return false;
            int res;
            do
            {
                res = sqlite3_step( stmt.get() );
            } while ( res == SQLITE_ROW );
            if ( res != SQLITE_DONE )
            {
#if SQLITE_VERSION_NUMBER >= 3007015
                auto err = sqlite3_errstr( res );
#else
                auto err = res;
#endif
                LOG_ERROR( "Failed to execute <", req, ">\nInvalid result: ", err,
                          ": ", sqlite3_errmsg( dbConnection->getConn() ) );
                return false;
            }
            return true;
        }

        template <typename... Args>
        static StmtPtr prepareRequest( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            return _prepareRequest<1>( dbConnection, req, std::forward<Args>( args )... );
        }

        template <int>
        static StmtPtr _prepareRequest( DBConnection dbConnection, const std::string& req )
        {
            sqlite3_stmt* stmt = nullptr;
            int res = sqlite3_prepare_v2( dbConnection->getConn(), req.c_str(), -1, &stmt, NULL );
            if ( res != SQLITE_OK )
            {
                LOG_ERROR( "Failed to execute request: ", req, ' ',
                            sqlite3_errmsg( dbConnection->getConn() ) );
            }
            return StmtPtr( stmt, &sqlite3_finalize );
        }

        template <int COLIDX, typename T, typename... Args>
        static StmtPtr _prepareRequest( DBConnection dbConnection, const std::string& req, T&& arg, Args&&... args )
        {
            auto stmt = _prepareRequest<COLIDX + 1>( dbConnection, req, std::forward<Args>( args )... );
            Traits<T>::Bind( stmt.get(), COLIDX, std::forward<T>( arg ) );
            return stmt;
        }

        friend SqliteConnection;
};

}

#endif // SQLITETOOLS_H
