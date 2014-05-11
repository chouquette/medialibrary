#include "AlbumTrack.h"
#include "Album.h"
#include "SqliteTools.h"

AlbumTrack::AlbumTrack( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_album( NULL )
{
    m_id = sqlite3_column_int( stmt, 1 );
    m_title = (const char*)sqlite3_column_text( stmt, 2 );
    m_genre = (const char*)sqlite3_column_text( stmt, 3 );
    m_trackNumber = sqlite3_column_int( stmt, 4 );
    m_albumId = sqlite3_column_int( stmt, 5 );
}

bool AlbumTrack::createTable(sqlite3* dbConnection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS AlbumTrack ("
                "id_track INTEGER PRIMARY KEY AUTO INCREMENT,"
                "title TEXT,"
                "genre TEXT,"
                "track_number UNSIGNED INTEGER,"
                "album_id UNSIGNED INTEGER NOT NULL,"
                "FOREIGN KEY (album_id) REFERENCES Album(id_album) ON DELETE CASCADE"
            ")";
    return SqliteTools::createTable( dbConnection, req );
}

const std::string& AlbumTrack::genre()
{
    return m_genre;
}

const std::string& AlbumTrack::title()
{
    return m_title;
}

unsigned int AlbumTrack::trackNumber()
{
    return m_trackNumber;
}

IAlbum* AlbumTrack::album()
{
    if ( m_album == NULL && m_albumId != 0 )
    {
        const char* req = "SELECT * FROM Album WHERE id_album = ?";
        m_album = SqliteTools::fetchOne<Album>( m_dbConnection, req, m_albumId );
    }
    return m_album;
}
