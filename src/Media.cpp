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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "AudioTrack.h"
#include "Media.h"
#include "File.h"
#include "Folder.h"
#include "Label.h"
#include "logging/Logger.h"
#include "Movie.h"
#include "ShowEpisode.h"
#include "database/SqliteTools.h"
#include "VideoTrack.h"
#include "filesystem/IFile.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IDevice.h"
#include "utils/Filename.h"

namespace medialibrary
{

const std::string policy::MediaTable::Name = "Media";
const std::string policy::MediaTable::PrimaryKeyColumn = "id_media";
int64_t Media::* const policy::MediaTable::PrimaryKey = &Media::m_id;

Media::Media( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_changed( false )
{
    row >> m_id
        >> m_type
        >> m_subType
        >> m_duration
        >> m_playCount
        >> m_lastPlayedDate
        >> m_progress
        >> m_rating
        >> m_insertionDate
        >> m_releaseDate
        >> m_thumbnail
        >> m_title
        >> m_filename
        >> m_isFavorite
        >> m_isPresent;
}

Media::Media( MediaLibraryPtr ml, const std::string& title, Type type )
    : m_ml( ml )
    , m_id( 0 )
    , m_type( type )
    , m_subType( SubType::Unknown )
    , m_duration( -1 )
    , m_playCount( 0 )
    , m_lastPlayedDate( 0 )
    , m_progress( .0f )
    , m_rating( -1 )
    , m_insertionDate( time( nullptr ) )
    , m_releaseDate( 0 )
    , m_title( title )
    // When creating a Media, meta aren't parsed, and therefor, is the filename
    , m_filename( title )
    , m_isFavorite( false )
    , m_isPresent( true )
    , m_changed( false )
{
}

std::shared_ptr<Media> Media::create( MediaLibraryPtr ml, Type type, const fs::IFile& file )
{
    auto self = std::make_shared<Media>( ml, file.name(), type );
    static const std::string req = "INSERT INTO " + policy::MediaTable::Name +
            "(type, insertion_date, title, filename) VALUES(?, ?, ?, ?)";

    if ( insert( ml, self, req, type, self->m_insertionDate, self->m_title, self->m_filename ) == false )
        return nullptr;
    return self;
}

AlbumTrackPtr Media::albumTrack() const
{
    if ( m_subType != SubType::AlbumTrack )
        return nullptr;
    auto lock = m_albumTrack.lock();

    if ( m_albumTrack.isCached() == false )
        m_albumTrack = AlbumTrack::fromMedia( m_ml, m_id );
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
    if ( m_subType != SubType::ShowEpisode )
        return nullptr;

    auto lock = m_showEpisode.lock();
    if ( m_showEpisode.isCached() == false )
        m_showEpisode = ShowEpisode::fromMedia( m_ml, m_id );
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
            "INNER JOIN LabelFileRelation lfr ON lfr.label_id = l.id_label "
            "WHERE lfr.media_id = ?";
    return Label::fetchAll<ILabel>( m_ml, req, m_id );
}

int Media::playCount() const
{
    return m_playCount;
}

bool Media::increasePlayCount()
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name + " SET "
            "play_count = ?, last_played_date = ? WHERE id_media = ?";
    auto lastPlayedDate = time( nullptr );
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_playCount + 1, lastPlayedDate, m_id ) == false )
        return false;
    m_playCount++;
    m_lastPlayedDate = lastPlayedDate;
    return true;
}

float Media::progress() const
{
    return m_progress;
}

bool Media::setProgress( float progress )
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name + " SET progress = ? WHERE id_media = ?";
    if ( progress == m_progress || progress < 0 || progress > 1.0 )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, progress, m_id ) == false )
        return false;
    m_progress = progress;
    return true;
}

int Media::rating() const
{
    return m_rating;
}

bool Media::setRating( int rating )
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name + " SET rating = ? WHERE id_media = ?";
    if ( m_rating == rating )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, rating, m_id ) == false )
        return false;
    m_rating = rating;
    return true;
}

bool Media::isFavorite() const
{
    return m_isFavorite;
}

bool Media::setFavorite( bool favorite )
{
    static const std::string req = "UPDATE " + policy::MediaTable::Name + " SET is_favorite = ? WHERE id_media = ?";
    if ( m_isFavorite == favorite )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, favorite, m_id ) == false )
        return false;
    m_isFavorite = favorite;
    return true;
}

const std::vector<FilePtr>& Media::files() const
{
    auto lock = m_files.lock();
    if ( m_files.isCached() == false )
    {
        static const std::string req = "SELECT * FROM " + policy::FileTable::Name
                + " WHERE media_id = ?";
        m_files = File::fetchAll<IFile>( m_ml, req, m_id );
    }
    return m_files;
}

MoviePtr Media::movie() const
{
    if ( m_subType != SubType::Movie )
        return nullptr;

    auto lock = m_movie.lock();

    if ( m_movie.isCached() == false )
        m_movie = Movie::fromMedia( m_ml, m_id );
    return m_movie.get();
}

