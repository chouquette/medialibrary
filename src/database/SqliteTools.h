/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#pragma once

#include <cassert>
#include <chrono>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <compat/Mutex.h>
#include "Common.h"
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
    explicit Row( sqlite3_stmt* stmt )
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
     * @brief extract Returns the next value and advances to the next column
     */
    template <typename T>
    T extract()
    {
        if ( m_idx >= m_nbColumns )
            throw errors::ColumnOutOfRange( m_idx, m_nbColumns );
        auto t = sqlite::Traits<T>::Load( m_stmt, m_idx );
        m_idx++;
        return t;
    }

    /**
     * @brief operator >> Extracts the next column from this result row.
     */
    template <typename T>
    Row& operator>>( T& t )
    {
        // Do not use extract as T might not be Copyable. Traits class however
        // always return a type that can be use to construct a T
        if ( m_idx >= m_nbColumns )
            throw errors::ColumnOutOfRange( m_idx, m_nbColumns );
        t = sqlite::Traits<T>::Load( m_stmt, m_idx );
        m_idx++;
        return *this;
    }

    unsigned int nbColumns() const
    {
        return m_nbColumns;
    }

    void advanceToColumn( unsigned int idx )
    {
        if ( idx >= m_nbColumns )
            throw errors::ColumnOutOfRange( idx, m_nbColumns );
        m_idx = idx;
    }

    /**
     * @brief Returns the value in column idx, but doesn't advance to the next column
     */
    template <typename T>
    T load(unsigned int idx) const
    {
        if ( idx >= m_nbColumns )
            throw errors::ColumnOutOfRange( idx, m_nbColumns );
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

    /**
     * @brief hasRemainingColumns Returns true if there are more column to extract
     *
     * This is meant to be used after a serie of calls to extract().
     * When load() is used, the internal index isn't updated and this function
     * would always return true if there is at least one column.
     */
    bool hasRemainingColumns() const
    {
        return m_idx < m_nbColumns;
    }

private:
    sqlite3_stmt* m_stmt;
    unsigned int m_idx;
    unsigned int m_nbColumns;
};

