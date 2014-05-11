#include <cassert>

#include "Album.h"
#include "AlbumTrack.h"
#include "File.h"
#include "Label.h"
#include "ShowEpisode.h"
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
    m_playCount = sqlite3_column_int( stmt, 4 );
    m_showEpisodeId = sqlite3_column_int( stmt, 5 );
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

IAlbumTrack* File::albumTrack()
{
    if ( m_albumTrack == NULL && m_albumTrackId != 0 )
    {
        const char* req = "SELECT * FROM AlbumTrack WHERE id_track = ?";
        m_albumTrack = SqliteTools::fetchOne<AlbumTrack>( m_dbConnection, req, m_albumTrackId );
    }
    return m_albumTrack;
}

unsigned int File::duration()
{
    return m_duration;
}

IShowEpisode*File::showEpisode()
{
    if ( m_showEpisode == NULL && m_showEpisodeId != 0 )
    {
        const char* req = "SELECT * FROM ShowEpisode WHERE id_episode = ?";
        m_showEpisode = SqliteTools::fetchOne<ShowEpisode>( m_dbConnection, req, m_showEpisodeId );
    }
    return m_showEpisode;
}

std::vector<ILabel*> File::labels()
{
    if ( m_labels == NULL )
    {
        const char* req = "SELECT * FROM Labels l"
                "LEFT JOIN LabelFileRelation lfr ON lfr.id_label = f.id_label "
                "WHERE lfr.id_file = ?";
        SqliteTools::fetchAll<Label>( m_dbConnection, req, m_id, m_labels );
    }
    return *m_labels;
}

int File::playCount()
{
    return m_playCount;
}

bool File::CreateTable(sqlite3* connection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS File("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER, duration UNSIGNED INTEGER,"
            "album_track_id UNSIGNED INTEGER,"
            "play_count UNSIGNED INTEGER"
            "show_episode_id UNSIGNED INTEGER"
            ")";
    return SqliteTools::createTable( connection, req );
}