void Media::setMovie(MoviePtr movie)
{
    auto lock = m_movie.lock();
    m_movie = movie;
    m_subType = SubType::Movie;
    m_changed = true;
}

bool Media::addVideoTrack(const std::string& codec, unsigned int width, unsigned int height, float fps,
                          const std::string& language, const std::string& description )
{
    return VideoTrack::create( m_ml, codec, width, height, fps, m_id, language, description ) != nullptr;
}

std::vector<VideoTrackPtr> Media::videoTracks()
{
    static const std::string req = "SELECT * FROM " + policy::VideoTrackTable::Name +
            " WHERE media_id = ?";
    return VideoTrack::fetchAll<IVideoTrack>( m_ml, req, m_id );
}

bool Media::addAudioTrack( const std::string& codec, unsigned int bitrate,
                          unsigned int sampleRate, unsigned int nbChannels,
                          const std::string& language, const std::string& desc )
{
    return AudioTrack::create( m_ml, codec, bitrate, sampleRate, nbChannels, language, desc, m_id ) != nullptr;
}

std::vector<AudioTrackPtr> Media::audioTracks()
{
    static const std::string req = "SELECT * FROM " + policy::AudioTrackTable::Name +
            " WHERE media_id = ?";
    return AudioTrack::fetchAll<IAudioTrack>( m_ml, req, m_id );
}

const std::string &Media::thumbnail()
{
    return m_thumbnail;
}

unsigned int Media::insertionDate() const
{
    return m_insertionDate;
}

unsigned int Media::releaseDate() const
{
    return m_releaseDate;
}

void Media::setReleaseDate( unsigned int date )
{
    if ( m_releaseDate == date )
        return;
    m_releaseDate = date;
    m_changed = true;
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
            "type = ?, subtype = ?, duration = ?, progress = ?, release_date = ?,"
            "thumbnail = ?, title = ? WHERE id_media = ?";
    if ( m_changed == false )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_type, m_subType, m_duration,
                                       m_progress, m_releaseDate, m_thumbnail, m_title, m_id ) == false )
    {
        return false;
    }
    m_changed = false;
    return true;
}

std::shared_ptr<File> Media::addFile( const fs::IFile& fileFs, Folder& parentFolder, fs::IDirectory& parentFolderFs, IFile::Type type )
{
    auto file = File::create( m_ml, m_id, type, fileFs, parentFolder.id(), parentFolderFs.device()->isRemovable() );
    if ( file == nullptr )
        return nullptr;
    auto lock = m_files.lock();
    if ( m_files.isCached() )
        m_files.get().push_back( file );
    return file;
}

void Media::removeFile( File& file )
{
    file.destroy();
    auto lock = m_files.lock();
    if ( m_files.isCached() == false )
        return;
    m_files.get().erase( std::remove_if( begin( m_files.get() ), end( m_files.get() ), [&file]( const FilePtr& f ) {
        return f->id() == file.id();
    }));
}

std::vector<MediaPtr> Media::listAll( MediaLibraryPtr ml, IMedia::Type type, SortingCriteria sort, bool desc )
{
    std::string req;
    if ( sort == SortingCriteria::LastModificationDate || sort == SortingCriteria::FileSize )
    {
        req = "SELECT m.* FROM " + policy::MediaTable::Name + " m INNER JOIN "
                + policy::FileTable::Name + " f ON m.id_media = f.media_id"
                " WHERE m.type = ?"
                " AND ( f.type = ? OR f.type = ? )";
        if ( sort == SortingCriteria::LastModificationDate )
            req += " ORDER BY f.last_modification_date";
        else
            req += " ORDER BY f.size";
        if ( desc == true )
            req += " DESC";
        return fetchAll<IMedia>( ml, req, type, File::Type::Entire, File::Type::Main );
    }
    req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE type = ? AND is_present = 1 ORDER BY ";
    switch ( sort )
    {
    case SortingCriteria::Duration:
        req += "duration";
        break;
    case SortingCriteria::InsertionDate:
        req += "insertion_date";
        break;
    case SortingCriteria::ReleaseDate:
        req += "release_date";
        break;
    default:
        req += "title";
        break;
    }
    if ( desc == true )
        req += " DESC";

    return fetchAll<IMedia>( ml, req, type );
}

int64_t Media::id() const
{
    return m_id;
}

IMedia::Type Media::type()
{
    return m_type;
}

IMedia::SubType Media::subType() const
{
    return m_subType;
}

void Media::setType( Type type )
{
    if ( m_type == type )
        return;
    m_type = type;
    m_changed = true;
}

const std::string &Media::title() const
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
            "last_played_date UNSIGNED INTEGER,"
            "progress REAL,"
            "rating INTEGER DEFAULT -1,"
            "insertion_date UNSIGNED INTEGER,"
            "release_date UNSIGNED INTEGER,"
            "thumbnail TEXT,"
            "title TEXT COLLATE NOCASE,"
            "filename TEXT,"
            "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
            "is_present BOOLEAN NOT NULL DEFAULT 1"
            ")";
    const std::string indexReq = "CREATE INDEX IF NOT EXISTS index_last_played_date ON "
            + policy::MediaTable::Name + "(last_played_date DESC)";
    const std::string vtableReq = "CREATE VIRTUAL TABLE IF NOT EXISTS "
                + policy::MediaTable::Name + "Fts USING FTS3("
                "title,"
                "labels"
            ")";
    return sqlite::Tools::executeRequest( connection, req ) &&
            sqlite::Tools::executeRequest( connection, indexReq ) &&
            sqlite::Tools::executeRequest( connection, vtableReq );
}

