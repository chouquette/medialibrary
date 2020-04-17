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
#include "AlbumTrack.h"
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

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"
#include "VideoTrack.h"
#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/Errors.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "thumbnails/ThumbnailerWorker.h"

namespace medialibrary
{

const std::string Media::Table::Name = "Media";
const std::string Media::Table::PrimaryKeyColumn = "id_media";
int64_t Media::* const Media::Table::PrimaryKey = &Media::m_id;
const std::string Media::FtsTable::Name = "MediaFts";

Media::Media( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    // DB field extraction:
    , m_id( row.load<decltype(m_id)>( 0 ) )
    , m_type( row.load<decltype(m_type)>( 1 ) )
    , m_subType( row.load<decltype(m_subType)>( 2 ) )
    , m_duration( row.load<decltype(m_duration)>( 3 ) )
    , m_progress( row.load<decltype(m_progress)>( 4 ) )
    , m_playCount( row.load<decltype(m_playCount)>( 5 ) )
    , m_lastPlayedDate( row.load<decltype(m_lastPlayedDate)>( 6 ) )
    // skip real_last_played_date as we don't need it in memory
    , m_insertionDate( row.load<decltype(m_insertionDate)>( 8 ) )
    , m_releaseDate( row.load<decltype(m_releaseDate)>( 9 ) )
    , m_title( row.load<decltype(m_title)>( 10 ) )
    , m_filename( row.load<decltype(m_filename)>( 11 ) )
    , m_isFavorite( row.load<decltype(m_isFavorite)>( 12 ) )
    // Skip is_present
    , m_deviceId( row.load<decltype(m_deviceId)>( 14 ) )
    , m_nbPlaylists( row.load<unsigned int>( 15 ) )
    , m_folderId( row.load<decltype(m_folderId)>( 16 ) )
    , m_importType( row.load<decltype(m_importType)>( 17 ) )
    , m_groupId( row.load<decltype(m_groupId)>( 18 ) )
    , m_forcedTitle( row.load<decltype(m_forcedTitle)>( 19 ) )

    // End of DB fields extraction
    , m_metadata( m_ml, IMetadata::EntityType::Media )
    , m_changed( false )
{
    assert( row.nbColumns() == 20 );
}

Media::Media( MediaLibraryPtr ml, const std::string& title, Type type,
              int64_t duration, int64_t deviceId, int64_t folderId )
    : m_ml( ml )
    , m_id( 0 )
    , m_type( type )
    , m_subType( SubType::Unknown )
    , m_duration( duration )
    , m_progress( -1.0f )
    , m_playCount( 0 )
    , m_lastPlayedDate( 0 )
    , m_insertionDate( time( nullptr ) )
    , m_releaseDate( 0 )
    , m_title( title )
    // When creating a Media, meta aren't parsed, and therefor, the title is the filename
    , m_filename( title )
    , m_isFavorite( false )
    , m_deviceId( deviceId )
    , m_nbPlaylists( 0 )
    , m_folderId( folderId )
    , m_importType( ImportType::Internal )
    , m_groupId( 0 )
    , m_forcedTitle( false )
    , m_metadata( m_ml, IMetadata::EntityType::Media )
    , m_changed( false )
{
}

Media::Media(MediaLibraryPtr ml, const std::string& fileName, ImportType importType )
    : m_ml( ml )
    , m_id( 0 )
    , m_type( Type::Unknown )
    , m_subType( SubType::Unknown )
    , m_duration( -1 )
    , m_progress( -1.0f )
    , m_playCount( 0 )
    , m_lastPlayedDate( 0 )
    , m_insertionDate( time( nullptr ) )
    , m_releaseDate( 0 )
    , m_title( fileName )
    , m_filename( fileName )
    , m_isFavorite( false )
    , m_deviceId( 0 )
    , m_nbPlaylists( 0 )
    , m_folderId( 0 )
    , m_importType( importType )
    , m_groupId( 0 )
    , m_forcedTitle( false )
    , m_metadata( m_ml, IMetadata::EntityType::Media )
    , m_changed( false )
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
                                                   ImportType importType )
{
    std::unique_ptr<sqlite::Transaction> t;
    if ( sqlite::Transaction::transactionInProgress() == false )
        t = ml->getConn()->newTransaction();

    auto fileName = utils::url::decode( utils::file::fileName( mrl ) );
    auto self = std::make_shared<Media>( ml, fileName, importType );
    static const std::string req = "INSERT INTO " + Media::Table::Name +
            "(type, insertion_date, title, filename, import_type) "
            "VALUES(?, ?, ?, ?, ?)";

    if ( insert( ml, self, req, Type::Unknown, self->m_insertionDate,
                 self->m_title, self->m_filename, importType ) == false )
        return nullptr;

    if ( self->addExternalMrl( mrl, IFile::Type::Main ) == nullptr )
        return nullptr;

    if ( t != nullptr )
        t->commit();
    return self;
}

std::shared_ptr<Media> Media::createExternal( MediaLibraryPtr ml,
                                              const std::string& fileName )
{
    return createExternalMedia( ml, fileName, ImportType::External );
}

std::shared_ptr<Media> Media::createStream( MediaLibraryPtr ml, const std::string& fileName )
{
    return createExternalMedia( ml, fileName, ImportType::Stream );
}

AlbumTrackPtr Media::albumTrack() const
{
    if ( m_subType != SubType::AlbumTrack )
        return nullptr;
    if ( m_albumTrack == nullptr )
        m_albumTrack = AlbumTrack::fromMedia( m_ml, m_id );
    return m_albumTrack;
}

void Media::setAlbumTrack( AlbumTrackPtr albumTrack )
{
    m_albumTrack = albumTrack;
    m_subType = SubType::AlbumTrack;
    m_changed = true;
}

int64_t Media::duration() const
{
    return m_duration;
}

float Media::progress() const
{
    return m_progress;
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

    if ( m_showEpisode == nullptr )
        m_showEpisode = ShowEpisode::fromMedia( m_ml, m_id );
    return m_showEpisode;
}

void Media::setShowEpisode( ShowEpisodePtr episode )
{
    m_showEpisode = episode;
    m_subType = SubType::ShowEpisode;
    m_changed = true;
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

bool Media::increasePlayCount()
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "play_count = ?, last_played_date = ?, real_last_played_date = ? "
            "WHERE id_media = ?";
    auto lastPlayedDate = time( nullptr );
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_playCount + 1,
                                       lastPlayedDate, lastPlayedDate, m_id ) == false )
        return false;
    m_playCount++;
    m_lastPlayedDate = lastPlayedDate;
    auto historyType = ( m_type == Type::Video || m_type == Type::Audio ) ?
                       HistoryType::Media : HistoryType::Network;
    m_ml->getCb()->onHistoryChanged( historyType );
    return true;
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
            "play_count = ?, last_played_date = ? WHERE id_media = ?";
    auto dbConn = m_ml->getConn();
    auto t = dbConn->newTransaction();

    if ( sqlite::Tools::executeUpdate( dbConn, req, 0, nullptr, m_id ) == false )
        return false;
    unsetMetadata( MetadataType::Progress );

    t->commit();
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

