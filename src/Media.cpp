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
#include "utils/Filename.h"

const std::string policy::MediaTable::Name = "Media";
const std::string policy::MediaTable::PrimaryKeyColumn = "id_media";
unsigned int Media::* const policy::MediaTable::PrimaryKey = &Media::m_id;

Media::Media( DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
    , m_changed( false )
{
    row >> m_id
        >> m_type
        >> m_subType
        >> m_duration
        >> m_playCount
        >> m_progress
        >> m_rating
        >> m_mrl
        >> m_folderId
        >> m_lastModificationDate
        >> m_insertionDate
        >> m_thumbnail
        >> m_isParsed
        >> m_title
        >> m_isPresent
        >> m_isRemovable;
}

Media::Media( const fs::IFile* file, unsigned int folderId, const std::string& title, Type type, bool isRemovable )
    : m_id( 0 )
    , m_type( type )
    , m_subType( SubType::Unknown )
    , m_duration( -1 )
    , m_playCount( 0 )
    , m_progress( .0f )
    , m_rating( -1 )
    , m_mrl( isRemovable == true ? file->name() : file->fullPath() )
    , m_folderId( folderId )
    , m_lastModificationDate( file->lastModificationDate() )
    , m_insertionDate( time( nullptr ) )
    , m_isParsed( false )
    , m_title( title )
    , m_isPresent( true )
    , m_isRemovable( isRemovable )
    , m_changed( false )
{
}

std::shared_ptr<Media> Media::create( DBConnection dbConnection, Type type, const fs::IFile* file, unsigned int folderId, bool isRemovable )
{
    auto self = std::make_shared<Media>( file, folderId, file->name(), type, isRemovable );
    static const std::string req = "INSERT INTO " + policy::MediaTable::Name +
            "(type, mrl, folder_id, last_modification_date, insertion_date, title, is_removable) VALUES(?, ?, ?, ?, ?, ?, ?)";

    if ( insert( dbConnection, self, req, type, self->m_mrl, sqlite::ForeignKey( folderId ),
                         self->m_lastModificationDate, self->m_insertionDate, self->m_title, isRemovable ) == false )
        return nullptr;
    self->m_dbConnection = dbConnection;
    self->m_fullPath = file->fullPath();
    return self;
}

AlbumTrackPtr Media::albumTrack() const
{
    auto lock = m_albumTrack.lock();

    if ( m_albumTrack.isCached() == false && m_subType == SubType::AlbumTrack )
        m_albumTrack = AlbumTrack::fromMedia( m_dbConnection, m_id );
    return m_albumTrack.get();
}

void Media::setAlbumTrack( AlbumTrackPtr albumTrack )
{
    auto lock = m_albumTrack.lock();
    m_albumTrack = albumTrack;
    m_subType = SubType::AlbumTrack;
    m_changed = true;
}

int64_t Media::duration() const
{
    return m_duration;
}

void Media::setDuration( int64_t duration )
{
    if ( m_duration == duration )
        return;
    m_duration = duration;
    m_changed = true;
}

ShowEpisodePtr Media::showEpisode() const
{
    auto lock = m_showEpisode.lock();

    if ( m_showEpisode.isCached() == false && m_subType == SubType::ShowEpisode )
        m_showEpisode = ShowEpisode::fromMedia( m_dbConnection, m_id );
    return m_showEpisode.get();
}

void Media::setShowEpisode( ShowEpisodePtr episode )
{
    auto lock = m_showEpisode.lock();
    m_showEpisode = episode;
    m_subType = SubType::ShowEpisode;
    m_changed = true;
}

std::vector<LabelPtr> Media::labels()
{
    static const std::string req = "SELECT l.* FROM " + policy::LabelTable::Name + " l "
            "LEFT JOIN LabelFileRelation lfr ON lfr.label_id = l.id_label "
            "WHERE lfr.media_id = ?";
    return Label::fetchAll<ILabel>( m_dbConnection, req, m_id );
}

int Media::playCount() const
{
    return m_playCount;
}

void Media::increasePlayCount()
{
    m_playCount++;
    m_changed = true;
}

float Media::progress() const
{
    return m_progress;
}

void Media::setProgress( float progress )
{
    if ( progress == m_progress || progress < 0 || progress > 1.0 )
        return;
    m_progress = progress;
    m_changed = true;
}

int Media::rating() const
{
    return m_rating;
}

void Media::setRating( int rating )
{
    if ( m_rating == rating )
        return;
    m_rating = rating;
    m_changed = true;
}

const std::string& Media::mrl() const
{
    if ( m_isRemovable == false )
        return m_mrl;

    auto lock = m_fullPath.lock();
    if ( m_fullPath.isCached() )
        return m_fullPath;
    auto folder = Folder::fetch( m_dbConnection, m_folderId );
    if ( folder == nullptr )
        return m_mrl;
    m_fullPath = folder->path() + m_mrl;
    return m_fullPath;
}

MoviePtr Media::movie() const
{
    auto lock = m_movie.lock();

    if ( m_movie.isCached() == false && m_subType == SubType::Movie )
        m_movie = Movie::fromMedia( m_dbConnection, m_id );
    return m_movie.get();
}

void Media::setMovie(MoviePtr movie)
{
    auto lock = m_movie.lock();
    m_movie = movie;
    m_subType = SubType::Movie;
    m_changed = true;
}

bool Media::addVideoTrack(const std::string& codec, unsigned int width, unsigned int height, float fps)
{
    return VideoTrack::create( m_dbConnection, codec, width, height, fps, m_id ) != nullptr;
}

std::vector<VideoTrackPtr> Media::videoTracks()
{
    static const std::string req = "SELECT * FROM " + policy::VideoTrackTable::Name +
            " WHERE media_id = ?";
    return VideoTrack::fetchAll<IVideoTrack>( m_dbConnection, req, m_id );
}

bool Media::addAudioTrack( const std::string& codec, unsigned int bitrate,
                          unsigned int sampleRate, unsigned int nbChannels,
                          const std::string& language, const std::string& desc )
{
    return AudioTrack::create( m_dbConnection, codec, bitrate, sampleRate, nbChannels, language, desc, m_id ) != nullptr;
}

std::vector<AudioTrackPtr> Media::audioTracks()
{
    static const std::string req = "SELECT * FROM " + policy::AudioTrackTable::Name +
            " WHERE media_id = ?";
    return AudioTrack::fetchAll<IAudioTrack>( m_dbConnection, req, m_id );
}

const std::string &Media::thumbnail()
{
    return m_thumbnail;
}

unsigned int Media::insertionDate() const
{
    return m_insertionDate;
}

void Media::setThumbnail(const std::string& thumbnail )
{
    if ( m_thumbnail == thumbnail )
        return;
    m_thumbnail = thumbnail;
    m_changed = true;
}

bool Media::save()
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name + " SET "
            "type = ?, subtype = ?, duration = ?, play_count = ?, progress = ?, rating = ?,"
            "last_modification_date = ?, thumbnail = ?, parsed = ?,"
            "title = ? WHERE id_media = ?";
    if ( m_changed == false )
        return true;
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, m_type, m_subType, m_duration, m_playCount,
                                       m_progress, m_rating, m_lastModificationDate,
                                       m_thumbnail, m_isParsed, m_title, m_id ) == false )
    {
        return false;
    }
    m_changed = false;
    return true;
}

