#include "Album.h"
#include "AlbumTrack.h"

#include "SqliteTools.h"

Album::Album(sqlite3* dbConnection, sqlite3_stmt* stmt)
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 1 );
    m_name = (const char*)sqlite3_column_text( stmt, 2 );
    m_releaseYear = sqlite3_column_int( stmt, 3 );
    m_shortSummary = (const char*)sqlite3_column_text( stmt, 4 );
    m_artworkUrl = (const char*)sqlite3_column_text( stmt, 5 );
    m_lastSyncDate = sqlite3_column_int( stmt, 6 );
}

Album::Album(sqlite3* dbConnection)
{

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

const std::vector<IAlbumTrack*>&Album::tracks()
{
    if ( m_tracks == NULL )
    {
        const char* req = "SELECT * FROM AlbumTrack WHERE id_album = ?";
        SqliteTools::fetchAll<AlbumTrack>( m_dbConnection, req, m_id, m_tracks );
    }
    return *m_tracks;
}

bool Album::createTable(sqlite3* dbConnection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS Album("
            "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT, UNSIGNED INTEGER release_year, TEXT short_summary,"
            "TEXT artwork_url, UNSIGNED INTEGER last_sync_date)";
    return SqliteTools::createTable( dbConnection, req );
}

Album* Album::fetch(sqlite3* dbConnection, unsigned int albumTrackId)
{
    const char* req = "SELECT * FROM Album WHERE id_album = ?";
    return SqliteTools::fetchOne<Album>( dbConnection, req, albumTrackId );
}
