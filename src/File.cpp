#include <cassert>

#include "File.h"
#include "SqliteTools.h"

File::File( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_albumTrack( NULL )
    , m_showEpisode( NULL )
    , m_labels( NULL )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_type = (Type)sqlite3_column_int( stmt, 1 );
    m_duration = sqlite3_column_int( stmt, 2 );
    m_albumTrackId = sqlite3_column_int( stmt, 3 );
}

File::File()
    : m_dbConnection( NULL )
    , m_id( 0 )
{
}

bool File::insert( sqlite3* dbConnection )
{
    assert( m_dbConnection == NULL );
    m_dbConnection = dbConnection;
    sqlite3_stmt* stmt;
    std::string req = "INSERT INTO File VALUES(NULL, ?, ?, ?)";
    if ( sqlite3_prepare_v2( m_dbConnection, req.c_str(), -1, &stmt, NULL ) != SQLITE_OK )
        return false;
    sqlite3_bind_int( stmt, 1, m_type );
    sqlite3_bind_int( stmt, 2, m_duration );
    sqlite3_bind_int( stmt, 3, m_albumTrackId );
    if ( sqlite3_step( stmt ) != SQLITE_DONE )
        return false;
    m_id = sqlite3_last_insert_rowid( m_dbConnection );
    return true;
}

IAlbumTrack*File::albumTrack()
{
    if ( m_albumTrack == NULL && m_albumTrackId != 0 )
    {
        m_albumTrack = AlbumTrack::fetch( m_albumTrackId );
    }
    return m_albumTrack;
}

const std::string&File::artworkUrl()
{
}

unsigned int File::duration()
{
    return m_duration;
}

IShowEpisode*File::showEpisode()
{
}

std::vector<ILabel*> File::labels()
{
}

bool File::CreateTable(sqlite3* connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS File("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER, duration UNSIGNED INTEGER,"
            "album_track_id UNSIGNED INTEGER)";
    return SqliteTools::CreateTable( connection, req );
}