unsigned int Media::lastModificationDate()
{
    return m_lastModificationDate;
}

bool Media::isParsed() const
{
    return m_isParsed;
}

void Media::markParsed()
{
    if ( m_isParsed == true )
        return;
    m_isParsed = true;
    m_changed = true;
}

unsigned int Media::id() const
{
    return m_id;
}

IMedia::Type Media::type()
{
    return m_type;
}

void Media::setType( Type type )
{
    if ( m_type != type )
        return;
    m_type = type;
    m_changed = true;
}

const std::string &Media::title()
{
    return m_title;
}

void Media::setTitle( const std::string &title )
{
    if ( m_title == title )
        return;
    m_title = title;
    m_changed = true;
}

bool Media::createTable( DBConnection connection )
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::MediaTable::Name + "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "subtype INTEGER,"
            "duration INTEGER DEFAULT -1,"
            "play_count UNSIGNED INTEGER,"
            "progress REAL,"
            "rating INTEGER DEFAULT -1,"
            "mrl TEXT,"
            "folder_id UNSIGNED INTEGER,"
            "last_modification_date UNSIGNED INTEGER,"
            "insertion_date UNSIGNED INTEGER,"
            "thumbnail TEXT,"
            "parsed BOOLEAN NOT NULL DEFAULT 0,"
            "title TEXT,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "is_removable BOOLEAN NOT NULL,"
            "FOREIGN KEY (folder_id) REFERENCES " + policy::FolderTable::Name
            + "(id_folder) ON DELETE CASCADE,"
            "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL"
            ")";
    std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_folder_present AFTER UPDATE OF is_present ON "
            + policy::FolderTable::Name +
            " BEGIN"
            " UPDATE " + policy::MediaTable::Name + " SET is_present = new.is_present WHERE folder_id = new.id_folder;"
            " END";
    return sqlite::Tools::executeRequest( connection, req ) &&
            sqlite::Tools::executeRequest( connection, triggerReq );
}

bool Media::addLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Both file & label need to be inserted in database before being linked together" );
        return false;
    }
    const char* req = "INSERT INTO LabelFileRelation VALUES(?, ?)";
    return sqlite::Tools::insert( m_dbConnection, req, label->id(), m_id ) != 0;
}

bool Media::removeLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Can't unlink a label/file not inserted in database" );
        return false;
    }
    const char* req = "DELETE FROM LabelFileRelation WHERE label_id = ? AND media_id = ?";
    return sqlite::Tools::executeDelete( m_dbConnection, req, label->id(), m_id );
}
