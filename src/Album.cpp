#include "Album.h"
#include "AlbumTrack.h"

#include "SqliteTools.h"

const std::string policy::AlbumTable::Name = "Album";
const std::string policy::AlbumTable::CacheColumn = "id_album";

Album::Album(sqlite3* dbConnection, sqlite3_stmt* stmt)
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_name = (const char*)sqlite3_column_text( stmt, 1 );
    m_releaseYear = sqlite3_column_int( stmt, 2 );
    m_shortSummary = (const char*)sqlite3_column_text( stmt, 3 );
    m_artworkUrl = (const char*)sqlite3_column_text( stmt, 4 );
    m_lastSyncDate = sqlite3_column_int( stmt, 5 );
}

const std::string& Album::name()
{
    return m_name;
}

unsigned int Album::releaseYear()
{
    return m_releaseYear;
}

const std::string& Album::shortSummary()
{
    return m_shortSummary;
}

const std::string& Album::artworkUrl()
{
    return m_artworkUrl;
}

time_t Album::lastSyncDate()
{
    return m_lastSyncDate;
}

const std::vector<std::shared_ptr<IAlbumTrack>>& Album::tracks()
{
    if ( m_tracks == NULL )
    {
        m_tracks = new std::vector<std::shared_ptr<IAlbumTrack>>;
        const char* req = "SELECT * FROM AlbumTrack WHERE id_album = ?";
        SqliteTools::fetchAll<AlbumTrack>( m_dbConnection, req, *m_tracks, m_id );
    }
    return *m_tracks;
}

bool Album::createTable( sqlite3* dbConnection )
{
    const char* req = "CREATE TABLE IF NOT EXISTS Album("
                "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT,"
                "release_year UNSIGNED INTEGER,"
                "short_summary TEXT,"
                "artwork_url TEXT,"
                "UNSIGNED INTEGER last_sync_date"
            ")";
    return SqliteTools::createTable( dbConnection, req );
}