void Media::setMovie(MoviePtr movie)
{
    m_movie = movie;
    m_subType = SubType::Movie;
    m_changed = true;
}

bool Media::addVideoTrack( const std::string& codec, unsigned int width, unsigned int height,
                           uint32_t fpsNum, uint32_t fpsDen, uint32_t bitrate,
                           uint32_t sarNum, uint32_t sarDen, const std::string& language,
                           const std::string& description )
{
    return VideoTrack::create( m_ml, codec, width, height, fpsNum, fpsDen,
                               bitrate, sarNum, sarDen, m_id, language, description ) != nullptr;
}

Query<IVideoTrack> Media::videoTracks() const
{
    static const std::string req = "FROM " + VideoTrack::Table::Name +
            " WHERE media_id = ?";
    return make_query<VideoTrack, IVideoTrack>( m_ml, "*", req, "", m_id );
}

bool Media::addAudioTrack( const std::string& codec, unsigned int bitrate,
                          unsigned int sampleRate, unsigned int nbChannels,
                          const std::string& language, const std::string& desc )
{
    return AudioTrack::create( m_ml, codec, bitrate, sampleRate, nbChannels, language, desc, m_id ) != nullptr;
}

bool Media::addSubtitleTrack( std::string codec, std::string language,
                              std::string description, std::string encoding )
{
    return SubtitleTrack::create( m_ml, std::move( codec ), std::move( language ),
                                  std::move( description ), std::move( encoding ),
                                  m_id ) != nullptr;
}

