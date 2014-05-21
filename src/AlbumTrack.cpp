#include "AlbumTrack.h"
#include "Album.h"
#include "SqliteTools.h"

const std::string policy::AlbumTrackTable::Name = "AlbumTrack";
const std::string policy::AlbumTrackTable::CacheColumn = "id_track";
unsigned int AlbumTrack::* const policy::AlbumTrackTable::PrimaryKey = &AlbumTrack::m_id;

AlbumTrack::AlbumTrack( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_album( NULL )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_title = (const char*)sqlite3_column_text( stmt, 1 );
    m_genre = (const char*)sqlite3_column_text( stmt, 2 );
    m_trackNumber = sqlite3_column_int( stmt, 3 );
    m_albumId = sqlite3_column_int( stmt, 4 );
}

bool AlbumTrack::createTable(sqlite3* dbConnection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS AlbumTrack ("
                "id_track INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT,"
                "genre TEXT,"
                "track_number UNSIGNED INTEGER,"
                "album_id UNSIGNED INTEGER NOT NULL,"
                "FOREIGN KEY (album_id) REFERENCES Album(id_album) ON DELETE CASCADE"
            ")";
    return SqliteTools::executeRequest( dbConnection, req );
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

std::shared_ptr<IAlbum> AlbumTrack::album()
{
    if ( m_album == nullptr && m_albumId != 0 )
    {
        m_album = Album::fetch( m_dbConnection, m_albumId );
    }
    return m_album;
}
