/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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
#include <ctime>

#include "Album.h"
#include "Artist.h"
#include "Bookmark.h"
#include "AudioTrack.h"
#include "Chapter.h"
#include "Device.h"
#include "Media.h"
#include "File.h"
#include "Folder.h"
#include "Label.h"
#include "logging/Logger.h"
#include "Movie.h"
#include "ShowEpisode.h"
#include "SubtitleTrack.h"
#include "Playlist.h"
#include "MediaGroup.h"
#include "parser/Task.h"
#include "parser/Parser.h"
#include "Genre.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"
#include "VideoTrack.h"
#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/Errors.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "thumbnails/ThumbnailerWorker.h"
#include "utils/ModificationsNotifier.h"
#include "utils/Enums.h"

namespace medialibrary
{

const std::string Media::Table::Name = "Media";
const std::string Media::Table::PrimaryKeyColumn = "id_media";
int64_t Media::* const Media::Table::PrimaryKey = &Media::m_id;
const std::string Media::FtsTable::Name = "MediaFts";

Media::Media( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    // DB field extraction:
    , m_id( row.extract<decltype(m_id)>() )
    , m_type( row.extract<decltype(m_type)>() )
    , m_subType( row.extract<decltype(m_subType)>() )
    , m_duration( row.extract<decltype(m_duration)>() )
    , m_lastPosition( row.extract<decltype(m_lastPosition)>() )
    , m_lastTime( row.extract<decltype(m_lastTime)>() )
    , m_playCount( row.extract<decltype(m_playCount)>() )
    , m_lastPlayedDate( row.extract<decltype(m_lastPlayedDate)>() )
    , m_insertionDate( row.extract<decltype(m_insertionDate)>() )
    , m_releaseDate( row.extract<decltype(m_releaseDate)>() )
    , m_title( row.extract<decltype(m_title)>() )
    , m_filename( row.extract<decltype(m_filename)>() )
    , m_isFavorite( row.extract<decltype(m_isFavorite)>() )
    , m_isPresent( row.extract<decltype(m_isPresent)>() )
    , m_deviceId( row.extract<decltype(m_deviceId)>() )
    , m_nbPlaylists( row.extract<unsigned int>() )
    , m_folderId( row.extract<decltype(m_folderId)>() )
    , m_importType( row.extract<decltype(m_importType)>() )
    , m_groupId( row.extract<decltype(m_groupId)>() )
    , m_forcedTitle( row.extract<decltype(m_forcedTitle)>() )
    , m_artistId( row.extract<decltype(m_artistId)>() )
    , m_genreId( row.extract<decltype(m_genreId)>() )
    , m_trackNumber( row.extract<decltype(m_trackNumber)>() )
    , m_albumId( row.extract<decltype(m_albumId)>() )
    , m_discNumber( row.extract<decltype(m_discNumber)>() )
    , m_lyrics( row.extract<decltype(m_lyrics)>() )

    // End of DB fields extraction
    , m_metadata( m_ml, IMetadata::EntityType::Media )
{
    assert( row.hasRemainingColumns() == false );
}

Media::Media( MediaLibraryPtr ml, const std::string& title, Type type,
              int64_t duration, int64_t deviceId, int64_t folderId )
    : m_ml( ml )
    , m_id( 0 )
    , m_type( type )
    , m_subType( SubType::Unknown )
    , m_duration( duration )
    , m_lastPosition( -1.f )
    , m_lastTime( -1 )
    , m_playCount( 0 )
    , m_lastPlayedDate( 0 )
    , m_insertionDate( time( nullptr ) )
    , m_releaseDate( 0 )
    , m_title( title )
    // When creating a Media, meta aren't parsed, and therefor, the title is the filename
    , m_filename( title )
    , m_isFavorite( false )
    , m_isPresent( true )
    , m_deviceId( deviceId )
    , m_nbPlaylists( 0 )
    , m_folderId( folderId )
    , m_importType( ImportType::Internal )
    , m_groupId( 0 )
    , m_forcedTitle( false )
    , m_artistId( 0 )
    , m_genreId( 0 )
    , m_trackNumber( 0 )
    , m_albumId( 0 )
    , m_discNumber( 0 )
    , m_metadata( m_ml, IMetadata::EntityType::Media )
{
}

Media::Media( MediaLibraryPtr ml, const std::string& fileName,
              ImportType importType , int64_t duration )
    : m_ml( ml )
    , m_id( 0 )
    , m_type( Type::Unknown )
    , m_subType( SubType::Unknown )
    , m_duration( duration )
    , m_lastPosition( -1.f )
    , m_lastTime( -1 )
    , m_playCount( 0 )
    , m_lastPlayedDate( 0 )
    , m_insertionDate( time( nullptr ) )
    , m_releaseDate( 0 )
    , m_title( fileName )
    , m_filename( fileName )
    , m_isFavorite( false )
    , m_isPresent( true )
    , m_deviceId( 0 )
    , m_nbPlaylists( 0 )
    , m_folderId( 0 )
    , m_importType( importType )
    , m_groupId( 0 )
    , m_forcedTitle( false )
    , m_artistId( 0 )
    , m_genreId( 0 )
    , m_trackNumber( 0 )
    , m_albumId( 0 )
    , m_discNumber( 0 )
    , m_metadata( m_ml, IMetadata::EntityType::Media )
{
}

std::shared_ptr<Media> Media::create( MediaLibraryPtr ml, Type type,
                                      int64_t deviceId, int64_t folderId,
                                      const std::string& fileName,
                                      int64_t duration )
{
    auto self = std::make_shared<Media>( ml, fileName, type, duration, deviceId,
                                         folderId );
    static const std::string req = "INSERT INTO " + Media::Table::Name +
            "(type, duration, insertion_date, title, filename, device_id, "
            "folder_id, import_type) "
            "VALUES(?, ?, ?, ?, ?, ?, ?, ?)";

    if ( insert( ml, self, req, type, self->m_duration, self->m_insertionDate,
                 self->m_title, self->m_filename, deviceId, folderId,
                 ImportType::Internal ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<Media> Media::createExternalMedia( MediaLibraryPtr ml,
                                                   const std::string& mrl,
                                                   ImportType importType,
                                                   int64_t duration )
{
    auto t = ml->getConn()->newTransaction();

    if ( duration <= 0 )
        duration = -1;
    auto fileName = utils::url::decode( utils::file::fileName( mrl ) );
    auto self = std::make_shared<Media>( ml, fileName, importType, duration );
    static const std::string req = "INSERT INTO " + Media::Table::Name +
            "(type, duration, insertion_date, title, filename, import_type) "
            "VALUES(?, ?, ?, ?, ?, ?)";

    if ( insert( ml, self, req, Type::Unknown, duration, self->m_insertionDate,
                 self->m_title, self->m_filename, importType ) == false )
        return nullptr;

    if ( self->addExternalMrl( mrl, IFile::Type::Main ) == nullptr )
        return nullptr;

    t->commit();
    return self;
}

std::shared_ptr<Media> Media::createExternal( MediaLibraryPtr ml,
                                              const std::string& fileName,
                                              int64_t duration )
{
    return createExternalMedia( ml, fileName, ImportType::External, duration );
}

std::shared_ptr<Media> Media::createStream( MediaLibraryPtr ml, const std::string& fileName )
{
    return createExternalMedia( ml, fileName, ImportType::Stream, -1 );
}

bool Media::markAsAlbumTrack( int64_t albumId, uint32_t trackNb,
                              uint32_t discNumber, int64_t artistId, Genre* genre )
{
    static const std::string req = "UPDATE " + Table::Name + " SET "
            "subtype = ?, album_id = ?, track_number = ?, disc_number = ?, "
            "artist_id = ?, genre_id = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, SubType::AlbumTrack,
            sqlite::ForeignKey{ albumId }, trackNb, discNumber,
            sqlite::ForeignKey{ artistId },
            sqlite::ForeignKey{ genre != nullptr ? genre->id() : 0 },
            m_id ) == false )
        return false;
    m_subType = SubType::AlbumTrack;
    m_albumId = albumId;
    m_trackNumber = trackNb;
    m_discNumber = discNumber;
    m_artistId = artistId;
    m_genreId = genre != nullptr ? genre->id() : 0;
    return true;
}

int64_t Media::duration() const
{
    return m_duration;
}

int64_t Media::lastTime() const
{
    return m_lastTime;
}

float Media::lastPosition() const
{
    return m_lastPosition;
}

bool Media::setDuration( int64_t duration )
{
    if ( m_duration == duration )
        return true;
    const std::string req = "UPDATE " + Table::Name + " SET "
        "duration = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req,
                                       duration, m_id ) == false )
        return false;
    m_duration = duration;
    return true;
}

ShowEpisodePtr Media::showEpisode() const
{
    if ( m_subType != SubType::ShowEpisode )
        return nullptr;

    if ( m_showEpisode == nullptr )
        m_showEpisode = ShowEpisode::fromMedia( m_ml, m_id );
    return m_showEpisode;
}

bool Media::setShowEpisode( ShowEpisodePtr episode )
{
    static const std::string req = "UPDATE " + Table::Name + " SET "
        "subtype = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req,
                                       SubType::ShowEpisode, m_id ) == false )
        return false;
    m_showEpisode = std::move( episode );
    m_subType = SubType::ShowEpisode;
    return true;
}

