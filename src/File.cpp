#include <cassert>
#include <cstdlib>
#include <cstring>

#include "Album.h"
#include "AlbumTrack.h"
#include "File.h"
#include "Label.h"
#include "ShowEpisode.h"
#include "SqliteTools.h"

const std::string policy::FileTable::Name = "File";
const std::string policy::FileTable::CacheColumn = "mrl";

File::File( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
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
{
}

bool File::insert( sqlite3* dbConnection )
{
    assert( m_dbConnection == NULL );
    assert( m_id == 0 );
    sqlite3_stmt* stmt;
    static const std::string req = "INSERT INTO " + policy::FileTable::Name +
            " VALUES(NULL, ?, ?, ?, ?, ?, ?)";
    if ( sqlite3_prepare_v2( dbConnection, req.c_str(), -1, &stmt, NULL ) != SQLITE_OK )
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

std::shared_ptr<IAlbumTrack> File::albumTrack()
{
    if ( m_albumTrack == nullptr && m_albumTrackId != 0 )
    {
        m_albumTrack = AlbumTrack::fetch( m_dbConnection, m_albumTrackId );
    }
    return m_albumTrack;
}

unsigned int File::duration()
{
    return m_duration;
}

std::shared_ptr<IShowEpisode> File::showEpisode()
{
    if ( m_showEpisode == nullptr && m_showEpisodeId != 0 )
    {
        m_showEpisode = ShowEpisode::fetch( m_dbConnection, m_showEpisodeId );
    }
    return m_showEpisode;
}

std::vector<std::shared_ptr<ILabel> > File::labels()
{
    std::vector<std::shared_ptr<ILabel> > labels;
    static const std::string req = "SELECT l.* FROM " + policy::LabelTable::Name + " l "
            "LEFT JOIN LabelFileRelation lfr ON lfr.id_label = l.id_label "
            "WHERE lfr.id_file = ?";
    SqliteTools::fetchAll<Label>( m_dbConnection, req.c_str(), m_id, labels );
    return labels;
}

int File::playCount()
{
    return m_playCount;
}

const std::string& File::mrl()
{
    return m_mrl;
}

unsigned int File::id() const
{
    return m_id;
}

bool File::createTable(sqlite3* connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FileTable::Name + "("
            "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "duration UNSIGNED INTEGER,"
            "album_track_id UNSIGNED INTEGER,"
            "play_count UNSIGNED INTEGER,"
            "show_episode_id UNSIGNED INTEGER,"
            "mrl TEXT"
            ")";
    return SqliteTools::createTable( connection, req.c_str() );
}

bool File::addLabel( LabelPtr label )
{
    if ( m_dbConnection == nullptr || m_id == 0 || label->id() == 0)
    {
        std::cerr << "Both file & label need to be inserted in database before being linked together" << std::endl;
        return false;
    }
    const char* req = "INSERT INTO LabelFileRelation VALUES(?, ?)";
    sqlite3_stmt* stmt;
    if ( sqlite3_prepare_v2( m_dbConnection, req, -1, &stmt, NULL ) != SQLITE_OK )
    {
        std::cerr << "Failed to insert record: " << sqlite3_errmsg( m_dbConnection ) << std::endl;
        return false;
    }
    sqlite3_bind_int( stmt, 1, label->id() );
    sqlite3_bind_int( stmt, 2, m_id );
    bool res = sqlite3_step( stmt ) == SQLITE_DONE;
    sqlite3_finalize( stmt );
    return res;
}

bool File::removeLabel( LabelPtr label )
{
    if ( m_dbConnection == nullptr || m_id == 0 || label->id() == 0 )
    {
        std::cerr << "Can't unlink a label/file not inserted in database" << std::endl;
        return false;
    }
    const char* req = "DELETE FROM LabelFileRelation WHERE id_label = ? AND id_file = ?";
    sqlite3_stmt* stmt;
    if ( sqlite3_prepare_v2( m_dbConnection, req, -1, &stmt, NULL ) != SQLITE_OK )
    {
        std::cerr << "Failed to remove record: " << sqlite3_errmsg( m_dbConnection ) << std::endl;
        return false;
    }
    sqlite3_bind_int( stmt, 1, label->id() );
    sqlite3_bind_int( stmt, 2, m_id );
    sqlite3_step( stmt );
    sqlite3_finalize( stmt );
    return sqlite3_changes( m_dbConnection ) > 0;
}

const std::string& policy::FileCache::key(const std::shared_ptr<File> self )
{
    return self->mrl();
}

std::string policy::FileCache::key(sqlite3_stmt* stmt)
{
    return Traits<std::string>::Load( stmt, 6 );
}
