/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "AudioTrack.h"
#include "Media.h"
#include "Folder.h"
#include "Label.h"
#include "logging/Logger.h"
#include "Movie.h"
#include "ShowEpisode.h"
#include "database/SqliteTools.h"
#include "VideoTrack.h"
#include "filesystem/IFile.h"

const std::string policy::MediaTable::Name = "Media";
const std::string policy::MediaTable::CacheColumn = "mrl";
unsigned int Media::* const policy::MediaTable::PrimaryKey = &Media::m_id;

Media::Media( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_type = (Type)sqlite3_column_int( stmt, 1 );
    m_duration = sqlite::Traits<int64_t>::Load( stmt, 2 );
    m_playCount = sqlite3_column_int( stmt, 3 );
    m_showEpisodeId = sqlite3_column_int( stmt, 4 );
    m_mrl = (const char*)sqlite3_column_text( stmt, 5 );
    m_movieId = sqlite::Traits<unsigned int>::Load( stmt, 6 );
    m_folderId = sqlite::Traits<unsigned int>::Load( stmt, 7 );
    m_lastModificationDate = sqlite::Traits<unsigned int>::Load( stmt, 8 );
    m_snapshot = sqlite::Traits<std::string>::Load( stmt, 9 );
    m_isParsed = sqlite::Traits<bool>::Load( stmt, 10 );
    m_title = sqlite::Traits<std::string>::Load( stmt, 11 );
}

Media::Media( const fs::IFile* file, unsigned int folderId, const std::string& title, Type type )
    : m_id( 0 )
    , m_type( type )
    , m_duration( -1 )
    , m_playCount( 0 )
    , m_showEpisodeId( 0 )
    , m_mrl( file->fullPath() )
    , m_movieId( 0 )
    , m_folderId( folderId )
    , m_lastModificationDate( file->lastModificationDate() )
    , m_isParsed( false )
    , m_title( title )
{
}

std::shared_ptr<Media> Media::create( DBConnection dbConnection, Type type, const fs::IFile* file, unsigned int folderId )
{
    auto self = std::make_shared<Media>( file, folderId, file->name(), type );
    static const std::string req = "INSERT INTO " + policy::MediaTable::Name +
            "(type, mrl, folder_id, last_modification_date, title) VALUES(?, ?, ?, ?, ?)";

    using type_t = std::underlying_type<Type>::type;
    if ( _Cache::insert( dbConnection, self, req, static_cast<type_t>( type ), self->m_mrl, sqlite::ForeignKey( folderId ),
                         self->m_lastModificationDate, self->m_title) == false )
        return nullptr;
    self->m_dbConnection = dbConnection;
    return self;
}

AlbumTrackPtr Media::albumTrack()
{
    if ( m_albumTrack == nullptr )
    {
        std::string req = "SELECT * FROM " + policy::AlbumTrackTable::Name +
                " WHERE media_id = ?";
        m_albumTrack = AlbumTrack::fetchOne( m_dbConnection, req, m_id );
    }
    return m_albumTrack;
}

bool Media::setAlbumTrack( AlbumTrackPtr albumTrack )
{
    m_albumTrack = albumTrack;
    return true;
}

bool Media::addArtist( ArtistPtr artist )
{
    static const std::string req = "INSERT INTO MediaArtistRelation VALUES(?, ?)";
    // If track's ID is 0, the request will fail due to table constraints
    sqlite::ForeignKey artistForeignKey( artist != nullptr ? artist->id() : 0 );
    return sqlite::Tools::executeRequest( m_dbConnection, req, m_id, artistForeignKey );
}

std::vector<ArtistPtr> Media::artists() const
{
    static const std::string req = "SELECT art.* FROM " + policy::ArtistTable::Name + " art "
            "LEFT JOIN MediaArtistRelation mar ON mar.id_artist = art.id_artist "
            "WHERE mar.id_media = ?";
    return Artist::fetchAll( m_dbConnection, req, m_id );
}

int64_t Media::duration() const
{
    return m_duration;
}

bool Media::setDuration( int64_t duration )
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name + " SET duration = ? "
            "WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, duration, m_id ) == false )
        return false;
    m_duration = duration;
    return true;
}

std::shared_ptr<IShowEpisode> Media::showEpisode()
{
    if ( m_showEpisode == nullptr && m_showEpisodeId != 0 )
    {
        m_showEpisode = ShowEpisode::fetch( m_dbConnection, m_showEpisodeId );
    }
    return m_showEpisode;
}

bool Media::setShowEpisode(ShowEpisodePtr showEpisode)
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name
            + " SET show_episode_id = ?  WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, showEpisode->id(), m_id ) == false )
        return false;
    m_showEpisodeId = showEpisode->id();
    m_showEpisode = showEpisode;
    return true;
}

std::vector<std::shared_ptr<ILabel> > Media::labels()
{
    static const std::string req = "SELECT l.* FROM " + policy::LabelTable::Name + " l "
            "LEFT JOIN LabelFileRelation lfr ON lfr.id_label = l.id_label "
            "WHERE lfr.id_media = ?";
    return Label::fetchAll( m_dbConnection, req, m_id );
}

int Media::playCount() const
{
    return m_playCount;
}

const std::string& Media::mrl() const
{
    return m_mrl;
}