Query<ILabel> Media::labels() const
{
    static const std::string req = "FROM " + Label::Table::Name + " l "
            "INNER JOIN " + Label::FileRelationTable::Name + " lfr ON lfr.label_id = l.id_label "
            "WHERE lfr.media_id = ?";
    return make_query<Label, ILabel>( m_ml, "l.*", req, "", m_id );
}

uint32_t Media::playCount() const
{
    return m_playCount;
}


Media::PositionTypes Media::computePositionType( float position ) const
{
    assert( m_duration > 0 );

    float margin;
    if ( m_duration < std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::hours{ 1 } ).count() )
        margin = 0.05;
    else if ( m_duration < std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::hours{ 2 } ).count() )
        margin = 0.04;
    else if ( m_duration < std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::hours{ 3 } ).count() )
        margin = 0.03;
    else if ( m_duration < std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::hours{ 4 } ).count() )
        margin = 0.02;
    else
        margin = 0.01;

    if ( position < margin )
        return PositionTypes::Begin;
    if ( position > ( 1.f - margin ) )
        return PositionTypes::End;
    return PositionTypes::Any;
}

IMedia::ProgressResult
Media::setLastPositionAndTime( PositionTypes positionType, float lastPos,
                               int64_t lastTime )
{
    auto lastPlayedDate = time( nullptr );
    float curatedPosition;
    int64_t curatedTime;
    bool incrementPlaycount = false;
    std::string req;

    switch ( positionType )
    {
    case PositionTypes::Begin:
        /*
         * This is not far enough in the playback to update the progress, except
         * in case we had a progress saved before, then we want to reset it to
         * avoid offering to resume the playback at an old position.
         * In any case we want to bump the last played date.
         */
        req = "UPDATE " + Table::Name + " SET last_position = ?, last_time = ?,"
              "last_played_date = ? WHERE id_media = ?";
        curatedPosition = -1.f;
        curatedTime = -1;
        break;
    case PositionTypes::End:
        /*
         * We consider this to have reached the end of the media, so we bump
         * the play count, reset the progress, and bump the last played date
         */
        req = "UPDATE " + Table::Name + " SET last_position = ?, last_time = ?,"
                " play_count = play_count + 1,"
                " last_played_date = ? WHERE id_media = ?";
        curatedPosition = -1.f;
        curatedTime = -1;
        incrementPlaycount = true;
        break;
    default:
        /*
         * Just bump the progress, the playback isn't completed but advanced
         * enough for us to care about the current position or we don't know the
         * duration and can't infere anything else
         */
        req = "UPDATE " + Table::Name + " SET last_position = ?, last_time = ?,"
                "last_played_date = ? WHERE id_media = ?";
        curatedPosition = lastPos;
        curatedTime = lastTime;
    }
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, curatedPosition,
                                       curatedTime, lastPlayedDate, m_id ) == false )
        return ProgressResult::Error;
    if ( incrementPlaycount == true )
        m_playCount++;
    m_lastPlayedDate = lastPlayedDate;
    m_lastPosition = curatedPosition;
    m_lastTime = curatedTime;
    m_ml->getCb()->onHistoryChanged( isStream() ? HistoryType::Media :
                                                  HistoryType::Network );
    switch ( positionType )
    {
    case PositionTypes::Begin:
        return ProgressResult::Begin;
    case PositionTypes::End:
        return ProgressResult::End;
    default:
        return ProgressResult::AsIs;
    }
}

IMedia::ProgressResult Media::setLastPosition( float lastPosition )
{
    int64_t lastTime;
    PositionTypes positionType;
    if ( m_duration > 0 )
    {
        lastTime = lastPosition * static_cast<float>( m_duration );
        positionType = computePositionType( lastPosition );
    }
    else
    {
        lastTime = -1;
        positionType = PositionTypes::Any;
    }
    return setLastPositionAndTime( positionType, lastPosition, lastTime );
}

IMedia::ProgressResult Media::setLastTime( int64_t lastTime )
{
    float position;
    PositionTypes positionType;
    if ( m_duration > 0 )
    {
        position = static_cast<float>( lastTime ) /
                   static_cast<float>( m_duration );
        positionType = computePositionType( position );
    }
    else
    {
        position = -1.f;
        positionType = PositionTypes::Any;
    }
    return setLastPositionAndTime( positionType, position, lastTime );
}

bool Media::setPlayCount( uint32_t playCount )
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "play_count = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, playCount, m_id ) == false )
        return false;
    m_playCount = playCount;
    return true;
}

time_t Media::lastPlayedDate() const
{
    return m_lastPlayedDate;
}

bool Media::removeFromHistory()
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "last_position = ?, last_time = ?, play_count = ?, "
            "last_played_date = ? WHERE id_media = ?";
    auto dbConn = m_ml->getConn();

    if ( sqlite::Tools::executeUpdate( dbConn, req, -1.f, -1, 0, nullptr, m_id ) == false )
        return false;

    m_lastPosition = -1.f;
    m_lastTime = -1;
    m_lastPlayedDate = 0;
    m_playCount = 0;
    auto historyType = ( m_type == Type::Video || m_type == Type::Audio ) ?
                       HistoryType::Media : HistoryType::Network;
    m_ml->getCb()->onHistoryChanged( historyType );
    return true;
}

bool Media::isFavorite() const
{
    return m_isFavorite;
}

bool Media::setFavorite( bool favorite )
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET is_favorite = ? WHERE id_media = ?";
    if ( m_isFavorite == favorite )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, favorite, m_id ) == false )
        return false;
    m_isFavorite = favorite;
    return true;
}

const std::vector<FilePtr>& Media::files() const
{
    if ( m_files.empty() == true )
    {
        static const std::string req = "SELECT * FROM " + File::Table::Name
                + " WHERE media_id = ?";
        m_files = File::fetchAll<IFile>( m_ml, req, m_id );
    }
    return m_files;
}

const std::string& Media::fileName() const
{
    return m_filename;
}

MoviePtr Media::movie() const
{
    if ( m_subType != SubType::Movie )
        return nullptr;

    if ( m_movie == nullptr )
        m_movie = Movie::fromMedia( m_ml, m_id );
    return m_movie;
}

bool Media::setMovie( MoviePtr movie )
{
    if ( m_subType == SubType::Movie )
        return true;
    static const std::string req = "UPDATE " + Table::Name + " SET "
        "subtype = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req,
                                       SubType::Movie, m_id ) == false )
        return false;
    m_movie = std::move( movie );
    m_subType = SubType::Movie;
    return true;
}

bool Media::addVideoTrack( std::string codec, unsigned int width, unsigned int height,
                           uint32_t fpsNum, uint32_t fpsDen, uint32_t bitrate,
                           uint32_t sarNum, uint32_t sarDen, std::string language,
                           std::string description )
{
    return VideoTrack::create( m_ml, std::move( codec ), width, height, fpsNum,
                               fpsDen, bitrate, sarNum, sarDen, m_id,
                               std::move( language ), std::move( description ) ) != nullptr;
}

Query<IVideoTrack> Media::videoTracks() const
{
    static const std::string req = "FROM " + VideoTrack::Table::Name +
            " WHERE media_id = ?";
    return make_query<VideoTrack, IVideoTrack>( m_ml, "*", req, "", m_id );
}

bool Media::addAudioTrack( std::string codec, unsigned int bitrate,
                           unsigned int sampleRate, unsigned int nbChannels,
                           std::string language, std::string desc,
                           int64_t attachedFileId )
{
    return AudioTrack::create( m_ml, std::move( codec ), bitrate, sampleRate, nbChannels,
                               std::move( language ), std::move( desc ), m_id,
                               attachedFileId ) != nullptr;
}

bool Media::addSubtitleTrack( std::string codec, std::string language,
                              std::string description, std::string encoding,
                              int64_t attachedFileId )
{
    return SubtitleTrack::create( m_ml, std::move( codec ), std::move( language ),
                                  std::move( description ), std::move( encoding ),
                                  m_id, attachedFileId ) != nullptr;
}

Query<IAudioTrack> Media::audioTracks() const
{
    return AudioTrack::fromMedia( m_ml, m_id, false );
}

Query<IAudioTrack> Media::integratedAudioTracks() const
{
    return AudioTrack::fromMedia( m_ml, m_id, true );
}

Query<ISubtitleTrack> Media::subtitleTracks() const
{
    return SubtitleTrack::fromMedia( m_ml, m_id, false );
}

Query<ISubtitleTrack> Media::integratedSubtitleTracks()
{
    return SubtitleTrack::fromMedia( m_ml, m_id, true );
}

Query<IChapter> Media::chapters( const QueryParameters* params ) const
{
    return Chapter::fromMedia( m_ml, m_id, params );
}

bool Media::addChapter(int64_t offset, int64_t duration, std::string name)
{
    return Chapter::create( m_ml, offset, duration, std::move( name ),
                            m_id ) != nullptr;
}

