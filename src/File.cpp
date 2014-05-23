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
unsigned int File::* const policy::FileTable::PrimaryKey = &File::m_id;

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
    : m_dbConnection( nullptr )
    , m_id( 0 )
    , m_type( UnknownType )
    , m_duration( 0 )
    , m_albumTrackId( 0 )
    , m_playCount( 0 )
    , m_showEpisodeId( 0 )
    , m_mrl( mrl )
{
}

FilePtr File::create( sqlite3* dbConnection, const std::string& mrl )
{
    auto self = std::make_shared<File>( mrl );
    static const std::string req = "INSERT INTO " + policy::FileTable::Name +
            " VALUES(NULL, ?, ?, ?, ?, ?, ?)";
    bool pKey = _Cache::insert( dbConnection, self, req, (int)self->m_type, self->m_duration,
        self->m_albumTrackId, self->m_playCount, self->m_showEpisodeId, self->m_mrl );
    if ( pKey == false )
        return nullptr;
    self->m_dbConnection = dbConnection;
    return self;
}

AlbumTrackPtr File::albumTrack()
{
    if ( m_albumTrack == nullptr && m_albumTrackId != 0 )
    {
        m_albumTrack = AlbumTrack::fetch( m_dbConnection, m_albumTrackId );
    }
    return m_albumTrack;
}

bool File::setAlbumTrack( AlbumTrackPtr albumTrack )
{
    static const std::string req = "UPDATE " + policy::FileTable::Name + " SET album_track_id = ? "
            "WHERE id_file = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, albumTrack->id(), m_id ) == false )
        return false;
    m_albumTrackId = albumTrack->id();
    m_albumTrack = albumTrack;
    return true;
}

unsigned int File::duration() const
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
    SqliteTools::fetchAll<Label>( m_dbConnection, req, labels, m_id );
    return labels;
}

int File::playCount() const
{
    return m_playCount;
}

const std::string& File::mrl() const
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
            "mrl TEXT UNIQUE ON CONFLICT FAIL"
            ")";
    return SqliteTools::executeRequest( connection, req );
}

bool File::addLabel( LabelPtr label )
{
    if ( m_dbConnection == nullptr || m_id == 0 || label->id() == 0)
    {
        std::cerr << "Both file & label need to be inserted in database before being linked together" << std::endl;
        return false;
    }
    const char* req = "INSERT INTO LabelFileRelation VALUES(?, ?)";
    return SqliteTools::executeRequest( m_dbConnection, req, label->id(), m_id );
}

bool File::removeLabel( LabelPtr label )
{
    if ( m_dbConnection == nullptr || m_id == 0 || label->id() == 0 )
    {
        std::cerr << "Can't unlink a label/file not inserted in database" << std::endl;
        return false;
    }
    const char* req = "DELETE FROM LabelFileRelation WHERE id_label = ? AND id_file = ?";
    return SqliteTools::executeDelete( m_dbConnection, req, label->id(), m_id );
}

const std::string& policy::FileCache::key(const std::shared_ptr<File> self )
{
    return self->mrl();
}

std::string policy::FileCache::key(sqlite3_stmt* stmt)
{
    return Traits<std::string>::Load( stmt, 6 );
}