bool Media::createTriggers( DBConnection connection )
{
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS has_files_present AFTER UPDATE OF "
            "is_present ON " + policy::FileTable::Name +
            " BEGIN "
            " UPDATE " + policy::MediaTable::Name + " SET is_present="
                "(SELECT COUNT(id_file) FROM " + policy::FileTable::Name + " WHERE media_id=new.media_id AND is_present=1) "
                "WHERE id_media=new.media_id;"
            " END;";
    static const std::string triggerReq2 = "CREATE TRIGGER IF NOT EXISTS cascade_file_deletion AFTER DELETE ON "
            + policy::FileTable::Name +
            " BEGIN "
            " DELETE FROM " + policy::MediaTable::Name + " WHERE "
                "(SELECT COUNT(id_file) FROM " + policy::FileTable::Name + " WHERE media_id=old.media_id) = 0"
                " AND id_media=old.media_id;"
            " END;";

    static const std::string vtableInsertTrigger = "CREATE TRIGGER IF NOT EXISTS insert_media_fts"
            " AFTER INSERT ON " + policy::MediaTable::Name +
            " BEGIN"
            " INSERT INTO " + policy::MediaTable::Name + "Fts(rowid,title,labels) VALUES(new.id_media, new.title, '');"
            " END";
    static const std::string vtableDeleteTrigger = "CREATE TRIGGER IF NOT EXISTS delete_media_fts"
            " BEFORE DELETE ON " + policy::MediaTable::Name +
            " BEGIN"
            " DELETE FROM " + policy::MediaTable::Name + "Fts WHERE rowid = old.id_media;"
            " END";
    static const std::string vtableUpdateTitleTrigger2 = "CREATE TRIGGER IF NOT EXISTS update_media_title_fts"
              " AFTER UPDATE OF title ON " + policy::MediaTable::Name +
              " BEGIN"
              " UPDATE " + policy::MediaTable::Name + "Fts SET title = new.title WHERE rowid = new.id_media;"
              " END";
    return sqlite::Tools::executeRequest( connection, triggerReq ) &&
            sqlite::Tools::executeRequest( connection, triggerReq2 ) &&
            sqlite::Tools::executeRequest( connection, vtableInsertTrigger ) &&
            sqlite::Tools::executeRequest( connection, vtableDeleteTrigger ) &&
            sqlite::Tools::executeRequest( connection, vtableUpdateTitleTrigger2 );
}

bool Media::addLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Both file & label need to be inserted in database before being linked together" );
        return false;
    }
    const char* req = "INSERT INTO LabelFileRelation VALUES(?, ?)";
    if ( sqlite::Tools::executeInsert( m_ml->getConn(), req, label->id(), m_id ) == 0 )
        return false;
    const std::string reqFts = "UPDATE " + policy::MediaTable::Name + "Fts "
        "SET labels = labels || ' ' || ? WHERE rowid = ?";
    return sqlite::Tools::executeUpdate( m_ml->getConn(), reqFts, label->name(), m_id );
}

bool Media::removeLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Can't unlink a label/file not inserted in database" );
        return false;
    }
    const char* req = "DELETE FROM LabelFileRelation WHERE label_id = ? AND media_id = ?";
    if ( sqlite::Tools::executeDelete( m_ml->getConn(), req, label->id(), m_id ) == false )
        return false;
    const std::string reqFts = "UPDATE " + policy::MediaTable::Name + "Fts "
            "SET labels = TRIM(REPLACE(labels, ?, '')) WHERE rowid = ?";
    return sqlite::Tools::executeUpdate( m_ml->getConn(), reqFts, label->name(), m_id );
}


std::vector<MediaPtr> Media::search( MediaLibraryPtr ml, const std::string& title )
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE"
            " id_media IN (SELECT rowid FROM " + policy::MediaTable::Name + "Fts"
            " WHERE " + policy::MediaTable::Name + "Fts MATCH '*' || ? || '*')"
            "AND is_present = 1";
    return Media::fetchAll<IMedia>( ml, req, title );
}

std::vector<MediaPtr> Media::fetchHistory( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE last_played_date IS NOT NULL"
            " ORDER BY last_played_date DESC LIMIT 100";
    return fetchAll<IMedia>( ml, req );
}

void Media::clearHistory( MediaLibraryPtr ml )
{
    auto dbConn = ml->getConn();
    static const std::string req = "UPDATE " + policy::MediaTable::Name + " SET "
            "play_count = 0,"
            "last_played_date = NULL,"
            "progress = 0";
    // Clear the entire cache since quite a few items are now containing invalid info.
    clear();
    sqlite::Tools::executeUpdate( dbConn, req );
}

}
