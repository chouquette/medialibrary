#include <cassert>
#include <cstdlib>
#include <cstring>

#include "Album.h"
#include "AlbumTrack.h"
#include "AudioTrack.h"
#include "File.h"
#include "Folder.h"
#include "Label.h"
#include "Movie.h"
#include "ShowEpisode.h"
#include "database/SqliteTools.h"
#include "VideoTrack.h"

const std::string policy::FileTable::Name = "File";
const std::string policy::FileTable::CacheColumn = "mrl";
unsigned int File::* const policy::FileTable::PrimaryKey = &File::m_id;

File::File( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_type = (Type)sqlite3_column_int( stmt, 1 );
    m_duration = sqlite3_column_int( stmt, 2 );
    m_albumTrackId = sqlite3_column_int( stmt, 3 );
    m_playCount = sqlite3_column_int( stmt, 4 );
    m_showEpisodeId = sqlite3_column_int( stmt, 5 );
    m_mrl = (const char*)sqlite3_column_text( stmt, 6 );
    m_movieId = sqlite::Traits<unsigned int>::Load( stmt, 7 );
    m_folderId = sqlite::Traits<unsigned int>::Load( stmt, 8 );

    m_isReady = m_type != UnknownType;
}

File::File( const std::string& mrl, unsigned int folderId )
    : m_id( 0 )
    , m_type( UnknownType )
    , m_duration( 0 )
    , m_albumTrackId( 0 )
    , m_playCount( 0 )
    , m_showEpisodeId( 0 )
    , m_mrl( mrl )
    , m_movieId( 0 )
    , m_folderId( folderId )
    , m_isReady( false )
{
}

FilePtr File::create( DBConnection dbConnection, const std::string& mrl, unsigned int folderId )
{
    auto self = std::make_shared<File>( mrl, folderId );
    static const std::string req = "INSERT INTO " + policy::FileTable::Name +
            "(mrl, folder_id) VALUES(?, ?)";

    if ( _Cache::insert( dbConnection, self, req, mrl, sqlite::ForeignKey( folderId ) ) == false )
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
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, albumTrack->id(), m_id ) == false )
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

bool File::setShowEpisode(ShowEpisodePtr showEpisode)
{
    static const std::string req = "UPDATE " + policy::FileTable::Name
            + " SET show_episode_id = ?  WHERE id_file = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, showEpisode->id(), m_id ) == false )
        return false;
    m_showEpisodeId = showEpisode->id();
    m_showEpisode = showEpisode;
    return true;
}

std::vector<std::shared_ptr<ILabel> > File::labels()
{
    static const std::string req = "SELECT l.* FROM " + policy::LabelTable::Name + " l "
            "LEFT JOIN LabelFileRelation lfr ON lfr.id_label = l.id_label "
            "WHERE lfr.id_file = ?";
    return sqlite::Tools::fetchAll<Label, ILabel>( m_dbConnection, req, m_id );
}

int File::playCount() const
{
    return m_playCount;
}

const std::string& File::mrl() const
{
    return m_mrl;
}

MoviePtr File::movie()
{
    if ( m_movie == nullptr && m_movieId != 0 )
    {
        m_movie = Movie::fetch( m_dbConnection, m_movieId );
    }
    return m_movie;
}

bool File::setMovie( MoviePtr movie )
{
    static const std::string req = "UPDATE " + policy::FileTable::Name
            + " SET movie_id = ? WHERE id_file = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, movie->id(), m_id ) == false )
        return false;
    m_movie = movie;
    m_movieId = movie->id();
    return true;
}

bool File::addVideoTrack(const std::string& codec, unsigned int width, unsigned int height, float fps)
{
    static const std::string req = "INSERT INTO VideoTrackFileRelation VALUES(?, ?)";

    auto track = VideoTrack::fetch( m_dbConnection, codec, width, height, fps );
    if ( track == nullptr )
    {
        track = VideoTrack::create( m_dbConnection, codec, width, height, fps );
        if ( track == nullptr )
            return false;
    }
    return sqlite::Tools::executeRequest( m_dbConnection, req, track->id(), m_id );
}

std::vector<VideoTrackPtr> File::videoTracks()
{
    static const std::string req = "SELECT t.* FROM " + policy::VideoTrackTable::Name +
            " t LEFT JOIN VideoTrackFileRelation vtfr ON vtfr.id_track = t.id_track"
            " WHERE vtfr.id_file = ?";
    return sqlite::Tools::fetchAll<VideoTrack, IVideoTrack>( m_dbConnection, req, m_id );
}

