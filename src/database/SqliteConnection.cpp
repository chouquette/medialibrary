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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "SqliteConnection.h"

#include "database/SqliteTools.h"

namespace medialibrary
{

SqliteConnection::SqliteConnection( const std::string &dbPath )
    : SqliteConnection( dbPath, true )
{
}

SqliteConnection::SqliteConnection( const std::string &dbPath, bool enableForeignKeys )
    : m_dbPath( dbPath )
    , m_readLock( m_contextLock )
    , m_writeLock( m_contextLock )
    , m_enableForeignKeys( enableForeignKeys )
{
    if ( sqlite3_threadsafe() == 0 )
        throw std::runtime_error( "SQLite isn't built with threadsafe mode" );
    if ( sqlite3_config( SQLITE_CONFIG_MULTITHREAD ) == SQLITE_ERROR )
        throw std::runtime_error( "Failed to enable sqlite multithreaded mode" );
}

SqliteConnection::~SqliteConnection()
{
    sqlite::Statement::FlushStatementCache();
}

SqliteConnection::Handle SqliteConnection::getConn()
{
    std::unique_lock<compat::Mutex> lock( m_connMutex );
    sqlite3* dbConnection;
    auto it = m_conns.find( compat::this_thread::get_id() );
    if ( it == end( m_conns ) )
    {
        auto res = sqlite3_open( m_dbPath.c_str(), &dbConnection );
        ConnPtr dbConn( dbConnection, &sqlite3_close );
        if ( res != SQLITE_OK )
            throw sqlite::errors::Generic( std::string( "Failed to connect to database: " )
                                           + sqlite3_errstr( res ) );
        sqlite3_extended_result_codes( dbConnection, 1 );
        sqlite3_busy_timeout( dbConnection, 500 );
        std::string pragmaForeignReq = "PRAGMA foreign_keys = ";
        sqlite::Statement s( dbConnection, pragmaForeignReq + ( m_enableForeignKeys ? "ON" : "OFF" ) );
        s.execute();
        while ( s.row() != nullptr )
            ;
        sqlite::Statement s2( dbConnection, "PRAGMA recursive_triggers = ON" );
        s2.execute();
        while ( s2.row() != nullptr )
            ;
        m_conns.emplace( compat::this_thread::get_id(), std::move( dbConn ) );
        sqlite3_update_hook( dbConnection, &updateHook, this );
        return dbConnection;
    }
    return it->second.get();
}

std::unique_ptr<sqlite::Transaction> SqliteConnection::newTransaction()
{
    return std::unique_ptr<sqlite::Transaction>{ new sqlite::Transaction( this ) };
}

SqliteConnection::ReadContext SqliteConnection::acquireReadContext()
{
    return ReadContext{ m_readLock };
}

SqliteConnection::WriteContext SqliteConnection::acquireWriteContext()
{
    return WriteContext{ m_writeLock };
}

std::string SqliteConnection::getDBPath()
{
    return m_dbPath;
}

void SqliteConnection::registerUpdateHook( const std::string& table, SqliteConnection::UpdateHookCb cb )
{
    m_hooks.emplace( table, cb );
}

void SqliteConnection::updateHook( void* data, int reason, const char*,
                                   const char* table, sqlite_int64 rowId )
{
    const auto self = reinterpret_cast<SqliteConnection*>( data );
    auto it = self->m_hooks.find( table );
    if ( it == end( self->m_hooks ) )
        return;
    switch ( reason )
    {
    case SQLITE_INSERT:
        it->second( HookReason::Insert, rowId );
        break;
    case SQLITE_UPDATE:
        it->second( HookReason::Update, rowId );
        break;
    case SQLITE_DELETE:
        it->second( HookReason::Delete, rowId );
        break;
    }
}

}