std::shared_ptr<Thumbnail> Media::thumbnail( ThumbnailSizeType sizeType ) const
{
    auto idx = Thumbnail::SizeToInt(sizeType);
    if ( m_thumbnails[idx] == nullptr )
        m_thumbnails[idx] = Thumbnail::fetch( m_ml, Thumbnail::EntityType::Media,
                                              m_id, sizeType );
    return m_thumbnails[idx];
}

const std::string& Media::thumbnailMrl( ThumbnailSizeType sizeType ) const
{
    auto t = thumbnail( sizeType );
    if ( t == nullptr || t->status() != ThumbnailStatus::Available )
        return Thumbnail::EmptyMrl;
    assert( t == m_thumbnails[Thumbnail::SizeToInt( sizeType )] );
    return t->mrl();
}

ThumbnailStatus Media::thumbnailStatus( ThumbnailSizeType sizeType ) const
{
    auto t = thumbnail( sizeType );
    if ( t == nullptr )
        return ThumbnailStatus::Missing;
    return t->status();
}

unsigned int Media::insertionDate() const
{
    return static_cast<unsigned int>( m_insertionDate );
}

unsigned int Media::releaseDate() const
{
    return m_releaseDate;
}

uint32_t Media::nbPlaylists() const
{
    return m_nbPlaylists;
}

const IMetadata& Media::metadata( IMedia::MetadataType type ) const
{
    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    return m_metadata.get( static_cast<MDType>( type ) );
}

std::unordered_map<IMedia::MetadataType, std::string> Media::metadata() const
{
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    const auto& records = m_metadata.all();
    std::unordered_map<IMedia::MetadataType, std::string> res;
    for ( const auto& r : records )
    {
        if ( r.isSet() == false )
            continue;
        res.emplace( static_cast<IMedia::MetadataType>( r.type() ),
                     r.asStr() );
    }
    return res;
}

bool Media::setMetadata( IMedia::MetadataType type, const std::string& value )
{
    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    auto res = m_metadata.set( static_cast<MDType>( type ), value );
    if ( res == false )
        return false;
    return true;
}

bool Media::setMetadata( IMedia::MetadataType type, int64_t value )
{
    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    return m_metadata.set( static_cast<MDType>( type ), value );
}

bool Media::unsetMetadata(IMedia::MetadataType type)
{
    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    return m_metadata.unset( static_cast<MDType>( type ) );
}

bool Media::setMetadata( const std::unordered_map<IMedia::MetadataType, std::string>& metadata )
{
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    auto t = m_ml->getConn()->newTransaction();
    for ( const auto& m : metadata )
    {
        if ( m_metadata.set( static_cast<MDType>( m.first ), m.second ) == false )
        {
            m_metadata.clear();
            return false;
        }
    }
    t->commit();
    return true;
}

bool Media::requestThumbnail( ThumbnailSizeType sizeType, uint32_t desiredWidth,
                              uint32_t desiredHeight, float position )
{
    auto thumbnailer = m_ml->thumbnailer();
    if ( thumbnailer == nullptr )
        return false;
    thumbnailer->requestThumbnail( shared_from_this(), sizeType,
                                   desiredWidth, desiredHeight, position );
    return true;
}

bool Media::isDiscoveredMedia() const
{
    return m_importType == ImportType::Internal;
}

bool Media::isExternalMedia() const
{
    return m_importType != ImportType::Internal;
}

bool Media::isStream() const
{
    return m_importType == ImportType::Stream;
}

/* Static helper to allow MediaGroup class to assign a group to a media without
 * fetching an instance first.
 */
bool Media::setMediaGroup( MediaLibraryPtr ml, int64_t mediaId, int64_t groupId )
{
    assert( groupId != 0 );
    const std::string req = "UPDATE " + Table::Name + " SET group_id = ?"
            " WHERE id_media = ?";
    return sqlite::Tools::executeUpdate( ml->getConn(), req, groupId, mediaId );
}

bool Media::addToGroup( IMediaGroup& group )
{
    if ( group.id() == m_groupId )
        return true;
    return group.add( *this );
}

bool Media::addToGroup( int64_t groupId )
{
    if ( m_groupId == groupId )
        return true;
    auto group = MediaGroup::fetch( m_ml, groupId );
    if ( group == nullptr )
        return false;
    return addToGroup( *group );
}

void Media::setMediaGroupId( int64_t groupId )
{
    m_groupId = groupId;
}

bool Media::removeFromGroup()
{
    auto g = group();
    if ( g == nullptr )
        return false;
    return g->remove( *this );
}

MediaGroupPtr Media::group() const
{
    if ( m_groupId == 0 )
        return nullptr;
    return MediaGroup::fetch( m_ml, m_groupId );
}

int64_t Media::groupId() const
{
    return m_groupId;
}

std::vector<std::shared_ptr<Media>> Media::fetchMatchingUngrouped()
{
    const std::string req = "SELECT m.* FROM " + Table::Name + " m "
            " INNER JOIN " + MediaGroup::Table::Name + " mg ON "
                " m.group_id = mg.id_group "
            " WHERE mg.forced_singleton != 0 AND"
            " SUBSTR(title, 1, ?) = ? COLLATE NOCASE";
    auto prefix = MediaGroup::prefix( m_title );
    return fetchAll<Media>( m_ml, req, prefix.length(), prefix );
}

bool Media::regroup()
{
    assert( m_groupId != 0 );
    auto singleton = std::static_pointer_cast<MediaGroup>( group() );
    /* Refuse to regroup a media that isn't in a locked group */
    if ( singleton == nullptr || singleton->isForcedSingleton() == false )
        return false;
    auto t = m_ml->getConn()->newTransaction();
    auto group = MediaGroup::create( m_ml, m_title, false, false );
    if ( group == nullptr )
        return false;
    if ( group->add( *this ) == false )
        return false;
    auto candidates = fetchMatchingUngrouped();
    std::string groupName = m_title;
    for ( const auto& c : candidates )
    {
        groupName = MediaGroup::commonPattern( groupName, c->title() );
        group->add( c->id() );
    }
    group->rename( std::move( groupName ), false );
    t->commit();
    return true;
}

bool Media::setReleaseDate( unsigned int date )
{
    if ( m_releaseDate == date )
        return true;
    static const std::string req = "UPDATE " + Table::Name + " SET "
        "release_date = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, date, m_id ) == false )
        return false;
    m_releaseDate = date;
    return true;
}

int64_t Media::deviceId() const
{
    return m_deviceId;
}

int64_t Media::folderId() const
{
    return m_folderId;
}

bool Media::markAsInternal( Type type, int64_t duration, int64_t deviceId, int64_t folderId )
{
    static const std::string req = "UPDATE " + Table::Name + " SET "
        "type = ?, duration = ?, device_id = ?, folder_id = ?, import_type = ? "
        "WHERE id_media = ?";
    assert( deviceId != 0 );
    assert( folderId != 0 );
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, type, duration,
                                       deviceId, folderId, ImportType::Internal,
                                       m_id ) == false )
        return false;
    m_type = type;
    m_duration = duration;
    m_deviceId = deviceId;
    m_folderId = folderId;
    m_importType = ImportType::Internal;
    return true;
}

bool Media::convertToExternal()
{
    auto t = m_ml->getConn()->newTransaction();
    for ( const auto& fptr : files() )
    {
        auto f = static_cast<File*>( fptr.get() );
        if ( f->convertToExternal() == false )
            return false;
    }
    switch ( m_subType )
    {
    case IMedia::SubType::Unknown:
        break;
    case IMedia::SubType::ShowEpisode:
        if ( ShowEpisode::deleteByMediaId( m_ml, m_id ) == false )
            return false;
        break;
    case IMedia::SubType::AlbumTrack:
        if ( markAsAlbumTrack( 0, 0, 0, 0, nullptr ) == false )
            return false;
        if ( Artist::dropMediaArtistRelation( m_ml, m_id ) == false )
            return false;
        break;
    case IMedia::SubType::Movie:
        if ( Movie::deleteByMediaId( m_ml, m_id ) == false )
            return false;
        break;
    }

    const std::string req = "UPDATE " + Table::Name + " SET subtype = ?, "
        "device_id = NULL, folder_id = NULL, import_type = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, IMedia::SubType::Unknown,
                                       Media::ImportType::External, m_id ) == false )
        return false;
    t->commit();
    auto notifier = m_ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyMediaConvertedToExternal( m_id );
    m_subType = IMedia::SubType::Unknown;
    m_importType = ImportType::External;
    m_deviceId = 0;
    m_folderId = 0;
    return true;
}

bool Media::shouldUpdateThumbnail( const Thumbnail& currentThumbnail )
{
    /*
     * We're about to update a media thumbnail.
     * A media thumbnail might be shared by an album or an artist, which we most
     * likely don't want to update when updating a specific media.
     * In case the thumbnail is shared, we should just insert a new one.
     * Otherwise, it's fine to just update the existing one.
     */
    return currentThumbnail.isShared() == false;
}

