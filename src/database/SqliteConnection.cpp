#include "SqliteConnection.h"

#include "database/SqliteTools.h"

SqliteConnection::SqliteConnection( const std::string &dbPath )
    : m_dbPath( dbPath )
{
    if ( sqlite3_threadsafe() == 0 )
        throw std::runtime_error( "SQLite isn't built with threadsafe mode" );
    if ( sqlite3_config( SQLITE_CONFIG_MULTITHREAD ) == SQLITE_ERROR )
        throw std::runtime_error( "Failed to enable sqlite multithreaded mode" );
}

sqlite3 *SqliteConnection::getConn()
{
    std::unique_lock<std::mutex> lock( m_connMutex );
    sqlite3* dbConnection;
    auto it = m_conns.find( std::this_thread::get_id() );
    if ( it == end( m_conns ) )
    {
        auto res = sqlite3_open( m_dbPath.c_str(), &dbConnection );
        if ( res != SQLITE_OK )
            throw std::runtime_error( "Failed to connect to database" );
        res = sqlite3_busy_timeout( dbConnection, 500 );
        if ( res != SQLITE_OK )
            LOG_WARN( "Failed to enable sqlite busy timeout" );
        m_conns.emplace(std::this_thread::get_id(), ConnPtr( dbConnection, &sqlite3_close ) );
        lock.unlock();
        if ( sqlite::Tools::executeRequest( this, "PRAGMA foreign_keys = ON" ) == false )
            throw std::runtime_error( "Failed to enable foreign keys" );
        return dbConnection;
    }
    return it->second.get();
}

void SqliteConnection::release()
{
    std::unique_lock<std::mutex> lock( m_connMutex );
    m_conns.erase( std::this_thread::get_id() );
}