class Statement
{
public:
    Statement( Connection::Handle dbConnection, const std::string& req )
        : m_stmt( nullptr, [](sqlite3_stmt* stmt) {
                sqlite3_clear_bindings( stmt );
                sqlite3_reset( stmt );
            })
        , m_dbConn( dbConnection )
        , m_bindIdx( 0 )
        , m_isCommit( false )
    {
        std::lock_guard<compat::Mutex> lock( StatementsCacheLock );
        auto& connMap = StatementsCache[ dbConnection ];
        auto it = connMap.find( req );
        if ( it == end( connMap ) )
        {
            sqlite3_stmt* stmt;
            int res = sqlite3_prepare_v2( dbConnection, req.c_str(),
                                          req.size() + 1, &stmt, nullptr );
            if ( res != SQLITE_OK )
            {
                errors::mapToException( req.c_str(), sqlite3_errmsg( dbConnection ),
                                        res );
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

    Statement( const std::string& req )
        : Statement( Connection::Context::handle(), req )
    {
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
            auto extRes = sqlite3_step( m_stmt.get() );
            auto res = extRes & 0xFF;
            if ( res == SQLITE_ROW )
                return Row( m_stmt.get() );
            else if ( res == SQLITE_DONE )
                return Row{};
            else if ( ( Transaction::isInProgress() == false ||
                        m_isCommit == true ) &&
                     errors::isInnocuous( res ) && maxRetries-- > 0 )
                continue;
            auto errMsg = sqlite3_errmsg( m_dbConn );
            const char* reqStr = sqlite3_sql( m_stmt.get() );
            if ( reqStr == nullptr )
                reqStr = "<unknown request>";
            errors::mapToException( reqStr, errMsg, extRes );
            assert( !"unreachable" );
        }
    }

    static void FlushStatementCache()
    {
        std::lock_guard<compat::Mutex> lock( StatementsCacheLock );
        StatementsCache.clear();
    }

    static void FlushConnectionStatementCache( Connection::Handle h )
    {
        std::lock_guard<compat::Mutex> lock( StatementsCacheLock );
        auto it = StatementsCache.find( h );
        if ( it != end( StatementsCache ) )
            StatementsCache.erase( it );
    }

private:
    template <typename T>
    bool _bind( T&& value )
    {
        auto res = Traits<T>::Bind( m_stmt.get(), m_bindIdx,
                                    std::forward<T>( value ) );
        if ( res != SQLITE_OK )
        {
            auto sqlStr = sqlite3_sql( m_stmt.get() );
            errors::mapToException( sqlStr, sqlite3_errmsg( m_dbConn ), res );
        }
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
    Connection::Handle m_dbConn;
    int m_bindIdx;
    bool m_isCommit;
    static compat::Mutex StatementsCacheLock;
    using StatementsCacheMap = std::unordered_map<std::string, CachedStmtPtr,utils::hash::xxhasher>;
    static std::unordered_map<Connection::Handle, StatementsCacheMap> StatementsCache;
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
        static std::vector<std::shared_ptr<INTF>> fetchAll( MediaLibraryPtr ml,
                                                            const std::string& req,
                                                            Args&&... args )
        {
            auto dbConnection = ml->getConn();
            OPEN_READ_CONTEXT( ctx, dbConnection );
            auto chrono = std::chrono::steady_clock::now();

            std::vector<std::shared_ptr<INTF>> results;
            Statement stmt( Connection::Context::handle(), req );
            stmt.execute( std::forward<Args>( args )... );
            Row sqliteRow;
            while ( ( sqliteRow = stmt.row() ) != nullptr )
            {
                auto row = std::make_shared<IMPL>( ml, sqliteRow );
                results.push_back( row );
            }
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_VERBOSE("Executed ", req, " in ",
                std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
            return results;
        }

        template <typename T, typename... Args>
        static std::shared_ptr<T> fetchOne( MediaLibraryPtr ml,
                                            const std::string& req, Args&&... args )
        {
            auto dbConnection = ml->getConn();
            OPEN_READ_CONTEXT( ctx, dbConnection );
            auto chrono = std::chrono::steady_clock::now();

            Statement stmt( req );
            stmt.execute( std::forward<Args>( args )... );
            auto row = stmt.row();
            std::shared_ptr<T> res;
            if ( row != nullptr )
                res = std::make_shared<T>( ml, row );
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_VERBOSE("Executed ", req, " in ",
                std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
            return res;
        }

        template <typename... Args>
        static void executeRequest( sqlite::Connection* dbConnection,
                                    const std::string& req, Args&&... args )
        {
            OPEN_WRITE_CONTEXT( ctx, dbConnection );
            executeRequestLocked( Connection::Context::handle(), req, std::forward<Args>( args )... );
        }

        template <typename... Args>
        ML_FORCE_USED static bool executeDelete( sqlite::Connection* dbConnection,
                                                 const std::string& req,
                                                 Args&&... args )
        {
            OPEN_WRITE_CONTEXT( ctx, dbConnection );
            try
            {
                executeRequestLocked( Connection::Context::handle(), req,
                                      std::forward<Args>( args )... );
            }
            catch ( const sqlite::errors::Exception& ex )
            {
                if ( sqlite::errors::isInnocuous( ex ) == true )
                {
                    LOG_ERROR( "Failed to execute update/delete: ", ex.what() );
                    return false;
                }
                throw;
            }
            return true;
        }

        template <typename... Args>
        ML_FORCE_USED static bool executeUpdate( sqlite::Connection* dbConnection,
                                                 const std::string& req,
                                                 Args&&... args )
        {
            // The code would be exactly the same, do not freak out because it calls executeDelete :)
            return executeDelete( dbConnection, req, std::forward<Args>( args )... );
        }

        /**
         * Inserts a record to the DB and return the newly created primary key.
         * Returns 0 (which is an invalid sqlite primary key) when insertion fails.
         */
        template <typename... Args>
        ML_FORCE_USED static int64_t executeInsert( sqlite::Connection* dbConnection,
                                                    const std::string& req,
                                                    Args&&... args )
        {
            OPEN_WRITE_CONTEXT( ctx, dbConnection );
            try
            {
                auto handle = Connection::Context::handle();
                executeRequestLocked( handle, req, std::forward<Args>( args )... );
                return sqlite3_last_insert_rowid( handle );
            }
            catch ( const sqlite::errors::Exception& ex )
            {
                if ( sqlite::errors::isInnocuous( ex ) == true )
                {
                    LOG_ERROR( "Failed to execute update/delete: ", ex.what() );
                    return 0;
                }
                throw;
            }
        }

        static bool checkTriggerStatement( const std::string& expectedStatement,
                                           const std::string& triggerName )
        {
            auto actualStatement = fetchTriggerStatement( triggerName );
            if ( actualStatement == expectedStatement )
                return true;
            LOG_ERROR( "Mismatching statement for trigger ", triggerName, "." );
            LOG_ERROR( "Expected: ", expectedStatement );
            LOG_ERROR( "Actual:   ", actualStatement );
            return false;
        }

        static bool checkIndexStatement( const std::string& expectedStatement,
                                         const std::string& indexName )
        {
            auto actualStatement = fetchIndexStatement( indexName );
            if ( actualStatement == expectedStatement )
                return true;
            LOG_ERROR( "Mismatching statement for index ", indexName, "." );
            LOG_ERROR( "Expected: ", expectedStatement );
            LOG_ERROR( "Actual:   ", actualStatement );
            return false;
        }

        static bool checkTableSchema( const std::string& schema,
                                      const std::string& tableName )
        {
            auto actualSchema = fetchTableSchema( tableName );
            if ( actualSchema == schema )
                return true;
            LOG_ERROR( "Mismatching schema for table ", tableName, "." );
            LOG_ERROR( "Expected: ", schema );
            LOG_ERROR( "Actual:   ", actualSchema );
            return false;
        }

        static std::vector<std::string> listTables( sqlite::Connection* dbConn )
        {
            OPEN_READ_CONTEXT( ctx, dbConn );
            std::vector<std::string> tables;
            medialibrary::sqlite::Statement stmt{
                    "SELECT name FROM sqlite_master WHERE type='table' "
                    "AND name NOT LIKE '%#_%' ESCAPE '#'"
            };
            stmt.execute();
            sqlite::Row row;
            while ( ( row = stmt.row() ) != nullptr )
            {
                tables.push_back( row.load<std::string>( 0 ) );
            }
            return tables;
        }

        /**
         * @brief sanitizePattern Ensures the pattern is valid, and append a wildcard char
         * @return Essentially returns pattern with «'» and «"» encoded for
         *         sqlite, and a '*' appended
         */
        static std::string sanitizePattern( const std::string& pattern );

    private:
        template <typename... Args>
        static void executeRequestLocked( sqlite::Connection::Handle handle,
                                          const std::string& req, Args&&... args )
        {
            auto chrono = std::chrono::steady_clock::now();
            Statement stmt( handle, req );
            stmt.execute( std::forward<Args>( args )... );
            while ( stmt.row() != nullptr )
                ;
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_VERBOSE("Executed ", req, " in ",
                std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
        }

        static std::string fetchSchemaSql( const std::string& type,
                                           const std::string& name )
        {
            const std::string req{ "SELECT sql FROM sqlite_master "
                                 "WHERE type=? AND name=?" };
            auto chrono = std::chrono::steady_clock::now();
            Statement stmt( req );
            stmt.execute( type, name );
            auto row = stmt.row();
            if ( row == nullptr )
                return "";
            assert( row.nbColumns() == 1 );
            auto res = row.extract<std::string>();
            auto duration = std::chrono::steady_clock::now() - chrono;
            LOG_VERBOSE("Executed ", req, " in ",
                std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
            return res;
        }

        static std::string fetchTableSchema( const std::string& tableName )
        {
            return fetchSchemaSql( "table", tableName );
        }

        static std::string fetchTriggerStatement( const std::string& triggerName )
        {
            return fetchSchemaSql( "trigger", triggerName );
        }

        static std::string fetchIndexStatement( const std::string& indexName )
        {
            return fetchSchemaSql( "index", indexName );
        }
};

}

}