bool Media::setThumbnail( std::shared_ptr<Thumbnail> newThumbnail )
{
    assert( newThumbnail != nullptr );
    auto thumbnailIdx = Thumbnail::SizeToInt( newThumbnail->sizeType() );
    auto currentThumbnail = thumbnail( newThumbnail->sizeType() );
    currentThumbnail = Thumbnail::updateOrReplace( m_ml, std::move( currentThumbnail ),
                                                   std::move( newThumbnail ),
                                                   Media::shouldUpdateThumbnail,
                                                   m_id, Thumbnail::EntityType::Media );
    if ( currentThumbnail == nullptr )
        return false;
    m_thumbnails[thumbnailIdx] = std::move( currentThumbnail );
    auto notifier = m_ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyMediaModification( m_id );
    return true;
}

bool Media::removeThumbnail( ThumbnailSizeType sizeType )
{
    auto t = thumbnail( sizeType );
    if ( t == nullptr )
        return true;
    auto res = t->unlinkThumbnail( m_id, Thumbnail::EntityType::Media );
    if ( res == false )
        return false;
    m_thumbnails[Thumbnail::SizeToInt( sizeType )] = nullptr;
    return true;
}

bool Media::setThumbnail( const std::string& thumbnailMrl, ThumbnailSizeType sizeType )
{
    return setThumbnail( std::make_shared<Thumbnail>( m_ml, thumbnailMrl,
                                                      Thumbnail::Origin::UserProvided,
                                                      sizeType, false ) );
}

std::shared_ptr<File> Media::addFile( const fs::IFile& fileFs, int64_t parentFolderId,
                                      bool isFolderFsRemovable, IFile::Type type )
{
    return File::createFromMedia( m_ml, m_id, type, fileFs, parentFolderId, isFolderFsRemovable);
}

FilePtr Media::addFile( const std::string& mrl, IFile::Type fileType )
{
    auto fsFactory = m_ml->fsFactoryForMrl( mrl );
    if ( fsFactory == nullptr )
    {
        LOG_INFO( "Failed to find an fs factory for mrl: ", mrl );
        return nullptr;
    }
    auto deviceFs = fsFactory->createDeviceFromMrl( mrl );
    if ( deviceFs == nullptr )
    {
        LOG_INFO( "Failed to fetch device for mrl: ", mrl );
        return nullptr;
    }
    std::shared_ptr<fs::IFile> fileFs;
    try
    {
        fileFs = fsFactory->createFile( mrl );
    }
    catch ( const fs::errors::System& ex )
    {
        LOG_INFO( "Failed to create a file instance for mrl: ", mrl, ": ", ex.what() );
        return nullptr;
    }
    auto folderMrl = utils::file::directory( mrl );
    auto folder = Folder::fromMrl( m_ml, folderMrl );
    return addFile( *fileFs, folder != nullptr ? folder->id() : 0,
                    deviceFs->isRemovable(), fileType );
}

FilePtr Media::addExternalMrl( const std::string& mrl, IFile::Type type )
{
    FilePtr res;
    try
    {
        res = File::createFromExternalMedia( m_ml, m_id, type, mrl );
    }
    catch ( const sqlite::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to add media external MRL: ", ex.what() );
        return nullptr;
    }
    if ( res == nullptr )
        return nullptr;
    if ( m_files.empty() == false )
        m_files.push_back( res );
    return res;
}

void Media::removeFile( File& file )
{
    file.destroy();
    auto it = std::remove_if( begin( m_files ), end( m_files ), [&file]( const FilePtr& f ) {
        return f->id() == file.id();
    });
    if ( it != end( m_files ) )
        m_files.erase( it );
}

Query<IBookmark> Media::bookmarks( const QueryParameters* params ) const
{
    return Bookmark::fromMedia( m_ml, m_id, params );
}

BookmarkPtr Media::bookmark( int64_t time ) const
{
    return Bookmark::fromMedia( m_ml, m_id, time );
}

BookmarkPtr Media::addBookmark( int64_t time )
{
    return Bookmark::create( m_ml, time, m_id );
}

bool Media::removeBookmark( int64_t time )
{
    return Bookmark::remove( m_ml, time, m_id );
}

bool Media::removeAllBookmarks()
{
    return Bookmark::removeAll( m_ml, m_id );
}

bool Media::isPresent() const
{
    return m_isPresent;
}

ArtistPtr Media::artist() const
{
    if ( m_artistId == 0 )
        return nullptr;
    return Artist::fetch( m_ml, m_artistId );
}

int64_t Media::artistId() const
{
    return m_artistId;
}

GenrePtr Media::genre() const
{
    if ( m_genreId == 0 )
        return nullptr;
    return Genre::fetch( m_ml, m_genreId );
}

int64_t Media::genreId() const
{
    return m_genreId;
}

unsigned int Media::trackNumber() const
{
    return m_trackNumber;
}

AlbumPtr Media::album() const
{
    if ( m_albumId == 0 )
        return nullptr;
    return Album::fetch( m_ml, m_albumId );
}

int64_t Media::albumId() const
{
    return m_albumId;
}

uint32_t Media::discNumber() const
{
    return m_discNumber;
}

const std::string& Media::lyrics() const
{
    return m_lyrics;
}

bool Media::setLyrics( std::string lyrics )
{
    if ( lyrics == m_lyrics )
        return true;
    const std::string req = "UPDATE " + Table::Name + " SET lyrics = ? "
        " WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, lyrics, m_id ) == false )
        return false;
    m_lyrics = std::move( lyrics );
    return true;
}

std::string Media::addRequestJoin( const QueryParameters* params, bool forceFile )
{
    bool artist = false;
    bool album = false;
    bool file = forceFile;
    auto sort = params != nullptr ? params->sort : SortingCriteria::Alpha;

    switch( sort )
    {
        case SortingCriteria::Default:
        case SortingCriteria::Alpha:
        case SortingCriteria::PlayCount:
        case SortingCriteria::Duration:
        case SortingCriteria::InsertionDate:
        case SortingCriteria::ReleaseDate:
        case SortingCriteria::Filename:
            /* All those are stored in the media itself */
            break;
        case SortingCriteria::LastModificationDate:
        case SortingCriteria::FileSize:
            file = true;
            break;
        case SortingCriteria::Artist:
            artist = true;
            break;
        case SortingCriteria::Album:
            /* We need the album track to get the album id & the album for its title */
            album = true;
            break;
        case SortingCriteria::TrackId:
            album = true;
            break;
        default:
            // Unrelated to media requests
            break;
    }
    std::string req;
    // Use "LEFT JOIN to allow for ordering different media type
    // For instance ordering by albums on all media would not fetch the video if
    // we were using INNER JOIN
    if ( album == true )
    {
        req += " LEFT JOIN " + Album::Table::Name + " alb ON m.album_id = alb.id_album ";
    }
    if ( artist == true )
    {
        req += " LEFT JOIN " + Artist::Table::Name + " art ON m.artist_id = art.id_artist ";
    }
    if ( file == true )
        req += " LEFT JOIN " + File::Table::Name + " f ON m.id_media = f.media_id ";

    return req;
}

std::string Media::sortRequest( const QueryParameters* params )
{
    std::string req = " ORDER BY ";

    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    auto descAdded = false;
    switch ( sort )
    {
    case SortingCriteria::Duration:
        req += "m.duration";
        break;
    case SortingCriteria::InsertionDate:
        req += "m.insertion_date";
        break;
    case SortingCriteria::ReleaseDate:
        req += "m.release_date";
        break;
    case SortingCriteria::PlayCount:
        req += "m.play_count";
        desc = !desc; // Make decreasing order default for play count sorting
        break;
    case SortingCriteria::Filename:
        req += "m.filename";
        break;
    case SortingCriteria::LastModificationDate:
        req += "f.last_modification_date";
        break;
    case SortingCriteria::FileSize:
        req += "f.size";
        break;
    case SortingCriteria::Album:
        if ( desc == true )
            req += "alb.title DESC, m.track_number";
        else
            req += "alb.title, m.track_number";
        descAdded = true;
        break;
    case SortingCriteria::Artist:
        req += "art.name";
        break;
    case SortingCriteria::LastPlaybackDate:
        req += "m.last_played_date";
        break;
    case SortingCriteria::TrackId:
        if ( desc == true )
            req += "alb.title, m.track_number DESC, m.disc_number";
        else
            req += "alb.title, m.track_number, m.disc_number";
        descAdded = true;
        break;
    default:
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default (Alpha)" );
        /* fall-through */
    case SortingCriteria::Default:
    case SortingCriteria::Alpha:
        req += "m.title";
        break;
    }
    if ( desc == true && !descAdded )
        req += " DESC";
    return req;
}

Query<IMedia> Media::listAll( MediaLibraryPtr ml, IMedia::Type type,
                              const QueryParameters* params,
                              IMedia::SubType subType )
{
    assert( type == IMedia::Type::Audio || type == IMedia::Type::Video );
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true );
    // We want to include unknown media to the video listing, so we invert the
    // filter to exclude Audio (ie. we include Unknown & Video)
    // If we only want audio media, then we just filter 'type = Audio'
    if ( type == IMedia::Type::Video )
        req += " WHERE m.type != ?";
    else
        req += " WHERE m.type = ?";
    req +=  " AND (f.type = ? OR f.type = ?)"
            " AND f.is_external = 0";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";

    if ( subType != IMedia::SubType::Unknown )
    {
        req += " AND m.subtype = ?";
        return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                        sortRequest( params ), IMedia::Type::Audio,
                                        IFile::Type::Main, IFile::Type::Disc, subType );
    }

    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), IMedia::Type::Audio,
                                      IFile::Type::Main, IFile::Type::Disc );
}

