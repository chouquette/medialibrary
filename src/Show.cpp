#include "Show.h"
#include "SqliteTools.h"

Show::Show(sqlite3* dbConnection, sqlite3_stmt* stmt)
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 1 );
    m_name = (const char*)sqlite3_column_text( stmt, 2 );
    m_releaseYear = sqlite3_column_int( stmt, 3 );
    m_shortSummary = (const char*)sqlite3_column_text( stmt, 4 );
    m_artworkUrl = (const char*)sqlite3_column_text( stmt, 5 );
    m_lastSyncDate = sqlite3_column_int( stmt, 6 );
    m_tvdbId = (const char*)sqlite3_column_text( stmt, 7 );
}

Show::Show(sqlite3* dbConnection)
    : m_dbConnection( dbConnection )
    , m_id( 0 )
{
}

const std::string& Show::name()
{
    return m_name;
}

unsigned int Show::releaseYear()
{
    return m_releaseYear;
}

const std::string& Show::shortSummary()
{
    return m_shortSummary;
}

const std::string& Show::artworkUrl()
{
    return m_artworkUrl;
}


time_t Show::lastSyncDate()
{
    return m_lastSyncDate;
}

const std::string& Show::tvdbId()
{
    return m_tvdbId;
}

bool Show::CreateTable(sqlite3* dbConnection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS Show("
            "id_show INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT, UNSIGNED INTEGER release_year, TEXT short_summary,"
            "TEXT artwork_url, UNSIGNED INTEGER last_sync_date, TEXT tvdb_id)";
    return SqliteTools::CreateTable( dbConnection, req );
}