Query<IAudioTrack> Media::audioTracks() const
{
    static const std::string req = "FROM " + AudioTrack::Table::Name +
            " WHERE media_id = ?";
    return make_query<AudioTrack, IAudioTrack>( m_ml, "*", req, "", m_id );
}

Query<ISubtitleTrack> Media::subtitleTracks() const
{
    static const std::string req = "FROM " + SubtitleTrack::Table::Name +
            " WHERE media_id = ?";
    return make_query<SubtitleTrack, ISubtitleTrack>( m_ml, "*", req, "", m_id );
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

bool Media::setMetadata( std::unordered_map<IMedia::MetadataType, std::string> metadata )
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

void Media::setReleaseDate( unsigned int date )
{
    if ( m_releaseDate == date )
        return;
    m_releaseDate = date;
    m_changed = true;
}

int64_t Media::deviceId() const
{
    return m_deviceId;
}

void Media::setDeviceId( int64_t deviceId )
{
    if ( m_deviceId == deviceId )
        return;
    m_deviceId = deviceId;
    m_changed = true;
}

int64_t Media::folderId() const
{
    return m_folderId;
}

void Media::setFolderId( int64_t folderId )
{
    if ( m_folderId == folderId )
        return;
    m_folderId = folderId;
    m_changed = true;
}

void Media::markAsInternal()
{
    if ( m_importType == ImportType::Internal )
        return;
    m_importType = ImportType::Internal;
    m_changed = true;
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
    currentThumbnail = Thumbnail::updateOrReplace( m_ml, currentThumbnail,
                                                   newThumbnail,
                                                   Media::shouldUpdateThumbnail,
                                                   m_id, Thumbnail::EntityType::Media );
    auto res = currentThumbnail != nullptr;
    m_thumbnails[thumbnailIdx] = std::move( currentThumbnail );
    return res;
}

void Media::removeThumbnail( ThumbnailSizeType sizeType )
{
    auto t = thumbnail( sizeType );
    if ( t == nullptr )
        return;
    t->unlinkThumbnail( m_id, Thumbnail::EntityType::Media );
    m_thumbnails[Thumbnail::SizeToInt( sizeType )] = nullptr;
}

bool Media::setThumbnail( const std::string& thumbnailMrl, ThumbnailSizeType sizeType )
{
    return setThumbnail( std::make_shared<Thumbnail>( m_ml, thumbnailMrl,
                                                      Thumbnail::Origin::UserProvided,
                                                      sizeType, false ) );
}

bool Media::save()
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "type = ?, subtype = ?, duration = ?, release_date = ?,"
            "title = ?, device_id = ?, folder_id = ?, import_type = ? "
            "WHERE id_media = ?";
    if ( m_changed == false )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_type, m_subType, m_duration,
                                       m_releaseDate, m_title,
                                       sqlite::ForeignKey{ m_deviceId },
                                       sqlite::ForeignKey{ m_folderId },
                                       m_importType,
                                       m_id ) == false )
    {
        return false;
    }
    m_changed = false;
    return true;
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

