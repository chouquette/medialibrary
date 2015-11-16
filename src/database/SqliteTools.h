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
#include <chrono>
#include <cstring>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <thread>

#include "Types.h"
#include "database/SqliteConnection.h"
#include "database/SqliteErrors.h"
#include "database/SqliteTraits.h"
#include "database/SqliteTransaction.h"
#include "logging/Logger.h"

namespace sqlite
{

class Row
{
public:
    Row( sqlite3_stmt* stmt )
        : m_stmt( stmt )
        , m_idx( 0 )
        , m_nbColumns( sqlite3_column_count( stmt ) )
    {
    }

    Row()
        : m_stmt( nullptr )
        , m_idx( 0 )
    {
    }

    /**
     * @brief operator >> Extracts the next column from this result row.
     */
    template <typename T>
    Row& operator>>(T& t)
    {
        if ( m_idx + 1 > m_nbColumns )
            throw errors::ColumnOutOfRange( m_idx, m_nbColumns );
        t = sqlite::Traits<T>::Load( m_stmt, m_idx );
        m_idx++;
        return *this;
    }

    /**
     * @brief Returns the value in column idx, but doesn't advance to the next column
     */
    template <typename T>
    T load(unsigned int idx) const
    {
        if ( m_idx + 1 > m_nbColumns )
            throw errors::ColumnOutOfRange( m_idx, m_nbColumns );
        return sqlite::Traits<T>::Load( m_stmt, idx );
    }

    bool operator==(std::nullptr_t)
    {
        return m_stmt == nullptr;
    }

    bool operator!=(std::nullptr_t)
    {
        return m_stmt != nullptr;
    }

private:
    sqlite3_stmt* m_stmt;
    unsigned int m_idx;
    unsigned int m_nbColumns;
};

class Statement
{
public:
    Statement( DBConnection dbConnection, const std::string& req )
        : m_stmt( nullptr, &sqlite3_finalize )
        , m_dbConn( dbConnection )
        , m_req( req )
        , m_bindIdx( 0 )
    {
        sqlite3_stmt* stmt;
        int res = sqlite3_prepare_v2( dbConnection->getConn(), req.c_str(), -1, &stmt, NULL );
        if ( res != SQLITE_OK )
        {
            throw std::runtime_error( std::string( "Failed to execute request: " ) + req + " " +
                        sqlite3_errmsg( dbConnection->getConn() ) );
        }
        m_stmt.reset( stmt );
    }

    template <typename... Args>
    void execute(Args&&... args)
    {
        m_bindIdx = 1;
        (void)std::initializer_list<bool>{ _bind( std::forward<Args>( args ) )... };
    }

    Row row()
    {
        auto res = sqlite3_step( m_stmt.get() );
        if ( res == SQLITE_ROW )
            return Row( m_stmt.get() );
        else if ( res == SQLITE_DONE )
            return Row();
        else
        {
            std::string errMsg = sqlite3_errmsg( m_dbConn->getConn() );
            switch ( res )
            {
                case SQLITE_CONSTRAINT:
                    throw errors::ConstraintViolation( m_req, errMsg );
                default:
                    throw std::runtime_error( m_req + ": " + errMsg );
            }
        }
    }

private:
    template <typename T>
    bool _bind( T&& value )
    {
        auto res = Traits<T>::Bind( m_stmt.get(), m_bindIdx, std::forward<T>( value ) );
        if ( res != SQLITE_OK )
            throw std::runtime_error( "Failed to bind parameter" );
        m_bindIdx++;
        return true;
    }

private:
    std::unique_ptr<sqlite3_stmt, int (*)(sqlite3_stmt*)> m_stmt;
    DBConnection m_dbConn;
    std::string m_req;
    unsigned int m_bindIdx;
};

class Tools
{
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
            assert(Transaction::transactionInProgress() == false);
            auto ctx = dbConnection->acquireContext();
            auto chrono = std::chrono::steady_clock::now();

            std::vector<std::shared_ptr<INTF>> results;
            auto stmt = Statement( dbConnection, req );
            stmt.execute( std::forward<Args>( args )... );
            Row sqliteRow;
            while ( ( sqliteRow = stmt.row() ) != nullptr )
            {
                auto row = IMPL::load( dbConnection, sqliteRow );
                results.push_back( row );
            }
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_DEBUG("Executed ", req, " in ",
                     std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
            return results;
        }

        template <typename T, typename... Args>
        static std::shared_ptr<T> fetchOne( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            assert(Transaction::transactionInProgress() == false);
            auto ctx = dbConnection->acquireContext();
            auto chrono = std::chrono::steady_clock::now();

            auto stmt = Statement( dbConnection, req );
            stmt.execute( std::forward<Args>( args )... );
            auto row = stmt.row();
            if ( row == nullptr )
                return nullptr;
            auto res = T::load( dbConnection, row );
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_DEBUG("Executed ", req, " in ",
                     std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
            return res;
        }

        template <typename... Args>
        static bool executeRequest( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            SqliteConnection::RequestContext ctx;
            if (Transaction::transactionInProgress() == false)
                ctx = dbConnection->acquireContext();
            return executeRequestLocked( dbConnection, req, std::forward<Args>( args )... );
        }

        template <typename... Args>
        static bool executeDelete( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            SqliteConnection::RequestContext ctx;
            if (Transaction::transactionInProgress() == false)
                ctx = dbConnection->acquireContext();
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

        template <typename... Args>
        static bool executeInsert( DBConnection dbConnectionWeak, const std::string& req, Args&&... args )
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
            SqliteConnection::RequestContext ctx;
            if (Transaction::transactionInProgress() == false)
                ctx = dbConnection->acquireContext();
            if ( executeRequestLocked( dbConnection, req, std::forward<Args>( args )... ) == false )
                return 0;
            return sqlite3_last_insert_rowid( dbConnection->getConn() );
        }

    private:
        template <typename... Args>
        static bool executeRequestLocked( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            auto chrono = std::chrono::steady_clock::now();
            auto stmt = Statement( dbConnection, req );
            stmt.execute( std::forward<Args>( args )... );
            while ( stmt.row() != nullptr )
                ;
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_DEBUG("Executed ", req, " in ",
                     std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
            return true;
        }

        // Let SqliteConnection access executeRequestLocked
        friend SqliteConnection;
};

}

#endif // SQLITETOOLS_H