Query<IMedia> Media::listInProgress( MediaLibraryPtr ml, IMedia::Type type,
                                     const QueryParameters *params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, false );
    req += " WHERE (m.last_position >= 0 OR m.last_time > 0)";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    if ( type == IMedia::Type::Unknown )
    {
        return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                          sortRequest( params ) );
    }
    req += " AND m.type = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), type );
}

int64_t Media::id() const
{
    return m_id;
}

IMedia::Type Media::type() const
{
    return m_type;
}

bool Media::setType( IMedia::Type type )
{
    if ( type == m_type )
        return true;
    auto oldType = m_type;
    if ( setTypeInternal( type ) == false )
        return false;
    if ( oldType == IMedia::Type::Unknown )
    {
        auto task = parser::Task::createMediaRefreshTask( m_ml, *this );
        if ( task != nullptr )
        {
            auto parser = m_ml->getParser();
            if ( parser != nullptr )
                parser->parse( std::move( task ) );
        }
    }
    return true;
}

bool Media::setTypeInternal( IMedia::Type type )
{
    if ( type == m_type )
        return true;
    const std::string req = "UPDATE " + Table::Name + " SET type = ? "
            "WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, type, m_id ) == false )
        return false;
    m_type = type;
    return true;
}

IMedia::SubType Media::subType() const
{
    return m_subType;
}

bool Media::setSubTypeUnknown()
{
    static const std::string req = "UPDATE " + Table::Name + " SET "
            "subtype = ?, album_id = NULL, track_number = 0, disc_number = 0, "
            "artist_id = NULL, genre_id = NULL WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, SubType::Unknown,
            m_id ) == false )
        return false;
    m_subType = SubType::Unknown;
    m_albumId = 0;
    m_trackNumber = 0;
    m_discNumber = 0;
    m_artistId = 0;
    m_genreId = 0;
    return true;
}

const std::string& Media::title() const
{
    return m_title;
}

bool Media::setTitle( const std::string& title )
{
    return setTitle( title, true );
}

bool Media::setTitle( const std::string& title, bool forced )
{
    if ( ( m_forcedTitle == true && forced == false ) ||
         m_title == title )
        return true;
    static const std::string req = "UPDATE " + Media::Table::Name +
            " SET title = ?, forced_title = ? WHERE id_media = ?";
    try
    {
        if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, title, forced,
                                           m_id ) == false )
            return false;
    }
    catch ( const sqlite::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to set media title: ", ex.what() );
        return false;
    }

    m_title = title;
    m_forcedTitle = forced;

    return true;
}

bool Media::setForcedTitle( MediaLibraryPtr ml, int64_t mediaId )
{
    const std::string req = "UPDATE " + Media::Table::Name +
            " SET forced_title = 1 WHERE id_media = ?";
    return sqlite::Tools::executeUpdate( ml->getConn(), req, mediaId );
}

bool Media::isForcedTitle() const
{
    return m_forcedTitle;
}

bool Media::setFileName( std::string fileName )
{
    if ( fileName == m_filename )
        return true;
    static const std::string req = "UPDATE " + Media::Table::Name + " SET filename = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, fileName, m_id ) == false )
        return false;
    m_filename = std::move( fileName );
    return true;
}

void Media::createTable( sqlite::Connection* connection )
{
    std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
        schema( FtsTable::Name, Settings::DbModelVersion ),
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( connection, req );
}

void Media::createTriggers( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::IsPresent,
                                            Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::CascadeFileDeletion,
                                            Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::InsertFts,
                                            Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::CascadeFileUpdate,
                                            Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::DeleteFts,
                                            Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::UpdateFts,
                                            Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::IncrementNbPlaylist,
                                            Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::DecrementNbPlaylist,
                                            Settings::DbModelVersion ) );
}

void Media::createIndexes( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::LastPlayedDate,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::Presence,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::Types,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::InsertionDate,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::Folder,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::MediaGroup,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::Progress,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::AlbumTrack,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::Duration,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::ReleaseDate,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::PlayCount,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::Title,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::FileName,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::GenreId,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::ArtistId,
                                          Settings::DbModelVersion ) );
}

