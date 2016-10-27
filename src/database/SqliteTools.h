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
#include <unordered_map>
#include <vector>

#include <compat/Mutex.h>
#include "database/SqliteConnection.h"
#include "database/SqliteErrors.h"
#include "database/SqliteTraits.h"
#include "database/SqliteTransaction.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"

namespace medialibrary
{

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

    constexpr Row()
        : m_stmt( nullptr )
        , m_idx( 0 )
        , m_nbColumns( 0 )
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

    bool operator==(std::nullptr_t) const
    {
        return m_stmt == nullptr;
    }

    bool operator!=(std::nullptr_t) const
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
    Statement( SqliteConnection::Handle dbConnection, const std::string& req )
        : m_stmt( nullptr, [](sqlite3_stmt* stmt) {
                sqlite3_clear_bindings( stmt );
                sqlite3_reset( stmt );
            })
        , m_dbConn( dbConnection )
        , m_bindIdx( 0 )
    {
        std::lock_guard<compat::Mutex> lock( StatementsCacheLock );
        auto& connMap = StatementsCache[ dbConnection ];
        auto it = connMap.find( req );
        if ( it == end( connMap ) )
        {
            sqlite3_stmt* stmt;
            int res = sqlite3_prepare_v2( dbConnection, req.c_str(), -1, &stmt, NULL );
            if ( res != SQLITE_OK )
            {
                throw std::runtime_error( std::string( "Failed to compile request: " ) + req + " " +
                            sqlite3_errmsg( dbConnection ) );
            }
            m_stmt.reset( stmt );
            connMap.emplace( req, CachedStmtPtr( stmt, &sqlite3_finalize ) );
        }
        else
        {
            m_stmt.reset( it->second.get() );
        }
        if ( req == "COMMIT" )
            m_isCommit = true;
    }

    template <typename... Args>
    void execute(Args&&... args)
    {
        m_bindIdx = 1;
        (void)std::initializer_list<bool>{ _bind( std::forward<Args>( args ) )... };
    }

    Row row()
    {
        auto maxRetries = 10;
        while ( true )
        {
            auto res = sqlite3_step( m_stmt.get() );
            if ( res == SQLITE_ROW )
                return Row( m_stmt.get() );
            else if ( res == SQLITE_DONE )
                return Row();
            else if ( res == SQLITE_BUSY && ( Transaction::transactionInProgress() == false ||
                                              m_isCommit == true ) && maxRetries-- > 0 )
                continue;
            std::string errMsg = sqlite3_errmsg( m_dbConn );
            switch ( res )
            {
                case SQLITE_CONSTRAINT:
                    throw errors::ConstraintViolation( sqlite3_sql( m_stmt.get() ), errMsg );
                default:
                    throw std::runtime_error( std::string{ sqlite3_sql( m_stmt.get() ) }
                                              + ": " + errMsg );
            }
        }
    }

    static void FlushStatementCache()
    {
        std::lock_guard<compat::Mutex> lock( StatementsCacheLock );
        StatementsCache.clear();
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
    // Used during the connection lifetime. This holds a compiled request
    using CachedStmtPtr = std::unique_ptr<sqlite3_stmt, int (*)(sqlite3_stmt*)>;
    // Used for the current statement execution, this
    // basically holds the state of the currently executed request.
    using StatementPtr = std::unique_ptr<sqlite3_stmt, void(*)(sqlite3_stmt*)>;
    StatementPtr m_stmt;
    SqliteConnection::Handle m_dbConn;
    unsigned int m_bindIdx;
    bool m_isCommit;
    static compat::Mutex StatementsCacheLock;
    static std::unordered_map<SqliteConnection::Handle,
                            std::unordered_map<std::string, CachedStmtPtr>> StatementsCache;
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
        static std::vector<std::shared_ptr<INTF> > fetchAll( MediaLibraryPtr ml, const std::string& req, Args&&... args )
        {
            auto dbConnection = ml->getConn();
            SqliteConnection::ReadContext ctx;
            if (Transaction::transactionInProgress() == false)
                ctx = dbConnection->acquireReadContext();
            auto chrono = std::chrono::steady_clock::now();

            std::vector<std::shared_ptr<INTF>> results;
            Statement stmt( dbConnection->getConn(), req );
            stmt.execute( std::forward<Args>( args )... );
            Row sqliteRow;
            while ( ( sqliteRow = stmt.row() ) != nullptr )
            {
                auto row = IMPL::load( ml, sqliteRow );
                results.push_back( row );
            }
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_DEBUG("Executed ", req, " in ",
                     std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
            return results;
        }

        template <typename T, typename... Args>
        static std::shared_ptr<T> fetchOne( MediaLibraryPtr ml, const std::string& req, Args&&... args )
        {
            auto dbConnection = ml->getConn();
            SqliteConnection::ReadContext ctx;
            if (Transaction::transactionInProgress() == false)
                ctx = dbConnection->acquireReadContext();
            auto chrono = std::chrono::steady_clock::now();

            Statement stmt( dbConnection->getConn(), req );
            stmt.execute( std::forward<Args>( args )... );
            auto row = stmt.row();
            if ( row == nullptr )
                return nullptr;
            auto res = T::load( ml, row );
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_DEBUG("Executed ", req, " in ",
                     std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
            return res;
        }

        template <typename... Args>
        static bool executeRequest( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            SqliteConnection::WriteContext ctx;
            if (Transaction::transactionInProgress() == false)
                ctx = dbConnection->acquireWriteContext();
            return executeRequestLocked( dbConnection, req, std::forward<Args>( args )... );
        }

        template <typename... Args>
        static bool executeDelete( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            SqliteConnection::WriteContext ctx;
            if (Transaction::transactionInProgress() == false)
                ctx = dbConnection->acquireWriteContext();
            if ( executeRequestLocked( dbConnection, req, std::forward<Args>( args )... ) == false )
                return false;
            return sqlite3_changes( dbConnection->getConn() ) > 0;
        }

        template <typename... Args>
        static bool executeUpdate( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            // The code would be exactly the same, do not freak out because it calls executeDelete :)
            return executeDelete( dbConnection, req, std::forward<Args>( args )... );
        }

        /**
         * Inserts a record to the DB and return the newly created primary key.
         * Returns 0 (which is an invalid sqlite primary key) when insertion fails.
         */
        template <typename... Args>
        static int64_t executeInsert( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            SqliteConnection::WriteContext ctx;
            if (Transaction::transactionInProgress() == false)
                ctx = dbConnection->acquireWriteContext();
            if ( executeRequestLocked( dbConnection, req, std::forward<Args>( args )... ) == false )
                return 0;
            return sqlite3_last_insert_rowid( dbConnection->getConn() );
        }

    private:
        template <typename... Args>
        static bool executeRequestLocked( DBConnection dbConnection, const std::string& req, Args&&... args )
        {
            auto chrono = std::chrono::steady_clock::now();
            Statement stmt( dbConnection->getConn(), req );
            stmt.execute( std::forward<Args>( args )... );
            while ( stmt.row() != nullptr )
                ;
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_DEBUG("Executed ", req, " in ",
                     std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
            return true;
        }
};

}

}

#endif // SQLITETOOLS_H