MoviePtr Media::movie()
{
    if ( m_movie == nullptr && m_movieId != 0 )
    {
        m_movie = Movie::fetch( m_dbConnection, m_movieId );
    }
    return m_movie;
}

bool Media::setMovie( MoviePtr movie )
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name
            + " SET movie_id = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, movie->id(), m_id ) == false )
        return false;
    m_movie = movie;
    m_movieId = movie->id();
    return true;
}

bool Media::addVideoTrack(const std::string& codec, unsigned int width, unsigned int height, float fps)
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

std::vector<VideoTrackPtr> Media::videoTracks()
{
    static const std::string req = "SELECT t.* FROM " + policy::VideoTrackTable::Name +
            " t LEFT JOIN VideoTrackFileRelation vtfr ON vtfr.id_track = t.id_track"
            " WHERE vtfr.id_media = ?";
    return VideoTrack::fetchAll( m_dbConnection, req, m_id );
}

bool Media::addAudioTrack( const std::string& codec, unsigned int bitrate,
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

std::vector<AudioTrackPtr> Media::audioTracks()
{
    static const std::string req = "SELECT t.* FROM " + policy::AudioTrackTable::Name +
            " t LEFT JOIN AudioTrackFileRelation atfr ON atfr.id_track = t.id_track"
            " WHERE atfr.id_media = ?";
    return AudioTrack::fetchAll( m_dbConnection, req, m_id );
}

const std::string &Media::snapshot()
{
    return m_snapshot;
}

bool Media::setSnapshot(const std::string &snapshot)
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name
            + " SET snapshot = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, snapshot, m_id ) == false )
        return false;
    m_snapshot = snapshot;
    return true;
}

bool Media::isStandAlone()
{
    return m_folderId == 0;
}

unsigned int Media::lastModificationDate()
{
    return m_lastModificationDate;
}

bool Media::isParsed() const
{
    return m_isParsed;
}

bool Media::markParsed()
{
    if ( m_isParsed == true )
        return true;
    static const std::string req = "UPDATE " + policy::MediaTable::Name
            + " SET parsed = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, true, m_id ) == false )
        return false;
    m_isParsed = true;
    return true;
}

unsigned int Media::id() const
{
    return m_id;
}

IMedia::Type Media::type()
{
    return m_type;
}

bool Media::setType( Type type )
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name
            + " SET type = ? WHERE id_media = ?";
    // We need to convert to an integer representation for the sqlite traits to work properly
    using type_t = std::underlying_type<Type>::type;
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, static_cast<type_t>( type ), m_id ) == false )
        return false;
    m_type = type;
    return true;
}

const std::string &Media::title()
{
    return m_title;
}

bool Media::setTitle( const std::string &title )
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name
            + " SET title = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, title, m_id ) == false )
        return false;
    m_title = title;
    return true;
}

bool Media::createTable( DBConnection connection )
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::MediaTable::Name + "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "duration INTEGER,"
            "play_count UNSIGNED INTEGER,"
            "show_episode_id UNSIGNED INTEGER,"
            "mrl TEXT UNIQUE ON CONFLICT FAIL,"
            "movie_id UNSIGNED INTEGER,"
            "folder_id UNSIGNED INTEGER,"
            "last_modification_date UNSIGNED INTEGER,"
            "snapshot TEXT,"
            "parsed BOOLEAN,"
            "title TEXT,"
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
                "id_media INTEGER,"
                "PRIMARY KEY ( id_track, id_media ), "
                "FOREIGN KEY ( id_track ) REFERENCES " + policy::VideoTrackTable::Name +
                    "(id_track) ON DELETE CASCADE,"
                "FOREIGN KEY ( id_media ) REFERENCES " + policy::MediaTable::Name
                + "(id_media) ON DELETE CASCADE"
            ")";
    if ( sqlite::Tools::executeRequest( connection, req ) == false )
        return false;
    req = "CREATE TABLE IF NOT EXISTS AudioTrackFileRelation("
                "id_track INTEGER,"
                "id_media INTEGER,"
                "PRIMARY KEY ( id_track, id_media ), "
                "FOREIGN KEY ( id_track ) REFERENCES " + policy::AudioTrackTable::Name +
                    "(id_track) ON DELETE CASCADE,"
                "FOREIGN KEY ( id_media ) REFERENCES " + policy::MediaTable::Name
                + "(id_media) ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( connection, req );
}

bool Media::addLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Both file & label need to be inserted in database before being linked together" );
        return false;
    }
    const char* req = "INSERT INTO LabelFileRelation VALUES(?, ?)";
    return sqlite::Tools::executeRequest( m_dbConnection, req, label->id(), m_id );
}

bool Media::removeLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Can't unlink a label/file not inserted in database" );
        return false;
    }
    const char* req = "DELETE FROM LabelFileRelation WHERE id_label = ? AND id_media = ?";
    return sqlite::Tools::executeDelete( m_dbConnection, req, label->id(), m_id );
}

const std::string& policy::MediaCache::key(const std::shared_ptr<Media> self )
{
    return self->mrl();
}

std::string policy::MediaCache::key(sqlite3_stmt* stmt)
{
    return sqlite::Traits<std::string>::Load( stmt, 5 );
}