std::string Media::schema( const std::string& tableName, uint32_t dbModel )
{
    if ( tableName == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE " + FtsTable::Name +
               " USING FTS3(title,labels)";
    }
    assert( tableName == Table::Name );
    if ( dbModel <= 16 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "subtype INTEGER NOT NULL DEFAULT " +
                utils::enum_to_string( SubType::Unknown ) + ","
            "duration INTEGER DEFAULT -1,"
            "play_count UNSIGNED INTEGER,"
            "last_played_date UNSIGNED INTEGER,"
            "real_last_played_date UNSIGNED INTEGER,"
            "insertion_date UNSIGNED INTEGER,"
            "release_date UNSIGNED INTEGER,"
            "thumbnail_id INTEGER,"
            "title TEXT COLLATE NOCASE,"
            "filename TEXT COLLATE NOCASE,"
            "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "device_id INTEGER,"
            "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "FOREIGN KEY(thumbnail_id) REFERENCES " + Thumbnail::Table::Name
                + "(id_thumbnail),"
            "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
                + "(id_folder)"
        ")";
    }
    if ( dbModel < 23 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "subtype INTEGER NOT NULL DEFAULT " +
                utils::enum_to_string( SubType::Unknown ) + ","
            "duration INTEGER DEFAULT -1,"
            "play_count UNSIGNED INTEGER,"
            "last_played_date UNSIGNED INTEGER,"
            "real_last_played_date UNSIGNED INTEGER,"
            "insertion_date UNSIGNED INTEGER,"
            "release_date UNSIGNED INTEGER,"
            "title TEXT COLLATE NOCASE,"
            "filename TEXT COLLATE NOCASE,"
            "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "device_id INTEGER,"
            "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
                + "(id_folder)"
        ")";
    }
    if ( dbModel == 23 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "subtype INTEGER NOT NULL DEFAULT " +
                utils::enum_to_string( SubType::Unknown ) + ","
            "duration INTEGER DEFAULT -1,"
            "play_count UNSIGNED INTEGER,"
            "last_played_date UNSIGNED INTEGER,"
            "real_last_played_date UNSIGNED INTEGER,"
            "insertion_date UNSIGNED INTEGER,"
            "release_date UNSIGNED INTEGER,"
            "title TEXT COLLATE NOCASE,"
            "filename TEXT COLLATE NOCASE,"
            "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "device_id INTEGER,"
            "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "import_type UNSIGNED INTEGER NOT NULL,"
            "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
                + "(id_folder)"
        ")";
    }
    if ( dbModel == 24 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "subtype INTEGER NOT NULL DEFAULT " +
                utils::enum_to_string( SubType::Unknown ) + ","
            "duration INTEGER DEFAULT -1,"
            "play_count UNSIGNED INTEGER,"
            "last_played_date UNSIGNED INTEGER,"
            "real_last_played_date UNSIGNED INTEGER,"
            "insertion_date UNSIGNED INTEGER,"
            "release_date UNSIGNED INTEGER,"
            "title TEXT COLLATE NOCASE,"
            "filename TEXT COLLATE NOCASE,"
            "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "device_id INTEGER,"
            "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "import_type UNSIGNED INTEGER NOT NULL,"
            "group_id UNSIGNED INTEGER,"
            "has_been_grouped BOOLEAN NOT NULL DEFAULT 0,"
            "forced_title BOOLEAN NOT NULL DEFAULT 0,"
            "FOREIGN KEY(group_id) REFERENCES " + MediaGroup::Table::Name +
                "(id_group) ON DELETE RESTRICT,"
            "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
                + "(id_folder)"
        ")";
    }
    if ( dbModel < 27 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "subtype INTEGER NOT NULL DEFAULT " +
                utils::enum_to_string( SubType::Unknown ) + ","
            "duration INTEGER DEFAULT -1,"
            "play_count UNSIGNED INTEGER,"
            "last_played_date UNSIGNED INTEGER,"
            "real_last_played_date UNSIGNED INTEGER,"
            "insertion_date UNSIGNED INTEGER,"
            "release_date UNSIGNED INTEGER,"
            "title TEXT COLLATE NOCASE,"
            "filename TEXT COLLATE NOCASE,"
            "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "device_id INTEGER,"
            "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "import_type UNSIGNED INTEGER NOT NULL,"
            "group_id UNSIGNED INTEGER,"
            "forced_title BOOLEAN NOT NULL DEFAULT 0,"
            "FOREIGN KEY(group_id) REFERENCES " + MediaGroup::Table::Name +
                "(id_group) ON DELETE RESTRICT,"
            "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
                + "(id_folder)"
        ")";
    }
    if ( dbModel < 30 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "subtype INTEGER NOT NULL DEFAULT " +
                utils::enum_to_string( SubType::Unknown ) + ","
            "duration INTEGER DEFAULT -1,"
            "progress REAL DEFAULT -1,"
            "play_count UNSIGNED INTEGER,"
            "last_played_date UNSIGNED INTEGER,"
            "real_last_played_date UNSIGNED INTEGER,"
            "insertion_date UNSIGNED INTEGER,"
            "release_date UNSIGNED INTEGER,"
            "title TEXT COLLATE NOCASE,"
            "filename TEXT COLLATE NOCASE,"
            "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "device_id INTEGER,"
            "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "import_type UNSIGNED INTEGER NOT NULL,"
            "group_id UNSIGNED INTEGER,"
            "forced_title BOOLEAN NOT NULL DEFAULT 0,"
            "FOREIGN KEY(group_id) REFERENCES " + MediaGroup::Table::Name +
                "(id_group) ON DELETE RESTRICT,"
            "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
                + "(id_folder)"
        ")";
    }
    if ( dbModel < 33 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "subtype INTEGER NOT NULL DEFAULT " +
                utils::enum_to_string( SubType::Unknown ) + ","
            "duration INTEGER DEFAULT -1,"
            "last_position REAL DEFAULT -1,"
            "last_time INTEGER DEFAULT -1,"
            "play_count UNSIGNED INTEGER,"
            "last_played_date UNSIGNED INTEGER,"
            "real_last_played_date UNSIGNED INTEGER,"
            "insertion_date UNSIGNED INTEGER,"
            "release_date UNSIGNED INTEGER,"
            "title TEXT COLLATE NOCASE,"
            "filename TEXT COLLATE NOCASE,"
            "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "device_id INTEGER,"
            "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "import_type UNSIGNED INTEGER NOT NULL,"
            "group_id UNSIGNED INTEGER,"
            "forced_title BOOLEAN NOT NULL DEFAULT 0,"
            "FOREIGN KEY(group_id) REFERENCES " + MediaGroup::Table::Name +
                "(id_group) ON DELETE RESTRICT,"
            "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
                + "(id_folder)"
        ")";
    }
    if ( dbModel == 33 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER,"
            "subtype INTEGER NOT NULL DEFAULT " +
                utils::enum_to_string( SubType::Unknown ) + ","
            "duration INTEGER DEFAULT -1,"
            "last_position REAL DEFAULT -1,"
            "last_time INTEGER DEFAULT -1,"
            "play_count UNSIGNED INTEGER NON NULL DEFAULT 0,"
            "last_played_date UNSIGNED INTEGER,"
            "insertion_date UNSIGNED INTEGER,"
            "release_date UNSIGNED INTEGER,"
            "title TEXT COLLATE NOCASE,"
            "filename TEXT COLLATE NOCASE,"
            "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "device_id INTEGER,"
            "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "import_type UNSIGNED INTEGER NOT NULL,"
            "group_id UNSIGNED INTEGER,"
            "forced_title BOOLEAN NOT NULL DEFAULT 0,"
            "FOREIGN KEY(group_id) REFERENCES " + MediaGroup::Table::Name +
                "(id_group) ON DELETE RESTRICT,"
            "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
                + "(id_folder)"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "("
        "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
        "type INTEGER,"
        "subtype INTEGER NOT NULL DEFAULT " +
            utils::enum_to_string( SubType::Unknown ) + ","
        "duration INTEGER DEFAULT -1,"
        "last_position REAL DEFAULT -1,"
        "last_time INTEGER DEFAULT -1,"
        "play_count UNSIGNED INTEGER NOT NULL DEFAULT 0,"
        "last_played_date UNSIGNED INTEGER,"
        "insertion_date UNSIGNED INTEGER,"
        "release_date UNSIGNED INTEGER,"
        "title TEXT COLLATE NOCASE,"
        "filename TEXT COLLATE NOCASE,"
        "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
        "is_present BOOLEAN NOT NULL DEFAULT 1,"
        "device_id INTEGER,"
        "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
        "folder_id UNSIGNED INTEGER,"
        "import_type UNSIGNED INTEGER NOT NULL,"
        "group_id UNSIGNED INTEGER,"
        "forced_title BOOLEAN NOT NULL DEFAULT 0,"
        "artist_id UNSIGNED INTEGER,"
        "genre_id UNSIGNED INTEGER,"
        "track_number UNSIGEND INTEGER,"
        "album_id UNSIGNED INTEGER,"
        "disc_number UNSIGNED INTEGER,"
        "lyrics TEXT,"
        "FOREIGN KEY(group_id) REFERENCES " + MediaGroup::Table::Name +
            "(id_group) ON DELETE RESTRICT,"
        "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
            + "(id_folder)"
         "FOREIGN KEY(artist_id) REFERENCES " + Artist::Table::Name + "(id_artist)"
             " ON DELETE SET NULL,"
         "FOREIGN KEY(genre_id) REFERENCES " + Genre::Table::Name + "(id_genre),"
         "FOREIGN KEY(album_id) REFERENCES Album(id_album) "
             " ON DELETE SET NULL"
    ")";
}

std::string Media::trigger( Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::IsPresent:
        {
            if ( dbModel < 23 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                       " AFTER UPDATE OF "
                       "is_present ON " + Device::Table::Name + " "
                       "BEGIN "
                       "UPDATE " + Table::Name + " "
                           "SET is_present=new.is_present "
                                "WHERE device_id=new.id_device;"
                       "END";
            }
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER UPDATE OF "
                   "is_present ON " + Device::Table::Name + " "
                   "WHEN old.is_present != new.is_present "
                   "BEGIN "
                   "UPDATE " + Table::Name + " "
                       "SET is_present=new.is_present "
                       "WHERE device_id=new.id_device;"
                   "END";
        }
        case Triggers::CascadeFileDeletion:
        {
            if ( dbModel < 23 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER DELETE ON " + File::Table::Name +
                        " BEGIN "
                        " DELETE FROM " + Table::Name + " WHERE "
                            "(SELECT COUNT(id_file) FROM " + File::Table::Name +
                                " WHERE media_id=old.media_id) = 0"
                                " AND id_media=old.media_id;"
                        " END";
            }
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER DELETE ON " + File::Table::Name +
                   " WHEN old.type = " +
                        utils::enum_to_string( IFile::Type::Main ) +
                   " OR old.type = " +
                       utils::enum_to_string( IFile::Type::Disc ) +
                   " BEGIN "
                   " DELETE FROM " + Table::Name +
                       " WHERE id_media=old.media_id;"
                   " END";
        }
        case Triggers::CascadeFileUpdate:
        {
            assert( dbModel >= 27 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER UPDATE OF media_id ON " + File::Table::Name +
                   " WHEN old.media_id != new.media_id AND old.type = " +
                        utils::enum_to_string( IFile::Type::Main ) +
                   " BEGIN"
                   " DELETE FROM " + Table::Name +
                        " WHERE id_media = old.media_id;"
                   "END";
        }
        case Triggers::IncrementNbPlaylist:
            assert( dbModel >= 14 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER INSERT ON " + Playlist::MediaRelationTable::Name +
                    " BEGIN"
                        " UPDATE " + Table::Name + " SET nb_playlists = nb_playlists + 1 "
                            " WHERE id_media = new.media_id;"
                    " END";
        case Triggers::DecrementNbPlaylist:
            assert( dbModel >= 14 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER DELETE ON " + Playlist::MediaRelationTable::Name +
                    " BEGIN"
                        " UPDATE " + Table::Name + " SET nb_playlists = nb_playlists - 1 "
                            " WHERE id_media = old.media_id;"
                    " END";
        case Triggers::InsertFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER INSERT ON " + Table::Name +
                   " BEGIN"
                       " INSERT INTO " + FtsTable::Name + "(rowid,title,labels)"
                           " VALUES(new.id_media, new.title, '');"
                   " END";
        case Triggers::DeleteFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " BEFORE DELETE ON " + Table::Name +
                   " BEGIN"
                       " DELETE FROM " + FtsTable::Name +
                           " WHERE rowid = old.id_media;"
                   " END";
        case Triggers::UpdateFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER UPDATE OF title ON " + Table::Name +
                   " BEGIN"
                       " UPDATE " + FtsTable::Name + " SET title = new.title"
                           " WHERE rowid = new.id_media;"
                   " END";

        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Media::triggerName( Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::IsPresent:
        {
            if ( dbModel < 23 )
                return "is_media_device_present";
            return "media_update_device_presence";
        }
        case Triggers::CascadeFileDeletion:
        {
            if ( dbModel < 23 )
                return "cascade_file_deletion";
            return "media_cascade_file_deletion";
        }
        case Triggers::CascadeFileUpdate:
            assert( dbModel >= 27 );
            return "media_cascade_file_update";
        case Triggers::IncrementNbPlaylist:
            assert( dbModel >= 14 );
            return "increment_media_nb_playlist";
        case Triggers::DecrementNbPlaylist:
            assert( dbModel >= 14 );
            return "decrement_media_nb_playlist";
        case Triggers::InsertFts:
            return "insert_media_fts";
        case Triggers::DeleteFts:
            return "delete_media_fts";
        case Triggers::UpdateFts:
            return "update_media_title_fts";
        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Media::index( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::LastPlayedDate:
            return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(last_played_date DESC)";
        case Indexes::Presence:
            return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(is_present)";
        case Indexes::Types:
            return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(type, subtype)";
        case Indexes::InsertionDate:
            assert( dbModel >= 14 );
            // Don't create this index before model 14, as the real_last_played_date
            // column was introduced in model version 14
            if ( dbModel < 33 )
            {
                return "CREATE INDEX " + indexName( index, dbModel ) +
                            " ON " + Table::Name + "(last_played_date, "
                                "real_last_played_date, insertion_date)";
            }
            if ( dbModel == 33 )
            {
                return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(last_played_date, "
                            "insertion_date)";
            }
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(insertion_date)";
        case Indexes::Folder:
            assert( dbModel >= 22 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(folder_id)";
        case Indexes::MediaGroup:
            assert( dbModel >= 24 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(group_id)";
        case Indexes::Progress:
            assert( dbModel >= 27 );
            if ( dbModel < 31 )
                return "CREATE INDEX " + indexName( index, dbModel ) +
                       " ON " + Table::Name + "(progress)";
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(last_position, last_time)";
        case Indexes::AlbumTrack:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(album_id, genre_id, artist_id)";
        case Indexes::Duration:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(duration)";
        case Indexes::ReleaseDate:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(release_date)";
        case Indexes::PlayCount:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(play_count)";
        case Indexes::Title:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(title)";
        case Indexes::FileName:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(filename)";
        case Indexes::GenreId:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(genre_id)";
        case Indexes::ArtistId:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name + "(artist_id)";
        default:
            assert( !"Invalid index provided" );
    }
    return "<invalid request>";
}

std::string Media::indexName( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::LastPlayedDate:
            return "index_last_played_date";
        case Indexes::Presence:
            return "index_media_presence";
        case Indexes::Types:
            return "media_types_idx";
        case Indexes::InsertionDate:
            assert( dbModel >= 14 );
            // Don't create this index before model 14, as the real_last_played_date
            // column was introduced in model version 14
            if ( dbModel < 34 )
                return "media_last_usage_dates_idx";
            return "media_insertion_date_idx";
        case Indexes::Folder:
            assert( dbModel >= 22 );
            return "media_folder_id_idx";
        case Indexes::MediaGroup:
            assert( dbModel >= 24 );
            return "media_group_id_idx";
        case Indexes::Progress:
            assert( dbModel >= 27 );
            if ( dbModel < 31 )
                return "media_progress_idx";
            return "media_last_pos_time_idx";
        case Indexes::AlbumTrack:
            assert( dbModel >= 34 );
            return "media_album_track_idx";
        case Indexes::Duration:
            assert( dbModel >= 34 );
            return "media_duration_idx";
        case Indexes::ReleaseDate:
            assert( dbModel >= 34 );
            return "media_release_date_idx";
        case Indexes::PlayCount:
            assert( dbModel >= 34 );
            return "media_play_count_idx";
        case Indexes::Title:
            assert( dbModel >= 34 );
            return "media_title_idx";
        case Indexes::FileName:
            assert( dbModel >= 34 );
            return "media_filename_idx";
        case Indexes::GenreId:
            assert( dbModel >= 34 );
            return "media_genre_id_idx";
        case Indexes::ArtistId:
            assert( dbModel >= 34 );
            return "media_artist_id_idx";
        default:
            assert( !"Invalid index provided" );
    }
    return "<invalid request>";
}

bool Media::checkDbModel( MediaLibraryPtr ml )
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );
    if ( sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) == false ||
         sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( FtsTable::Name, Settings::DbModelVersion ),
                                       FtsTable::Name ) == false )
        return false;

    auto checkTrigger = []( sqlite::Connection* dbConn, Triggers t ) {
        return sqlite::Tools::checkTriggerStatement( dbConn,
                                    trigger( t, Settings::DbModelVersion ),
                                    triggerName( t, Settings::DbModelVersion ) );
    };

    auto checkIndex = []( sqlite::Connection* dbConn, Indexes i ) {
        return sqlite::Tools::checkIndexStatement( dbConn,
                                    index( i, Settings::DbModelVersion ),
                                    indexName( i, Settings::DbModelVersion ) );
    };

    return checkTrigger( ml->getConn(), Triggers::IsPresent ) &&
            checkTrigger( ml->getConn(), Triggers::CascadeFileDeletion ) &&
            checkTrigger( ml->getConn(), Triggers::IncrementNbPlaylist ) &&
            checkTrigger( ml->getConn(), Triggers::DecrementNbPlaylist ) &&
            checkTrigger( ml->getConn(), Triggers::InsertFts ) &&
            checkTrigger( ml->getConn(), Triggers::DeleteFts ) &&
            checkTrigger( ml->getConn(), Triggers::UpdateFts ) &&
            checkIndex( ml->getConn(), Indexes::LastPlayedDate ) &&
            checkIndex( ml->getConn(), Indexes::Presence ) &&
            checkIndex( ml->getConn(), Indexes::Types ) &&
            checkIndex( ml->getConn(), Indexes::InsertionDate ) &&
            checkIndex( ml->getConn(), Indexes::Folder ) &&
            checkIndex( ml->getConn(), Indexes::MediaGroup ) &&
            checkIndex( ml->getConn(), Indexes::Progress ) &&
            checkIndex( ml->getConn(), Indexes::AlbumTrack ) &&
            checkIndex( ml->getConn(), Indexes::Duration ) &&
            checkIndex( ml->getConn(), Indexes::ReleaseDate ) &&
            checkIndex( ml->getConn(), Indexes::PlayCount ) &&
            checkIndex( ml->getConn(), Indexes::Title ) &&
            checkIndex( ml->getConn(), Indexes::FileName ) &&
            checkIndex( ml->getConn(), Indexes::GenreId ) &&
            checkIndex( ml->getConn(), Indexes::ArtistId );
}

bool Media::addLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Both file & label need to be inserted in database before being linked together" );
        return false;
    }
    try
    {
        auto t = m_ml->getConn()->newTransaction();

        std::string req = "INSERT INTO " + Label::FileRelationTable::Name + " VALUES(?, ?)";
        if ( sqlite::Tools::executeInsert( m_ml->getConn(), req, label->id(), m_id ) == 0 )
            return false;
        const std::string reqFts = "UPDATE " + Media::FtsTable::Name + " "
            "SET labels = labels || ' ' || ? WHERE rowid = ?";
        if ( sqlite::Tools::executeUpdate( m_ml->getConn(), reqFts, label->name(), m_id ) == false )
            return false;
        t->commit();
        return true;
    }
    catch ( const sqlite::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to add label: ", ex.what() );
        return false;
    }
}

bool Media::removeLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Can't unlink a label/file not inserted in database" );
        return false;
    }
    try
    {
        auto t = m_ml->getConn()->newTransaction();

        std::string req = "DELETE FROM " + Label::FileRelationTable::Name + " WHERE label_id = ? AND media_id = ?";
        if ( sqlite::Tools::executeDelete( m_ml->getConn(), req, label->id(), m_id ) == false )
            return false;
        const std::string reqFts = "UPDATE " + Media::FtsTable::Name + " "
                "SET labels = TRIM(REPLACE(labels, ?, '')) WHERE rowid = ?";
        if ( sqlite::Tools::executeUpdate( m_ml->getConn(), reqFts, label->name(), m_id ) == false )
            return false;
        t->commit();
        return true;
    }
    catch ( const sqlite::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to remove label: ", ex.what() );
        return false;
    }
}