std::string Media::addRequestJoin( const QueryParameters* params, bool forceFile,
                                    bool forceAlbumTrack )
{
    bool albumTrack = forceAlbumTrack;
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
            albumTrack = true;
            break;
        case SortingCriteria::Album:
            /* We need the album track to get the album id & the album for its title */
            albumTrack = true;
            album = true;
            break;
        case SortingCriteria::TrackId:
            album = true;
            albumTrack = true;
            break;
        case SortingCriteria::NbAudio:
        case SortingCriteria::NbVideo:
        case SortingCriteria::NbMedia:
        case SortingCriteria::TrackNumber:
            // Unrelated to media requests
            break;
    }
    std::string req;
    // Use "LEFT JOIN to allow for ordering different media type
    // For instance ordering by albums on all media would not fetch the video if
    // we were using INNER JOIN
    if ( albumTrack == true )
        req += " LEFT JOIN " + AlbumTrack::Table::Name + " att ON m.id_media = att.media_id ";
    if ( album == true )
    {
        assert( albumTrack == true );
        req += " LEFT JOIN " + Album::Table::Name + " alb ON att.album_id = alb.id_album ";
    }
    if ( artist == true )
    {
        assert( albumTrack == true );
        req += " LEFT JOIN " + Artist::Table::Name + " art ON att.artist_id = art.id_artist ";
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
            req += "alb.title DESC, att.track_number";
        else
            req += "alb.title, att.track_number";
        descAdded = true;
        break;
    case SortingCriteria::Artist:
        req += "art.name";
        break;
    case SortingCriteria::TrackId:
        if ( desc == true )
            req += "alb.title, att.track_number DESC, att.disc_number";
        else
            req += "alb.title, att.track_number, att.disc_number";
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
                              const QueryParameters* params )
{
    assert( type == IMedia::Type::Audio || type == IMedia::Type::Video );
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, false );
    // We want to include unknown media to the video listing, so we invert the
    // filter to exclude Audio (ie. we include Unknown & Video)
    // If we only want audio media, then we just filter 'type = Audio'
    if ( type == IMedia::Type::Video )
        req += " WHERE m.type != ?";
    else
        req += " WHERE m.type = ?";
    req +=  " AND (f.type = ? OR f.type = ?)"
            " AND f.is_external = 0"
            " AND m.is_present != 0";

    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), IMedia::Type::Audio,
                                      IFile::Type::Main, IFile::Type::Disc );
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
    const std::string req = "UPDATE " + Table::Name + " SET type = ? "
            "WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, type, m_id ) == false )
        return false;
    if ( m_type == IMedia::Type::Unknown )
        parser::Task::createMediaRefreshTask( m_ml, shared_from_this() );
    m_type = type;
    return true;
}

IMedia::SubType Media::subType() const
{
    return m_subType;
}

void Media::setSubType( IMedia::SubType subType )
{
    if ( subType == m_subType )
        return;
    switch ( m_subType )
    {
        case IMedia::SubType::AlbumTrack:
            m_albumTrack = nullptr;
            break;
        case IMedia::SubType::ShowEpisode:
            m_showEpisode = nullptr;
            break;
        case IMedia::SubType::Movie:
            m_movie = nullptr;
            break;
        case IMedia::SubType::Unknown:
            break;
    }
    m_subType = subType;
    m_changed = true;
}