bool File::addAudioTrack( const std::string& codec, unsigned int bitrate,
                          unsigned int sampleRate, unsigned int nbChannels )
{
    static const std::string req = "INSERT INTO AudioTrackFileRelation VALUES(?, ?)";

    auto track = AudioTrack::fetch( m_dbConnection, codec, bitrate, sampleRate, nbChannels );
    if ( track == nullptr )
    {
        track = AudioTrack::create( m_dbConnection, codec, bitrate, sampleRate, nbChannels );
        if ( track == nullptr )
            return false;
    }
    return sqlite::Tools::executeRequest( m_dbConnection, req, track->id(), m_id );
}

std::vector<AudioTrackPtr> File::audioTracks()
{
    static const std::string req = "SELECT t.* FROM " + policy::AudioTrackTable::Name +
            " t LEFT JOIN AudioTrackFileRelation atfr ON atfr.id_track = t.id_track"
            " WHERE atfr.id_file = ?";
    return sqlite::Tools::fetchAll<AudioTrack, IAudioTrack>( m_dbConnection, req, m_id );
}

bool File::isStandAlone()
{
    return m_folderId == 0;
}

bool File::isReady() const
{
    return m_isReady;
}

void File::setReady()
{
    assert( m_isReady == false );
    m_isReady = true;
}

unsigned int File::id() const
{
    return m_id;
}

bool File::createTable( DBConnection connection )
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FileTable::Name + "("
            "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "duration UNSIGNED INTEGER,"
            "album_track_id UNSIGNED INTEGER,"
            "play_count UNSIGNED INTEGER,"
            "show_episode_id UNSIGNED INTEGER,"
            "mrl TEXT UNIQUE ON CONFLICT FAIL,"
            "movie_id UNSIGNED INTEGER,"
            "folder_id UNSIGNED INTEGER,"
            "FOREIGN KEY (album_track_id) REFERENCES " + policy::AlbumTrackTable::Name
            + "(id_track) ON DELETE CASCADE,"
            "FOREIGN KEY (show_episode_id) REFERENCES " + policy::ShowEpisodeTable::Name
            + "(id_episode) ON DELETE CASCADE,"
            "FOREIGN KEY (movie_id) REFERENCES " + policy::MovieTable::Name
            + "(id_movie) ON DELETE CASCADE,"
            "FOREIGN KEY (folder_id) REFERENCES " + policy::FolderTable::Name
            + "(id_folder) ON DELETE CASCADE"
            ")";
    if ( sqlite::Tools::executeRequest( connection, req ) == false )
        return false;
    req = "CREATE TABLE IF NOT EXISTS VideoTrackFileRelation("
                "id_track INTEGER,"
                "id_file INTEGER,"
                "PRIMARY KEY ( id_track, id_file ), "
                "FOREIGN KEY ( id_track ) REFERENCES " + policy::VideoTrackTable::Name +
                    "(id_track) ON DELETE CASCADE,"
                "FOREIGN KEY ( id_file ) REFERENCES " + policy::FileTable::Name
                + "(id_file) ON DELETE CASCADE"
            ")";
    if ( sqlite::Tools::executeRequest( connection, req ) == false )
        return false;
    req = "CREATE TABLE IF NOT EXISTS AudioTrackFileRelation("
                "id_track INTEGER,"
                "id_file INTEGER,"
                "PRIMARY KEY ( id_track, id_file ), "
                "FOREIGN KEY ( id_track ) REFERENCES " + policy::AudioTrackTable::Name +
                    "(id_track) ON DELETE CASCADE,"
                "FOREIGN KEY ( id_file ) REFERENCES " + policy::FileTable::Name
                + "(id_file) ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( connection, req );
}

bool File::addLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        std::cerr << "Both file & label need to be inserted in database before being linked together" << std::endl;
        return false;
    }
    const char* req = "INSERT INTO LabelFileRelation VALUES(?, ?)";
    return sqlite::Tools::executeRequest( m_dbConnection, req, label->id(), m_id );
}

bool File::removeLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        std::cerr << "Can't unlink a label/file not inserted in database" << std::endl;
        return false;
    }
    const char* req = "DELETE FROM LabelFileRelation WHERE id_label = ? AND id_file = ?";
    return sqlite::Tools::executeDelete( m_dbConnection, req, label->id(), m_id );
}

const std::string& policy::FileCache::key(const std::shared_ptr<File> self )
{
    return self->mrl();
}

std::string policy::FileCache::key(sqlite3_stmt* stmt)
{
    return sqlite::Traits<std::string>::Load( stmt, 6 );
}