Query<IMedia> Media::search( MediaLibraryPtr ml, const std::string& title,
                             const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, false );

    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND m.import_type = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( title ),
                                      ImportType::Internal );
}

Query<IMedia> Media::search( MediaLibraryPtr ml, const std::string& title,
                             Media::Type type, const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true );
    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND (f.type = ? OR f.type = ?)"
            " AND m.type = ?"
            " AND m.import_type = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( title ),
                                      File::Type::Main, File::Type::Disc,
                                      type, ImportType::Internal );
}

Query<IMedia> Media::searchAlbumTracks(MediaLibraryPtr ml, const std::string& pattern, int64_t albumId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true );
    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND m.album_id = ?"
            " AND f.type = ?"
            " AND m.subtype = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      albumId, File::Type::Main, Media::SubType::AlbumTrack );
}

Query<IMedia> Media::searchArtistTracks(MediaLibraryPtr ml, const std::string& pattern, int64_t artistId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true );

    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND m.artist_id = ?"
            " AND f.type = ?"
            " AND m.subtype = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      artistId, File::Type::Main, Media::SubType::AlbumTrack );
}

Query<IMedia> Media::searchGenreTracks(MediaLibraryPtr ml, const std::string& pattern, int64_t genreId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true );

    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND m.genre_id = ?"
            " AND f.type = ?"
            " AND m.subtype = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      genreId, File::Type::Main, Media::SubType::AlbumTrack );
}