void Media::setTypeBuffered( Type type )
{
    if ( m_type == type )
        return;
    m_type = type;
    m_changed = true;
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

void Media::setTitleBuffered( const std::string& title )
{
    if ( m_title == title || m_forcedTitle == true )
        return;
    m_title = title;
    m_changed = true;
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
                                   index( Indexes::LastUsageDate,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::Folder,
                                          Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::MediaGroup,
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
    auto req = "CREATE TABLE " + Table::Name +
    "("
        "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
        "type INTEGER,"
        "subtype INTEGER NOT NULL DEFAULT " +
            std::to_string( static_cast<typename std::underlying_type<SubType>::type>(
                                SubType::Unknown ) ) + ","
        "duration INTEGER DEFAULT -1,";

    if ( dbModel >= 27 )
        req += "progress REAL DEFAULT -1,";

    req +=
        "play_count UNSIGNED INTEGER,"
        "last_played_date UNSIGNED INTEGER,"
        "real_last_played_date UNSIGNED INTEGER,"
        "insertion_date UNSIGNED INTEGER,"
        "release_date UNSIGNED INTEGER,";

    if ( dbModel < 17 )
        req += "thumbnail_id INTEGER,";

    req += "title TEXT COLLATE NOCASE,"
        "filename TEXT COLLATE NOCASE,"
        "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
        "is_present BOOLEAN NOT NULL DEFAULT 1,"
        "device_id INTEGER,"
        "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
        "folder_id UNSIGNED INTEGER,";
    if ( dbModel >= 23 )
    {
        req += "import_type UNSIGNED INTEGER NOT NULL,";
    }
    if ( dbModel >= 24 )
    {
        req += "group_id UNSIGNED INTEGER,";
        if ( dbModel == 24 )
            req += "has_been_grouped BOOLEAN NOT NULL DEFAULT 0,";
        req += "forced_title BOOLEAN NOT NULL DEFAULT 0,";
    }
    if ( dbModel < 17 )
    {
          req += "FOREIGN KEY(thumbnail_id) REFERENCES " + Thumbnail::Table::Name
            + "(id_thumbnail),";
    }
    if ( dbModel >= 24 )
    {
        req += "FOREIGN KEY(group_id) REFERENCES " + MediaGroup::Table::Name +
                "(id_group) ON DELETE RESTRICT,";
    }
    req += "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
        + "(id_folder)"

    ")";
    return req;
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
                       std::to_string( static_cast<std::underlying_type_t<File::Type>>(
                                           IFile::Type::Main ) ) +
                   " OR old.type = " +
                       std::to_string( static_cast<std::underlying_type_t<File::Type>>(
                                           IFile::Type::Disc ) ) +
                   " BEGIN "
                   " DELETE FROM " + Table::Name +
                       " WHERE id_media=old.media_id;"
                   " END";
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
        case Indexes::LastUsageDate:
            assert( dbModel >= 14 );
            // Don't create this index before model 14, as the real_last_played_date
            // column was introduced in model version 14
            return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(last_played_date, "
                            "real_last_played_date, insertion_date)";
        case Indexes::Folder:
            assert( dbModel >= 22 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(folder_id)";
        case Indexes::MediaGroup:
            assert( dbModel >= 24 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(group_id)";
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
        case Indexes::LastUsageDate:
            assert( dbModel >= 14 );
            // Don't create this index before model 14, as the real_last_played_date
            // column was introduced in model version 14
            return "media_last_usage_dates_idx";
        case Indexes::Folder:
            assert( dbModel >= 22 );
            return "media_folder_id_idx";
        case Indexes::MediaGroup:
            assert( dbModel >= 24 );
            return "media_group_id_idx";
        default:
            assert( !"Invalid index provided" );
    }
    return "<invalid request>";
}

bool Media::checkDbModel( MediaLibraryPtr ml )
{
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
            checkIndex( ml->getConn(), Indexes::LastUsageDate ) &&
            checkIndex( ml->getConn(), Indexes::Folder ) &&
            checkIndex( ml->getConn(), Indexes::MediaGroup );
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

    req += addRequestJoin( params, false, false );

    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND m.is_present = 1"
            " AND m.import_type = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( title ),
                                      ImportType::Internal );
}

Query<IMedia> Media::search( MediaLibraryPtr ml, const std::string& title,
                             Media::Type type, const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, false );
    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND m.is_present = 1"
            " AND (f.type = ? OR f.type = ?)"
            " AND m.type = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( title ),
                                      File::Type::Main, File::Type::Disc,
                                      type );
}

Query<IMedia> Media::searchAlbumTracks(MediaLibraryPtr ml, const std::string& pattern, int64_t albumId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, true );
    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND att.album_id = ?"
            " AND m.is_present = 1"
            " AND f.type = ?"
            " AND m.subtype = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      albumId, File::Type::Main, Media::SubType::AlbumTrack );
}

