#include <cassert>

#include "File.h"
#include "Label.h"
#include "SqliteTools.h"
#include "Album.h"

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

IAlbumTrack* File::albumTrack()
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
    if ( m_labels == NULL )
    {
        m_labels = new std::vector<ILabel*>;
        const char* req = "SELECT * FROM Labels l"
                "LEFT JOIN LabelFileRelation lfr ON lfr.id_label = f.id_label "
                "WHERE lfr.id_file = ?";
        sqlite3_stmt* stmt;
        int res = sqlite3_prepare_v2( m_dbConnection, req, -1, &stmt, NULL );
        if ( res != SQLITE_OK )
            return *m_labels;
        sqlite3_bind_int( stmt, 1, m_id );
        res = sqlite3_step( stmt );
        while ( res == SQLITE_ROW )
        {
            ILabel* l = new Label( m_dbConnection, stmt );
            m_labels->push_back( l );
            res = sqlite3_step( stmt );
        }
        sqlite3_finalize( stmt );
    }
    return *m_labels;
}

bool File::CreateTable(sqlite3* connection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS File("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER, duration UNSIGNED INTEGER,"
            "album_track_id UNSIGNED INTEGER)";
    return SqliteTools::CreateTable( connection, req );
}