Query<IMedia> Media::searchShowEpisodes(MediaLibraryPtr ml, const std::string& pattern,
                                        int64_t showId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true );

    req +=  " INNER JOIN " + ShowEpisode::Table::Name + " ep ON ep.media_id = m.id_media "
            " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND ep.show_id = ?"
            " AND f.type = ?"
            " AND m.subtype = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      showId, File::Type::Main, Media::SubType::ShowEpisode );
}

Query<IMedia> Media::searchInPlaylist( MediaLibraryPtr ml, const std::string& pattern,
                                       int64_t playlistId, const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true );

    req += "LEFT JOIN " + Playlist::MediaRelationTable::Name + " pmr "
           "ON pmr.media_id = m.id_media "
           "WHERE pmr.playlist_id = ?";

    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";

    req += " AND m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
           " WHERE " + Media::FtsTable::Name + " MATCH ?)";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), playlistId,
                                      sqlite::Tools::sanitizePattern( pattern ) );
}

Query<IMedia> Media::fetchHistory( MediaLibraryPtr ml )
{
    static const std::string req = "FROM " + Media::Table::Name +
            " WHERE last_played_date IS NOT NULL"
            " AND import_type != ?";
    return make_query<Media, IMedia>( ml, "*", req,
                                      "ORDER BY last_played_date DESC",
                                      ImportType::Stream );
}

Query<IMedia> Media::fetchHistoryByType( MediaLibraryPtr ml, IMedia::Type type )
{
    static const std::string req = "FROM " + Media::Table::Name +
            " WHERE last_played_date IS NOT NULL"
            " AND type = ? AND import_type = ?";
    return make_query<Media, IMedia>( ml, "*", req,
                                      "ORDER BY last_played_date DESC", type,
                                      ImportType::Internal );
}

Query<IMedia> Media::fetchHistory( MediaLibraryPtr ml, IMedia::Type type )
{
    assert( type == IMedia::Type::Audio || type == IMedia::Type::Video );
    return fetchHistoryByType( ml, type );
}

Query<IMedia> Media::fetchStreamHistory(MediaLibraryPtr ml)
{
    static const std::string req = "FROM " + Media::Table::Name +
            " WHERE last_played_date IS NOT NULL"
            " AND import_type = ?";
    return make_query<Media, IMedia>( ml, "*", req,
                                      "ORDER BY last_played_date DESC",
                                      ImportType::Stream );
}

Query<IMedia> Media::fromFolderId( MediaLibraryPtr ml, IMedia::Type type,
                                   int64_t folderId, const QueryParameters* params )
{
    /*
     * We don't need to filter by presence. If the user is trying to list their
     * media from a missing folder, they want the missing media to be displayed
     * Otherwise, the folder containing the missing media will not be returned
     * to them.
     */
    std::string req = "FROM " + Table::Name +  " m ";
    req += addRequestJoin( params, false );
    req += " WHERE m.folder_id = ?";
    if ( type != Type::Unknown )
    {
        req += " AND m.type = ?";
        return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                          folderId, type );
    }
    // Don't explicitely filter by type since only video/audio media have a
    // non NULL folder_id
    return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                      folderId );
}

Query<IMedia> Media::searchFromFolderId( MediaLibraryPtr ml,
                                         const std::string& pattern,
                                         IMedia::Type type, int64_t folderId,
                                         const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name +  " m ";
    req += addRequestJoin( params, false );
    req += " WHERE m.folder_id = ?";
    req += " AND m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
    " WHERE " + Media::FtsTable::Name + " MATCH ?)";
    if ( type != Type::Unknown )
    {
        req += " AND m.type = ?";
        return make_query<Media, IMedia>( ml, "*", req, sortRequest( params ),
                                          folderId, pattern, type );
    }
    // Don't explicitely filter by type since only video/audio media have a
    // non NULL folder_id
    return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                      folderId,
                                      sqlite::Tools::sanitizePattern( pattern ) );
}

Query<IMedia> Media::fromMediaGroup(MediaLibraryPtr ml, int64_t groupId, Type type,
                                     const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " m ";
    req += addRequestJoin( params, false );
    req += " WHERE m.group_id = ? AND m.import_type = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    if ( type != Type::Unknown )
    {
        req += " AND m.type = ?";
        return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                          groupId, ImportType::Internal, type );
    }
    return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                      groupId, ImportType::Internal );
}

Query<IMedia> Media::searchFromMediaGroup( MediaLibraryPtr ml, int64_t groupId,
                                           IMedia::Type type,
                                           const std::string& pattern,
                                           const QueryParameters* params )
{
    if ( pattern.size() < 3 )
        return nullptr;
    std::string req = "FROM " + Table::Name + " m ";
    req += addRequestJoin( params, false );
    req += " WHERE m.id_media IN (SELECT rowid FROM " + FtsTable::Name +
            " WHERE " + FtsTable::Name + " MATCH ?)"
           " AND m.group_id = ? AND m.import_type = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    if ( type != Type::Unknown )
    {
        req += " AND m.type = ?";
        return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                          sqlite::Tools::sanitizePattern( pattern ),
                                          groupId, ImportType::Internal, type );
    }
    return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      groupId, ImportType::Internal );
}

bool Media::clearHistory( MediaLibraryPtr ml )
{
    auto dbConn = ml->getConn();
    auto t = dbConn->newTransaction();
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "last_position = -1, last_time = -1, play_count = 0,"
            "last_played_date = NULL";

    if ( sqlite::Tools::executeUpdate( dbConn, req ) == false )
        return false;
    t->commit();
    ml->getCb()->onHistoryChanged( HistoryType::Media );
    ml->getCb()->onHistoryChanged( HistoryType::Network );
    return true;
}

bool Media::removeOldMedia( MediaLibraryPtr ml, std::chrono::seconds maxLifeTime )
{
    // Media that were never played have a real_last_played_date = NULL, so they
    // won't match for real_last_played_date < X
    // However we need to take care about media that were inserted but never played
    const std::string req = "DELETE FROM " + Media::Table::Name + " "
            "WHERE ( last_played_date < ? OR "
                "( last_played_date IS NULL AND insertion_date < ? ) )"
            "AND import_type != ? "
            "AND nb_playlists = 0";
    auto deadline = std::chrono::duration_cast<std::chrono::seconds>(
                (std::chrono::system_clock::now() - maxLifeTime).time_since_epoch() );
    return sqlite::Tools::executeDelete( ml->getConn(), req, deadline.count(),
                                         deadline.count(),
                                         ImportType::Internal );
}

bool Media::resetSubTypes( MediaLibraryPtr ml )
{
    const std::string req = "UPDATE " + Media::Table::Name +
            /* Reset all subtypes */
            " SET subtype = ?, "
            /* Reset album track related properties */
            "album_id = NULL, track_number = 0, disc_number = 0, artist_id = NULL,"
            "genre_id = NULL "
            "WHERE type = ? OR type = ?";
    return sqlite::Tools::executeUpdate( ml->getConn(), req, SubType::Unknown,
                                         Type::Video, Type::Audio );
}

bool Media::regroupAll( MediaLibraryPtr ml )
{
    const std::string req = "SELECT m.* FROM " + Table::Name + " m "
                           " INNER JOIN " + MediaGroup::Table::Name + " mg ON "
                               " m.group_id = mg.id_group "
                           " WHERE mg.forced_singleton != 0 LIMIT 1";
    while ( true )
    {
        auto m = fetch( ml, req );
        if ( m == nullptr )
            return true;
        if ( m->regroup() == false )
            return false;
    }
}

Query<IMedia> Media::tracksFromGenre( MediaLibraryPtr ml, int64_t genreId,
                                      IGenre::TracksIncluded included,
                                      const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " m"
            " WHERE m.genre_id = ?1 AND m.is_present = 1";
    if ( included == IGenre::TracksIncluded::WithThumbnailOnly )
    {
        req += " AND EXISTS(SELECT entity_id FROM " + Thumbnail::LinkingTable::Name +
               " WHERE entity_id = m.id_media AND entity_type = ?2)";
    }
    std::string orderBy = "ORDER BY ";
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    switch ( sort )
    {
    case SortingCriteria::Duration:
        orderBy += "m.duration";
        break;
    case SortingCriteria::InsertionDate:
        orderBy += "m.insertion_date";
        break;
    case SortingCriteria::ReleaseDate:
        orderBy += "m.release_date";
        break;
    case SortingCriteria::Alpha:
        orderBy += "m.title";
        break;
    default:
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default" );
        /* fall-through */
    case SortingCriteria::Default:
        if ( desc == true )
            orderBy += "m.artist_id DESC, m.album_id DESC, m.disc_number DESC, m.track_number DESC, m.filename";
        else
            orderBy += "m.artist_id, m.album_id, m.disc_number, m.track_number, m.filename";
        break;
    }

    if ( desc == true )
        orderBy += " DESC";
    if ( included == IGenre::TracksIncluded::WithThumbnailOnly )
        return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                          std::move( orderBy ), genreId,
                                          Thumbnail::EntityType::Media );
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      std::move( orderBy ), genreId );
}

}