Query<IMedia> Media::searchArtistTracks(MediaLibraryPtr ml, const std::string& pattern, int64_t artistId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, true );

    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND att.artist_id = ?"
            " AND m.is_present = 1"
            " AND f.type = ?"
            " AND m.subtype = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      artistId, File::Type::Main, Media::SubType::AlbumTrack );
}

Query<IMedia> Media::searchGenreTracks(MediaLibraryPtr ml, const std::string& pattern, int64_t genreId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, true );

    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND att.genre_id = ?"
            " AND m.is_present = 1"
            " AND f.type = ?"
            " AND m.subtype = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      genreId, File::Type::Main, Media::SubType::AlbumTrack );
}

Query<IMedia> Media::searchShowEpisodes(MediaLibraryPtr ml, const std::string& pattern,
                                        int64_t showId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, false );

    req +=  " INNER JOIN " + ShowEpisode::Table::Name + " ep ON ep.media_id = m.id_media "
            " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name +
            " WHERE " + Media::FtsTable::Name + " MATCH ?)"
            " AND ep.show_id = ?"
            " AND m.is_present = 1"
            " AND f.type = ?"
            " AND m.subtype = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      showId, File::Type::Main, Media::SubType::ShowEpisode );
}

Query<IMedia> Media::searchInPlaylist( MediaLibraryPtr ml, const std::string& pattern,
                                       int64_t playlistId, const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, false );

    req += "LEFT JOIN " + Playlist::MediaRelationTable::Name + " pmr "
           "ON pmr.media_id = m.id_media "
           "WHERE pmr.playlist_id = ? AND m.is_present != 0 AND "
           "m.id_media IN (SELECT rowid FROM " + Media::FtsTable::Name + " "
           "WHERE " + Media::FtsTable::Name + " MATCH ?)";
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
    // This assumes the folder is present, as folders are not expected to be
    // manipulated when the device is not present
    std::string req = "FROM " + Table::Name +  " m ";
    req += addRequestJoin( params, false, false );
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
    req += addRequestJoin( params, false, false );
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
    req += addRequestJoin( params, false, false );
    req += " WHERE m.group_id = ?"
           " AND m.is_present != 0";
    if ( type != Type::Unknown )
    {
        req += " AND m.type = ?";
        return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                          groupId, type );
    }
    return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ), groupId );
}

Query<IMedia> Media::searchFromMediaGroup( MediaLibraryPtr ml, int64_t groupId,
                                           IMedia::Type type,
                                           const std::string& pattern,
                                           const QueryParameters* params )
{
    if ( pattern.size() < 3 )
        return nullptr;
    std::string req = "FROM " + Table::Name + " m ";
    req += addRequestJoin( params, false, false );
    req += " WHERE m.id_media IN (SELECT rowid FROM " + FtsTable::Name +
            " WHERE " + FtsTable::Name + " MATCH ?)"
           " AND m.group_id = ?"
           " AND m.is_present != 0";
    if ( type != Type::Unknown )
    {
        req += " AND m.type = ?";
        return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                          sqlite::Tools::sanitizePattern( pattern ),
                                          groupId, type );
    }
    return make_query<Media, IMedia>( ml, "m.*", req, sortRequest( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      groupId );
}

bool Media::clearHistory( MediaLibraryPtr ml )
{
    auto dbConn = ml->getConn();
    auto t = dbConn->newTransaction();
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "play_count = 0,"
            "last_played_date = NULL";

    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    if ( Metadata::unset( dbConn, IMetadata::EntityType::Media,
                    static_cast<MDType>( IMedia::MetadataType::Progress ) ) == false )
        return false;

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
            "WHERE ( real_last_played_date < ? OR "
                "( real_last_played_date IS NULL AND insertion_date < ? ) )"
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
            " SET subtype = ? WHERE type = ? OR type = ?";
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

}
