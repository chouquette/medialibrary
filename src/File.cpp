#include <cassert>
#include <cstdlib>
#include <cstring>

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
    m_mrl = (const char*)sqlite3_column_text( stmt, 6 );
}

File::File( const std::string& mrl )
    : m_dbConnection( NULL )
    , m_id( 0 )
    , m_type( UnknownType )
    , m_duration( 0 )
    , m_albumTrackId( 0 )
    , m_playCount( 0 )
    , m_showEpisodeId( 0 )
    , m_mrl( mrl )
    , m_albumTrack( NULL )
    , m_showEpisode( NULL )
    , m_labels( NULL )
{
}

bool File::insert( sqlite3* dbConnection )
{
    assert( m_dbConnection == NULL );
    assert( m_id == 0 );
    sqlite3_stmt* stmt;
    const char* req = "INSERT INTO File VALUES(NULL, ?, ?, ?, ?, ?, ?)";
    if ( sqlite3_prepare_v2( dbConnection, req, -1, &stmt, NULL ) != SQLITE_OK )
    {
        std::cerr << "Failed to insert record: " << sqlite3_errmsg( dbConnection ) << std::endl;
        return false;
    }
    const char* tmpMrl = strdup( m_mrl.c_str() );
    sqlite3_bind_int( stmt, 1, m_type );
    sqlite3_bind_int( stmt, 2, m_duration );
    sqlite3_bind_int( stmt, 3, m_albumTrackId );
    sqlite3_bind_int( stmt, 4, m_playCount );
    sqlite3_bind_int( stmt, 5, m_showEpisodeId );
    sqlite3_bind_text( stmt, 6, tmpMrl, -1, &free );
    if ( sqlite3_step( stmt ) != SQLITE_DONE )
        return false;
    m_id = sqlite3_last_insert_rowid( dbConnection );
    m_dbConnection = dbConnection;
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
        const char* req = "SELECT l.* FROM Label l "
                "LEFT JOIN LabelFileRelation lfr ON lfr.id_label = l.id_label "
                "WHERE lfr.id_file = ?";
        SqliteTools::fetchAll<Label>( m_dbConnection, req, m_id, m_labels );
    }
    return *m_labels;
}

int File::playCount()
{
    return m_playCount;
}

const std::string& File::mrl()
{
    return m_mrl;
}

ILabel* File::addLabel(const std::string& label)
{
    Label* l = new Label( label );
    if ( l->insert( m_dbConnection ) == false )
    {
        delete l;
        return NULL;
    }
    l->link( this );
    return l;
}

unsigned int File::id() const
{
    return m_id;
}

bool File::createTable(sqlite3* connection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS File("
            "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "duration UNSIGNED INTEGER,"
            "album_track_id UNSIGNED INTEGER,"
            "play_count UNSIGNED INTEGER,"
            "show_episode_id UNSIGNED INTEGER,"
            "mrl TEXT"
            ")";
    return SqliteTools::createTable( connection, req );
}
