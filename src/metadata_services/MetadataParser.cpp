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

#include "MetadataParser.h"
#include "Album.h"
#include "Artist.h"
#include "File.h"
#include "medialibrary/filesystem/Errors.h"
#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "Folder.h"
#include "Genre.h"
#include "Media.h"
#include "Playlist.h"
#include "Show.h"
#include "ShowEpisode.h"
#include "Movie.h"
#include "utils/Defer.h"
#include "utils/Directory.h"
#include "utils/File.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "utils/Date.h"
#include "utils/ModificationsNotifier.h"
#include "utils/TitleAnalyzer.h"
#include "utils/Date.h"
#include "discoverer/FsDiscoverer.h"
#include "Device.h"
#include "VideoTrack.h"
#include "AudioTrack.h"
#include "SubtitleTrack.h"
#include "parser/Task.h"
#include "parser/Parser.h"
#include "MediaGroup.h"
#include "Subscription.h"

#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <stack>

namespace medialibrary
{
namespace parser
{

MetadataAnalyzer::MetadataAnalyzer()
    : m_ml( nullptr )
    , m_previousFolderId( 0 )
    , m_flushed( false )
    , m_stopped( false )
{
}

bool MetadataAnalyzer::cacheUnknownArtist()
{
    m_unknownArtist = Artist::fetch( m_ml, UnknownArtistID );
    if ( m_unknownArtist == nullptr )
        LOG_ERROR( "Failed to cache unknown artist" );
    return m_unknownArtist != nullptr;
}

bool MetadataAnalyzer::cacheUnknownShow()
{
    m_unknownShow = Show::fetch( m_ml, UnknownShowID );
    if ( m_unknownShow == nullptr )
    {
        LOG_ERROR( "Failed to cache unknown show" );
        return false;
    }
    return true;
}

bool MetadataAnalyzer::initialize( IMediaLibrary* ml )
{
    m_ml = static_cast<MediaLibrary*>( ml );
    m_notifier = m_ml->getNotifier();
    return cacheUnknownArtist() && cacheUnknownShow();
}

int MetadataAnalyzer::toInt( IItem& item, IItem::Metadata meta )
{
    auto str = item.meta( meta );
    if ( str.empty() == false )
    {
        try
        {
            return std::stoi( str );
        }
        catch( const std::logic_error& ex )
        {
            LOG_WARN( "Invalid meta #",
                      static_cast<typename std::underlying_type<IItem::Metadata>::type>( meta ),
                      " provided (", str, "): ", ex.what() );
        }
    }
    return 0;
}

Status MetadataAnalyzer::run( IItem& item )
{
    if ( item.isRefresh() )
    {
        LOG_DEBUG( "Refreshing MRL: ", item.mrl() );
        bool success;
        bool needRescan;
        std::tie( success, needRescan ) = refreshFile( item );
        if ( success == false )
            return Status::Fatal;
        auto file = static_cast<File*>( item.file().get() );
        // Now that the refresh request was processed, we can update the last
        // modification date in database
        if ( item.fileFs() != nullptr )
        {
            file->updateFsInfo( item.fileFs()->lastModificationDate(),
                                item.fileFs()->size() );
        }
        else
        {
            /*
             * A subscription might not be represented by a file on disk, for
             * instance if its a podcast, it's likely to just be an external
             * file pointing to the RSS feed for it.
             * In any other case so far, it is an invalid state.
             */
            assert( item.fileType() == IFile::Type::Subscription );
        }
        if ( needRescan == false )
            return Status::Success;
    }
    if ( item.fileType() == IFile::Type::Playlist )
    {
        assert( item.nbLinkedItems() > 0 );
        auto res = parsePlaylist( item );
        if ( res != Status::Success ) // playlist addition may fail due to constraint violation
            return res;

        assert( item.file() != nullptr || item.isRestore() == true );
        return Status::Completed;
    }
    if ( item.fileType() == IFile::Type::Subscription )
    {
        assert( item.nbLinkedItems() > 0 );
        auto res = parseSubscription( item );
        if ( res != Status::Success ) // playlist addition may fail due to constraint violation
            return res;

        return Status::Completed;
    }

    bool isFileModified = false;

    if ( item.file() == nullptr )
    {
        auto status = createFileAndMedia( item );
        if ( status != Status::Success )
            return status;
    }
    else if ( item.media() == nullptr )
    {
        // If we have a file but no media, this is a problem, we can analyze as
        // much as we want, but won't be able to store anything.
        // Keep in mind that if we are in this code path, we are not analyzing
        // a playlist.
        assert( false );
        return Status::Fatal;
    }
    else
    {
        // If we re-parsing this media, there should not be any tracks known for now
        // However in case we crashed or got interrupted in the middle of parsing,
        // we don't want to recreate tracks
        auto media = static_cast<Media*>( item.media().get() );
        if ( media->integratedAudioTracks()->count() == 0 &&
             media->videoTracks()->count() == 0 &&
             media->integratedSubtitleTracks()->count() == 0 )
        {
            try
            {
                auto t = m_ml->getConn()->newTransaction();
                createTracks( static_cast<Media&>( *item.media() ), item.tracks() );
                t->commit();
            }
            catch ( const sqlite::errors::ConstraintForeignKey& )
            {
                /* We're aiming at catching an error during the insertion of the
                 * tracks, which have a foreign key pointing to Media.id_media
                 * If the media was removed by the Discoverer thread, the insertion
                 * will fail, and we need to abort & discard this task, and the
                 * file/media don't exist anymore
                 */
                LOG_INFO( "Failed to add tracks to a media. Assuming it was "
                          "concurrently deleted" );
                return Status::Discarded;
            }
        }
        /* Best effort attempt to detect that a file changed while a rescan
         * was triggered. In this case, we want to avoid avoid creating a refresh
         * task in the future, since we're about to rescan it.
         * However, if we were to update the modification date now, a parsing
         * failure would prevent this file for being refreshed, so we postpone
         * the actual modification in db
         */
        if ( item.file()->lastModificationDate() !=
             item.fileFs()->lastModificationDate() )
            isFileModified = true;
    }
    auto media = std::static_pointer_cast<Media>( item.media() );

    if ( media->type() == IMedia::Type::Audio )
    {
        Cache cache{};
        {
            /*
             * Load the cache before running anything else. The current task list
             * might be flushed at any point while the task executes, which would
             * invalidate the cache.
             */
            std::lock_guard<compat::Mutex> lock{ m_cacheMutex };
            cache.previousAlbum = std::move( m_previousAlbum );
            cache.previousFolderId = m_previousFolderId;
            m_flushed = false;
        }
        auto status = parseAudioFile( item, cache );
        if ( status != Status::Success )
            return status;
        std::lock_guard<compat::Mutex> lock{ m_cacheMutex };
        if ( m_flushed == false )
        {
            m_previousAlbum = std::move( cache.previousAlbum );
            m_previousFolderId = cache.previousFolderId;
        }
    }
    else if ( media->type() == IMedia::Type::Video )
    {
        if (parseVideoFile( item ) == false )
            return Status::Fatal;
    }

    if ( isFileModified == true )
    {
        auto file = static_cast<File*>( item.file().get() );
        file->updateFsInfo( item.fileFs()->lastModificationDate(),
                            item.fileFs()->size() );
    }
    return Status::Success;
}

/* Playlist files */

Status MetadataAnalyzer::parsePlaylist( IItem& item ) const
{
    const auto& mrl = item.mrl();
    LOG_DEBUG( "Try to import ", mrl, " as a playlist" );
    std::shared_ptr<Playlist> playlistPtr;
    if ( item.file() != nullptr )
    {
        // We are most likely re-scanning a file representing a playlist.
        // If a task has a file, it means the playlist & the associated file have
        // been created.
        playlistPtr = Playlist::fromFile( m_ml, item.file()->id() );
        if ( playlistPtr == nullptr )
        {
            // The playlist had to be created, something is very wrong, give up
            // We *must* not discard the task though, the error might be sporadic
            // and we don't want to recreate this task during next reload.
            assert( false );
            return Status::Fatal;
        }
    }
    else
    {
        auto playlistName = item.meta( IItem::Metadata::Title );
        if ( playlistName.empty() == true )
            playlistName = utils::url::decode( utils::file::fileName( mrl ) );
        /*
         * Check if that MRL matches an existing external file. In which case
         * it might be linked to an external media which has been wrongfully
         * inserted as media instead of playlist
         */
        try
        {
            auto t = m_ml->getConn()->newTransaction();
            playlistPtr = Playlist::create( m_ml, std::move( playlistName ) );
            if ( playlistPtr == nullptr )
            {
                LOG_ERROR( "Failed to create playlist ", mrl, " to the media library" );
                return Status::Fatal;
            }

            // If we're in a restore task, we can accept a playlist without a parent
            // folder, as we're trying to restore a user-created playlist, which isn't
            // backed by an actual playlist file.
            // Actually we don't want to store any reference to the backup file even
            // if we have some, since this would make the playlist look like a file
            // backed one.
            if ( item.isRestore() == false )
            {
                auto deviceFs = item.parentFolderFs()->device();
                if ( deviceFs == nullptr )
                    throw fs::errors::DeviceRemoved{};
                auto file = File::fromExternalMrl( m_ml, mrl );
                if ( file != nullptr )
                {
                    LOG_DEBUG( "Reassigning file ", mrl, " to playlist" );
                    assert( file->isExternal() == true );
                    /*
                     * Mind the operations order. If we destroy the media before
                     * reassigning the file to the playlist, the file would be
                     * deleted through the foreign key
                     */
                    if ( file->update( *item.fileFs(), item.parentFolder()->id(),
                                       deviceFs->isRemovable() ) == false ||
                         file->setPlaylistId( playlistPtr->id() ) == false ||
                         Media::destroy( m_ml, file->mediaId() ) == false )
                    {
                        return Status::Fatal;
                    }
                }
                else
                {
                    file = File::createFromPlaylist( m_ml, playlistPtr->id(),
                                *item.fileFs(), item.parentFolder()->id(),
                                deviceFs->isRemovable(), playlistPtr->creationDate() );
                    if ( file == nullptr )
                        return Status::Fatal;
                }
                if ( item.setFile( std::move( file ) ) == false )
                    return Status::Fatal;
            }
            t->commit();
        }
        catch ( const sqlite::errors::ConstraintUnique& )
        {
            // Attempt to recover from some potentially invalid tasks records
            // See https://code.videolan.org/videolan/medialibrary/issues/166
            assert( sqlite::Transaction::isInProgress() == false );
            auto t = m_ml->getConn()->newTransaction();
            auto f = File::fromMrl( m_ml, mrl );
            if ( f != nullptr )
            {
                item.setFile( std::move( f ) );
                t->commit();
            }
        }
        m_notifier->notifyPlaylistCreation( playlistPtr );
    }
    // Now regardless of if the playlist is re-scanned or discovered from the
    // first time, just schedule all members for insertion. media & files will
    // be recreated if need be, and appropriate entries in PlaylistMediaRelation
    // table will be recreated to link things together.
    for ( auto i = 0u; i < item.nbLinkedItems(); ++i )
    {
        if ( m_stopped.load() == true )
            break;
        const auto& subItem = item.linkedItem( i );
        auto subItemMrl = subItem.mrl();
        if ( utils::url::schemeIs( "file://", subItemMrl ) == true )
        {
            auto path = utils::url::toLocalPath( subItemMrl );
            /*
             * If we're trying to add a directory, we need to crawl it to add
             * individual files. Otherwise we can just link a file MRL directly
             */
            if ( utils::fs::isDirectory( path ) == true )
            {
                addFolderToPlaylist( item, playlistPtr, subItem );
                continue;
            }
        }
        addPlaylistElement( item, playlistPtr->id(), subItem.mrl(),
                            subItem.meta( Task::IItem::Metadata::Title ),
                            subItem.linkExtra() );
    }

    return Status::Success;
}

void MetadataAnalyzer::addPlaylistElement( IItem& item,
                                           int64_t playlistId,
                                           const std::string& mrl,
                                           const std::string& itemTitle,
                                           int64_t itemIdx ) const
{
    const auto& playlistMrl = item.mrl();
    LOG_DEBUG( "Trying to add ", mrl, " to the playlist ", playlistMrl,
               " at index ", itemIdx );

    auto t = m_ml->getConn()->newTransaction();

    std::shared_ptr<Media> media;
    auto file = File::fromMrl( m_ml, mrl );
    if ( file == nullptr )
    {
        file = File::fromExternalMrl( m_ml, mrl );
        if ( file == nullptr )
        {
            /* Temporarily create an external media to represent that playlist item
             * If the media ends up being discovered, it will be converted to
             *  an internal one
             */
            media = Media::createExternal( m_ml, mrl, item.duration() );
            if ( media == nullptr )
                return;
            auto files = media->files();
            auto mainFileIt = std::find_if( cbegin( files ), cend( files ),
                                          []( const std::shared_ptr<IFile>& f ) {
                return f->isMain();
            });
            if ( mainFileIt == cend( files ) )
            {
                assert( !"A main file was expected but not found" );
                return;
            }
            file = std::static_pointer_cast<File>( *mainFileIt );
            if ( itemTitle.empty() == false )
                media->setTitle( itemTitle, false );
        }
    }
    try
    {
        auto task = Task::createLinkTask( m_ml, mrl, playlistId,
                                          Task::LinkType::Playlist,
                                          itemIdx );
        if ( task == nullptr )
            return;
        auto parser = m_ml->getParser();
        if ( parser != nullptr )
            parser->parse( std::move( task ) );
    }
    catch ( const sqlite::errors::ConstraintUnique& ex )
    {
        // A duplicated file shouldn't happen as we're in a transaction, meaning
        // other threads can't write to the database. However, we might be trying
        // to insert a duplicated link task
        LOG_INFO( "Failed to insert duplicated link task: ", ex.what(),
                  ". Ignoring" );
        return;
    }

    if ( t != nullptr )
        t->commit();
}

void MetadataAnalyzer::addFolderToPlaylist( IItem& item,
                                            std::shared_ptr<Playlist> playlistPtr,
                                            const IItem& subitem ) const
{
    const auto& mrl = subitem.mrl();
    LOG_DEBUG( "Adding folder ", mrl, " to playlist ", playlistPtr->mrl() );
    auto fsFactory = m_ml->fsFactoryForMrl( mrl );
    std::stack<std::shared_ptr<fs::IDirectory>> directories;
    int64_t itemIdx = 0;

    try
    {
        directories.push( fsFactory->createDirectory( mrl ) );
    }
    catch ( const std::exception& ex )
    {
        return;
    }
    while ( directories.empty() == false )
    {
        auto dir = directories.top();
        directories.pop();
        std::vector<std::shared_ptr<fs::IDirectory>> subDirs;
        std::vector<std::shared_ptr<fs::IFile>> subFiles;
        try
        {
            subDirs = dir->dirs();
            subFiles = dir->files();
        }
        catch ( const std::exception& ex )
        {
            continue;
        }

        for ( auto& d : subDirs )
            directories.push( std::move( d ) );
        auto t = m_ml->getConn()->newTransaction();
        for ( auto& f : subFiles )
        {
            addPlaylistElement( item, playlistPtr->id(), f->mrl(), {}, itemIdx++ );
        }
        t->commit();
    }
}

/* Subscriptions */

Status MetadataAnalyzer::parseSubscription( IItem& item ) const
{
    const auto& mrl = item.mrl();
    LOG_DEBUG( "Importing ", mrl, " as a subscription" );
    std::shared_ptr<Subscription> subscription;
    if ( item.file() != nullptr )
    {
        /*
         * If the file exists in DB, we already inserted this subscription and are
         * most likely refreshing it
         */
        subscription = Subscription::fromFile( m_ml, item.file()->id() );
        if ( subscription == nullptr )
        {
            assert( false );
            return Status::Fatal;
        }
    }
    else
    {
        auto name = item.meta( IItem::Metadata::Title );
        auto artwork_mrl = item.meta( IItem::Metadata::ArtworkUrl );
        if ( name.empty() == true )
            name = utils::file::fileName( mrl );
        auto t = m_ml->getConn()->newTransaction();
        subscription = Subscription::create( m_ml, static_cast<IService::Type>( item.linkToId() ),
                                             std::move( name ), std::move( artwork_mrl ), 0 );
        if ( subscription == nullptr )
            return Status::Fatal;
        auto file = File::createFromSubscription( m_ml, mrl, subscription->id() );
        if ( file == nullptr )
            return Status::Fatal;
        if ( item.setFile( file ) == false )
            return Status::Fatal;
        t->commit();
    }
    for ( auto i = 0u; i < item.nbLinkedItems(); ++i )
    {
        if ( m_stopped.load() == true )
            break;
        const auto& subItem = item.linkedItem( i );
        addSubscriptionElement( subItem, subscription->id(), subItem.mrl(),
                              subItem.meta( IItem::Metadata::Title ) );
    }
    return Status::Success;
}

void MetadataAnalyzer::addSubscriptionElement( const IItem& item, int64_t subscriptionId,
                                               std::string mrl, std::string title ) const
{
    auto t = m_ml->getConn()->newTransaction();
    auto file = File::fromExternalMrl( m_ml, mrl );
    if ( file == nullptr )
    {
        auto media = Media::createExternal( m_ml, mrl, item.duration() );
        if ( media == nullptr )
            return;
        if ( title.empty() == false )
            media->setTitle( std::move( title ), false );
        auto desc = item.meta( IItem::Metadata::Description );
        if ( desc.empty() == false )
            media->setDescription( std::move( desc ) );
        auto date = item.meta( IItem::Metadata::Date );
        if ( date.empty() == false )
        {
            struct tm tm;
            if ( utils::date::fromStr( date, &tm ) == true )
            {
                auto ts = utils::date::mktime( &tm );
                if ( ts != static_cast<time_t>( -1 ) )
                    media->setReleaseDate( ts );
            }
        }
        auto files = media->files();
        auto mainFileIt = std::find_if( cbegin( files ), cend( files ),
                                      []( const std::shared_ptr<IFile>& f ) {
            return f->isMain();
        });
        if ( mainFileIt == cend( files ) )
        {
            assert( !"A main file was expected but not found" );
            return;
        }
        file = std::static_pointer_cast<File>( *mainFileIt );
        if ( file == nullptr )
            return;
    }
    try
    {
        auto task = Task::createLinkTask( m_ml, std::move( mrl ), subscriptionId,
                                          parser::IItem::LinkType::Subscription, 0 );
        auto parser = m_ml->getParser();
        t->commit();
        if ( parser == nullptr )
            return;
        parser->parse( std::move( task ) );
    }
    catch ( const sqlite::errors::ConstraintUnique& ex )
    {
        LOG_DEBUG( "Failed to insert link task: ", ex.what() );
        return;
    }
}

/* Video files */

bool MetadataAnalyzer::parseVideoFile( IItem& item ) const
{
    auto media = static_cast<Media*>( item.media().get() );
    // Even though the title is usually the same as the filename when dealing
    // with a video file, we might be refreshing that media, and it might already
    // have a title, so let's analyse the filename again instead.
    auto title = utils::title::sanitize( item.media()->fileName() );
    auto showInfo = utils::title::analyze( title );

    const auto& embeddedThumbnails = item.embeddedThumbnails();
    std::shared_ptr<Thumbnail> thumbnail;
    if ( embeddedThumbnails.empty() == false )
    {
        thumbnail = std::make_shared<Thumbnail>( m_ml, embeddedThumbnails[0],
                ThumbnailSizeType::Thumbnail );
        auto fileSize = embeddedThumbnails[0]->size();
        thumbnail->setHash( embeddedThumbnails[0]->hash(), fileSize );
    }

    auto t = m_ml->getConn()->newTransaction();
    media->setTitle( title, false );

    if ( media->groupId() == 0 )
    {
        assignMediaToGroup( item );
    }
    if ( thumbnail != nullptr )
    {
        if ( media->setThumbnail( std::move( thumbnail ) ) == false )
            return false;
    }
    if ( std::get<0>( showInfo ) == true )
    {
        auto seasonId = std::get<1>( showInfo );
        auto episodeId = std::get<2>( showInfo );
        auto showName = std::get<3>( showInfo );
        auto episodeTitle = std::get<4>( showInfo );

        auto show = findShow( showName );
        if ( show == nullptr )
        {
            show = m_ml->createShow( std::move( showName ) );
            if ( show == nullptr )
                return false;
        }
        show->addEpisode( *media, seasonId, episodeId, std::move( episodeTitle ) );
    }
    else
    {
        // How do we know if it's a movie or a random video?
    }
    t->commit();
    return true;
}

IMedia::Type MetadataAnalyzer::guessMediaType( const IItem &item ) const
{
    auto ext = utils::file::extension( item.mrl() );
    std::transform(cbegin( ext ), cend( ext ), begin( ext ),
                   [](unsigned char c){ return tolower(c); } );
    if ( ext == "vob" || ext == "ts" || ext == "ogv" || ext == "mpg" ||
         ext == "ogm" || ext == "m2ts" || ext == "m2v" || ext == "mpeg" ||
         ext == "xvid" || ext == "ogx" )
        return IMedia::Type::Video;

    if ( ext == "oga" || ext == "flac" || ext == "opus" || ext == "spx" )
        return IMedia::Type::Audio;

    if ( ext == "ogg" )
    {
        if ( item.fileFs()->size() > 15 * 1024 * 1024 )
            return IMedia::Type::Video;
        return IMedia::Type::Audio;
    }

    return IMedia::Type::Unknown;
}

Status MetadataAnalyzer::createFileAndMedia( IItem& item ) const
{
    assert( item.media() == nullptr );
    // Try to create Media & File
    auto mrl = item.mrl();
    const auto& tracks = item.tracks();

    IMedia::Type mediaType;
    if ( tracks.empty() == false )
    {
        if ( std::find_if( begin( tracks ), end( tracks ), [](const IItem::Track& t) {
            return t.type == IItem::Track::Type::Video;
        }) == end( tracks ) )
        {
            mediaType = IMedia::Type::Audio;
        }
        else
            mediaType = IMedia::Type::Video;
    }
    else
        mediaType = guessMediaType( item );
    auto t = m_ml->getConn()->newTransaction();
    auto file = File::fromExternalMrl( m_ml, mrl );
    // Check if this media was already added before as an external media
    if ( file != nullptr && file->type() == IFile::Type::Main )
    {
        auto media = file->media();
        if ( media == nullptr )
        {
            assert( !"External file must have an associated media" );
            return Status::Fatal;
        }
        if ( media->isExternalMedia() == false )
        {
            assert( !"An external main file must be associated with an external media" );
            return Status::Fatal;
        }
        auto res = overrideExternalMedia( item, *media, *file, mediaType );
        if ( res != Status::Success )
            return res;
        // IItem::setFile will update the task in db, so run it as part of
        // the transaction
        item.setFile( std::move( file ) );
        t->commit();
        item.setMedia( std::move( media ) );
        return res;
    }
    LOG_DEBUG( "Adding ", mrl );
    std::shared_ptr<Media> m;
    try
    {
        auto folder = static_cast<Folder*>( item.parentFolder().get() );
        m = Media::create( m_ml, mediaType, folder->deviceId(), folder->id(),
                                utils::url::decode( utils::file::fileName( mrl ) ),
                                item.duration() );
    }
    catch ( const sqlite::errors::ConstraintForeignKey& ex )
    {
        LOG_INFO( "Failed to add a media: ", ex.what(), ". Assuming the "
                  "containing folder got removed concurrently" );
        return Status::Discarded;
    }
    if ( m == nullptr )
    {
        LOG_ERROR( "Failed to add media ", mrl, " to the media library" );
        return Status::Fatal;
    }
    auto deviceFs = item.parentFolderFs()->device();
    if ( deviceFs == nullptr )
        throw fs::errors::DeviceRemoved{};
    // For now, assume all media are made of a single file
    try
    {
        file = m->addFile( *item.fileFs(), item.parentFolder()->id(),
                           deviceFs->isRemovable(), File::Type::Main );
        if ( file == nullptr )
        {
            LOG_ERROR( "Failed to add file ", mrl, " to media #", m->id() );
            return Status::Fatal;
        }
    }
    catch ( const sqlite::errors::ConstraintUnique& ex )
    {
        /*
         * This can happen if:
         * - We end up with two logically identical MRLs. The
         *   string representations differ, but they point to the same actual file.
         *   When inserting the File in DB, we account for multiple mountpoints and
         *   will end up adding a file with the same folder_id & mrl, which would
         *   be a UNIQUE constraint violation.
         * - A long running discovery task gets interrupted and a new task
         *   triggers a task refresh. Until #103 is fixed, we might re-run a
         *   task that completed properly, which might case a constraint
         *   violation later on (See #424)
         * If that happens, just discard the 2nd task.
         */
        LOG_INFO( "Failed to insert File in db: ", ex.what(), ". Assuming the"
                  " mrl is duplicated" );
        return Status::Discarded;
    }

    createTracks( *m, tracks );

    item.setMedia( std::move( m ) );
    item.setFile( std::move( file ) );

    t->commit();
    m_notifier->notifyMediaCreation( item.media() );
    return Status::Success;
}

Status MetadataAnalyzer::overrideExternalMedia( IItem& item, Media& media,
                                                File& file,
                                                IMedia::Type newType ) const
{
    LOG_DEBUG( "Converting media ", item.mrl(), " from external to internal" );
    // If the file is on a removable device, we need to update its mrl
    assert( sqlite::Transaction::isInProgress() == true );
    auto fsDir = item.parentFolderFs();
    auto deviceFs = fsDir->device();
    if ( deviceFs == nullptr )
        return Status::Fatal;
    auto device = Device::fromUuid( m_ml, deviceFs->uuid(), deviceFs->scheme() );
    if ( device == nullptr )
        return Status::Fatal;
    if ( file.update( *item.fileFs(), item.parentFolder()->id(),
                       deviceFs->isRemovable() ) == false )
        return Status::Fatal;
    auto updatedTitle = item.meta( Task::IItem::Metadata::Title );
    if ( updatedTitle.empty() == false )
        media.setTitle( updatedTitle, false );
    if ( media.markAsInternal( newType, item.duration(), device->id(),
                          item.parentFolder()->id() ) == false )
        return Status::Fatal;
    return Status::Success;
}

void MetadataAnalyzer::createTracks( Media& m, const std::vector<IItem::Track>& tracks ) const
{
    assert( sqlite::Transaction::isInProgress() == true );
    for ( const auto& track : tracks )
    {
        if ( track.type == IItem::Track::Type::Video )
        {
            m.addVideoTrack( track.codec, track.u.v.width, track.u.v.height,
                                  track.u.v.fpsNum, track.u.v.fpsDen, track.bitrate,
                                  track.u.v.sarNum, track.u.v.sarDen,
                                  track.language, track.description );
        }
        else if ( track.type == IItem::Track::Type::Audio )
        {
            m.addAudioTrack( track.codec, track.bitrate,
                                       track.u.a.rate, track.u.a.nbChannels,
                                       track.language, track.description, 0 );
        }
        else
        {
            assert( track.type == IItem::Track::Type::Subtitle );
            m.addSubtitleTrack( track.codec, track.language, track.description,
                                track.u.s.encoding, 0 );
        }
    }
}

std::tuple<bool, bool> MetadataAnalyzer::refreshFile( IItem& item ) const
{
    auto file = item.file();

    if ( file == nullptr )
    {
        assert( !"Can't refresh a non-existing file" );
        return std::make_tuple( false, false );
    }

    switch ( file->type() )
    {
        case IFile::Type::Main:
            return refreshMedia( item );
        case IFile::Type::Playlist:
            return refreshPlaylist( item );
        case IFile::Type::Subscription:
            return refreshSubscription( item );
        case IFile::Type::Part:
        case IFile::Type::Soundtrack:
        case IFile::Type::Subtitles:
        case IFile::Type::Disc:
        case IFile::Type::Unknown:
        case IFile::Type::Cache:
            break;
    }
    LOG_WARN( "Refreshing of file type ",
              static_cast<std::underlying_type<IFile::Type>::type>( file->type() ),
              " is unsupported" );
    return std::make_tuple( false, false );
}

std::tuple<bool, bool> MetadataAnalyzer::refreshMedia( IItem& item ) const
{
    auto file = std::static_pointer_cast<File>( item.file() );
    // If we restored this task, we already know the media. Otherwise, it's not
    // loaded in the item yet.
    auto media = std::static_pointer_cast<Media>( item.media() );
    if ( media == nullptr )
        media = file->media();

    if ( media == nullptr )
    {
        assert( !"Can't refresh a non-existing media" );
        return std::make_tuple( false, false );
    }

    if ( media->duration() != item.duration() )
        media->setDuration( item.duration() );

    auto tracks = item.tracks();
    auto isAudio = std::find_if( begin( tracks ), end( tracks ), [](const IItem::Track& t) {
        return t.type == IItem::Track::Type::Video;
    }) == end( tracks );

    if ( isAudio == false )
    {
        auto newTitle = utils::title::sanitize( media->fileName() );
        media->setTitle( newTitle, false );
    }

    auto t = m_ml->getConn()->newTransaction();

    /*
     * Media::setType already checks that the assigned type is different from
     * the current one so we can just call it blindly from here
     */
    if ( media->setTypeInternal( isAudio == true ? IMedia::Type::Audio :
                         IMedia::Type::Video ) == false ||
         VideoTrack::removeFromMedia( m_ml, media->id() ) == false ||
         AudioTrack::removeFromMedia( m_ml, media->id(), true ) == false ||
         SubtitleTrack::removeFromMedia( m_ml, media->id(), true ) == false )
    {
        return std::make_tuple( false, false );
    }
    try
    {
        createTracks( *media, tracks );
    }
    catch ( const sqlite::errors::ConstraintForeignKey& )
    {
        /* We're aiming at catching an error during the insertion of the
         * tracks, which have a foreign key pointing to Media.id_media
         * If the media was removed by the Discoverer thread, the insertion
         * will fail, and we need to abort & discard this task, and the
         * file/media don't exist anymore
         */
        LOG_INFO( "Failed to add tracks to a media. Assuming it was "
                  "concurrently deleted" );
        return std::make_tuple( false, false );
    }
    bool needRescan = false;
    if ( media->subType() != IMedia::SubType::Unknown )
    {
        needRescan = true;
        switch( media->subType() )
        {
            case IMedia::SubType::AlbumTrack:
            {
                auto album = std::static_pointer_cast<Album>( media->album() );
                if ( album == nullptr )
                {
                    LOG_ERROR( "Can't fetch album associated to album track ",
                               media->id(), "(media ", media->id(), ")" );
                    assert( false );
                    return std::make_tuple( false, false );
                }

                // In case we don't have a thumbnail for the updated media
                // but had a thumbnail in the past, we don't want to lose it.
                // In case the file changed completely but has the same mrl
                // this will lead to a wrong thumbnail being used, but this
                // case is deemed unlikely enough to care.
                if ( item.meta( IItem::Metadata::ArtworkUrl ).empty() == false )
                {
                    for ( auto i = 0u; i < Thumbnail::SizeToInt( ThumbnailSizeType::Count ); ++i )
                        media->removeThumbnail( static_cast<ThumbnailSizeType>( i ) );
                }

                album->removeTrack( *media );
                if ( Artist::dropMediaArtistRelation( m_ml, media->id() ) == false )
                    return std::make_tuple( false, false );
                break;
            }
            case IMedia::SubType::Movie:
            {
                auto movie = media->movie();
                if ( movie == nullptr )
                {
                    LOG_ERROR( "Failed to fetch movie associated with media ", media->id() );
                    assert( false );
                    return std::make_tuple( false, false );
                }
                Movie::destroy( m_ml, movie->id() );
                if ( media->setSubTypeUnknown() == false )
                    return std::make_tuple( false, false );
                break;
            }
            case IMedia::SubType::ShowEpisode:
            {
                auto episode = std::static_pointer_cast<ShowEpisode>( media->showEpisode() );
                if ( episode == nullptr )
                {
                    LOG_ERROR( "Failed to fetch show episode associated with media ", media->id() );
                    assert( false );
                    return std::make_tuple( false, false );
                }
                ShowEpisode::destroy( m_ml, episode->id() );
                if ( media->setSubTypeUnknown() == false )
                    return std::make_tuple( false, false );
                break;
            }
            case IMedia::SubType::Unknown:
                assert( !"Unreachable" );
                return std::make_tuple( false, false );
        }
    }

    t->commit();
    item.setMedia( std::move( media ) );
    return std::make_tuple( true, needRescan );
}

std::tuple<bool, bool> MetadataAnalyzer::refreshPlaylist(IItem& item) const
{
    auto playlist = Playlist::fromFile( m_ml, item.file()->id() );
    if ( playlist == nullptr )
    {
        LOG_WARN( "Failed to find playlist associated to modified playlist file ",
                  item.mrl() );
        return std::make_tuple( false, false );
    }
    LOG_DEBUG( "Reloading playlist ", playlist->name(), " on ", item.mrl() );
    /*
     * We need to remove all the existing tasks associated with this playlist.
     * When scanning a playlist, all its content gets analyzed by the path
     * crawler, and ultimately triggers MediaLibrary::onDiscoveredFile.
     * Not removing the tasks would lead to a constraint violation (since
     * the playlist has been ingested before), and while the file would
     * exist in database, it wouldn't be added back to the playlist.
     * We can't simply force all existing tasks to be re-run, as some of
     * the previous playlist content might have been removed.
     * Should the media be imported as non external, they will be refreshed
     * if they need to since the FsDiscoverer will scan them again.
     */
    auto t = m_ml->getConn()->newTransaction();
    if ( parser::Task::removePlaylistContentTasks( m_ml, playlist->id() ) == false ||
         playlist->clearContent() == false )
        return std::make_tuple( false, false );
    t->commit();
    return std::make_tuple( true, true );
}

std::tuple<bool, bool> MetadataAnalyzer::refreshSubscription( IItem& item ) const
{
    auto subscription = Subscription::fromFile( m_ml, item.file()->id() );
    if ( subscription == nullptr )
    {
        LOG_WARN( "Failed to find a subscription associated to MRL ", item.mrl() );
        return std::make_tuple( false, false );
    }
    auto t = m_ml->getConn()->newTransaction();
    if ( parser::Task::removeSubscriptionContentTasks( m_ml, subscription->id() ) == false ||
         subscription->clearContent() == false )
        return std::make_tuple( false, false );
    t->commit();
    return std::make_tuple( true, true );
}

/* Audio files */

Status MetadataAnalyzer::parseAudioFile( IItem& item, Cache& cache )
{
    auto genre = handleGenre( item );
    auto artists = findOrCreateArtist( item );
    if ( artists.first == nullptr && artists.second == nullptr )
        return Status::Fatal;
    auto albumName = item.meta( IItem::Metadata::Album );

    std::shared_ptr<Album> album;
    if ( albumName.empty() == false )
        album = findAlbum( item, albumName, artists.first.get(), artists.second.get(), cache );
    else
        album = handleUnknownAlbum( artists.first.get(), artists.second.get() );
    auto newAlbum = album == nullptr;

    /*
     * Check for a cover file out of the transaction/retry scope
     * We only check for album artwork when the album is about to be created
     * If a thumbnail needs to be inserted in database, this will be done later
     * in the transaction scope
     */
    auto thumbnail = fetchThumbnail( item, album.get() );

    auto t = m_ml->getConn()->newTransaction();

    if ( album == nullptr )
    {
        if ( albumName.empty() == false )
            album = m_ml->createAlbum( std::move( albumName ) );
        else
            album = createUnknownAlbum( artists.first.get(), artists.second.get() );
        if ( album == nullptr )
            return Status::Fatal;
    }

    try
    {
        // If we know a track artist, specify it, otherwise, fallback to the album/unknown artist
        if ( handleTrack( *album, item,
                          artists.second ? artists.second->id() : artists.first->id(),
                          genre.get() ) == nullptr )
            return Status::Fatal;
    }
    catch ( const sqlite::errors::ConstraintForeignKey& ex )
    {
        LOG_INFO( "Failed to insert album track: ", ex.what(), ". "
                  "Assuming the media was deleted concurrently" );
        return Status::Fatal;
    }
    catch ( const sqlite::errors::ConstraintUnique& ex )
    {
        // If the application crashed before the parser bookkeeping completed
        // we might try to reprocess this task while it's finished.
        // If the AlbumTrack was already inserted, it means that we already
        // committed the current transaction, so there's nothing more
        // for this task to do.
        //
        // See https://code.videolan.org/videolan/medialibrary/issues/103
        LOG_INFO( "Failed to insert album track: ", ex.what(), ". "
                  "Assuming the task was already processed." );
        return Status::Completed;
    }

    link( item, *album, std::move( artists.first ), std::move( artists.second ),
          newAlbum, thumbnail );
    t->commit();

    if ( newAlbum == true )
        m_notifier->notifyAlbumCreation( std::move( album ) );

    if ( thumbnail != nullptr && thumbnail->id() != 0 &&
         thumbnail->isOwned() == false )
    {
        assert( thumbnail->status() == ThumbnailStatus::Available );
        thumbnail->relocate();
    }
    return Status::Success;
}

std::shared_ptr<Genre> MetadataAnalyzer::handleGenre( IItem& item ) const
{
    auto genreStr = item.meta( IItem::Metadata::Genre );
    if ( genreStr.length() == 0 )
        return nullptr;
    auto genre = Genre::fromName( m_ml, genreStr );
    if ( genre == nullptr )
    {
        genre = Genre::create( m_ml, std::move( genreStr ) );
        if ( genre == nullptr )
        {
            LOG_ERROR( "Failed to get/create Genre" );
            return nullptr;
        }
        m_notifier->notifyGenreCreation( genre );
    }
    return genre;
}

std::shared_ptr<Thumbnail> MetadataAnalyzer::findAlbumArtwork( IItem& item )
{
    static const std::string validExtensions[] = { "jpeg", "jpg", "png" };
    auto fsDir = item.parentFolderFs();
    auto files = fsDir->files();
    files.erase( std::remove_if( begin( files ), end( files ),
                                 []( const std::shared_ptr<fs::IFile>& fileFs ) {
        auto ext = utils::file::extension( fileFs->name() );
        return std::find( cbegin( validExtensions ), cend( validExtensions ),
                          ext ) == cend( validExtensions );
    }), end( files ) );
    if ( files.empty() == true )
        return nullptr;
    if ( files.size() > 1 )
        LOG_INFO( "More than one album thumbnail candidate: we need a better "
                  "discrimination logic." );
    auto fileFs = files[0];
    return std::make_shared<Thumbnail>( m_ml, fileFs->mrl(),
                                        Thumbnail::Origin::CoverFile,
                                        ThumbnailSizeType::Thumbnail, false );
}

std::shared_ptr<Show> MetadataAnalyzer::findShow( const std::string& showName ) const
{
    if ( showName.empty() == true )
        return m_unknownShow;

    const std::string req = "SELECT * FROM " + Show::Table::Name +
            " WHERE title = ?";

    auto shows = Show::fetchAll<Show>( m_ml, req, showName );
    if ( shows.empty() == true )
        return nullptr;
    //FIXME: Discriminate amongst shows
    return shows[0];
}

bool MetadataAnalyzer::assignMediaToGroup( IItem &item ) const
{
    assert( item.media() != nullptr );
    auto m = static_cast<Media*>( item.media().get() );
    if ( m->type() != IMedia::Type::Video )
        return true;
    return MediaGroup::assignToGroup( m_ml, *m );
}

/* Album handling */

std::shared_ptr<Album> MetadataAnalyzer::findAlbum( IItem& item,
                                                    const std::string& albumName,
                                                    Artist* albumArtist,
                                                    Artist* trackArtist,
                                                    Cache& cache )
{
    assert( albumName.empty() == false );
    auto file = static_cast<File*>( item.file().get() );
    if ( cache.previousAlbum != nullptr &&
         albumName == cache.previousAlbum->title() &&
         cache.previousFolderId != 0 &&
         cache.previousFolderId == file->folderId() )
    {
        return cache.previousAlbum;
    }
    cache.previousAlbum.reset();
    cache.previousFolderId = 0;

    // Album matching depends on the difference between artist & album artist.
    // Specifically pass the albumArtist here.
    static const std::string req = "SELECT * FROM " + Album::Table::Name +
            " WHERE title = ?";
    auto albums = Album::fetchAll<Album>( m_ml, req, albumName );

    if ( albums.empty() == true )
        return nullptr;

    const auto discTotal = toInt( item, IItem::Metadata::DiscTotal );
    const auto discNumber = toInt( item, IItem::Metadata::DiscNumber );
    /*
     * Even if we get only 1 album, we need to filter out invalid matches.
     * For instance, if we have already inserted an album "A" by an artist "john"
     * but we are now trying to handle an album "A" by an artist "doe", not filtering
     * candidates would yield the only "A" album we know, while we should return
     * nullptr, so the link() method can create a new one.
     */
    for ( auto it = begin( albums ); it != end( albums ); )
    {
        auto a = (*it).get();
        auto candidateAlbumArtist = a->albumArtist();
        // When we find an album, we will systematically assign an artist to it.
        // Not having an album artist (even it it's only a temporary one in the
        // case of a compilation album) is not expected at all.
        assert( candidateAlbumArtist != nullptr );
        if ( albumArtist != nullptr )
        {
            // We assume that an album without album artist is a positive match.
            // At the end of the day, without proper tags, there's only so much we can do.
            if ( candidateAlbumArtist->id() != albumArtist->id() )
            {
                it = albums.erase( it );
                continue;
            }
        }
        // If this is a multidisc album, assume it could be in a multiple amount of folders.
        // Since folders can come in any order, we can't assume the first album will be the
        // first media we see. If the discTotal or discNumber meta are provided, that's easy. If not,
        // we assume that another CD with the same name & artists, and a disc number > 1
        // denotes a multi disc album
        // Check the first case early to avoid fetching tracks if unrequired.
        if ( discTotal > 1 || discNumber > 1 )
        {
            ++it;
            continue;
        }
        const auto tracks = a->cachedTracks();
        // If there is no tracks to compare with, we just have to hope this will be the only valid
        // album match
        if ( tracks.empty() == true )
        {
            ++it;
            continue;
        }

        auto multiDisc = false;
        auto multipleArtists = false;
        int64_t previousArtistId = trackArtist != nullptr ? trackArtist->id() : 0;
        for ( auto& t : tracks )
        {
            if ( t->discNumber() > 1 )
                multiDisc = true;
            if ( previousArtistId != 0 && previousArtistId != t->artistId() )
                multipleArtists = true;
            previousArtistId = t->artistId();
            // We now know enough about the album, we can stop looking at its tracks
            if ( multiDisc == true && multipleArtists == true )
                break;
        }
        if ( multiDisc )
        {
            ++it;
            continue;
        }

        // Assume album files will be in the same folder.
        auto newFileFolder = utils::file::directory( file->mrl() );
        auto trackFiles = tracks[0]->files();
        bool differentFolder = false;
        for ( const auto& existingTrackIFile : trackFiles )
        {
            const auto existingTrackFile = static_cast<File*>( existingTrackIFile.get() );
            if ( existingTrackFile->folderId() != file->folderId() )
            {
                differentFolder = true;
                break;
            }
        }
        // We now have a candidate by the same artist in the same folder, assume it to be
        // a positive match.
        if ( differentFolder == false )
        {
            ++it;
            continue;
        }

        // Attempt to discriminate by date, but only for the same artists.
        // Not taking the artist in consideration would cause compilation to
        // create multiple albums, especially when track are only partially
        // tagged with a year.
        if ( multipleArtists == false )
        {
            auto candidateDate = item.meta( IItem::Metadata::Date );
            if ( candidateDate.empty() == false )
            {
                try
                {
                    unsigned int year = std::stoi( candidateDate );
                    if ( year != a->releaseYear() )
                        it = albums.erase( it );
                    else
                        ++it;
                    continue;
                }
                catch (...)
                {
                    // Date wasn't helpful, simply ignore the error and continue
                }
            }
        }
        // The candidate is :
        // - in a different folder
        // - not a multidisc album
        // - Either:
        //      - from the same artist & without a date to discriminate
        //      - from the same artist & with a different date
        //      - from different artists
        // Assume it's a negative match.
        it = albums.erase( it );
    }
    if ( albums.empty() == true )
        return nullptr;
    if ( albums.size() > 1 )
    {
        LOG_WARN( "Multiple candidates for album ", albumName, ". Selecting first one out of luck" );
    }
    cache.previousFolderId = file->folderId();
    cache.previousAlbum = albums[0];
    return albums[0];
}

std::shared_ptr<Album>
MetadataAnalyzer::handleUnknownAlbum( Artist* albumArtist, Artist* trackArtist )
{
    if ( albumArtist != nullptr )
        return albumArtist->unknownAlbum();
    if ( trackArtist != nullptr )
        return trackArtist->unknownAlbum();
    return m_unknownArtist->unknownAlbum();
}

std::shared_ptr<Album> MetadataAnalyzer::createUnknownAlbum(Artist* albumArtist, Artist* trackArtist)
{
    if ( albumArtist != nullptr )
        return albumArtist->createUnknownAlbum();
    if ( trackArtist != nullptr )
        return trackArtist->createUnknownAlbum();
    return m_unknownArtist->createUnknownAlbum();
}

///
/// \brief MetadataParser::handleArtists Returns Artist's involved on a track
/// \param task The current parser task
/// \return A pair containing:
/// The album artist as a first element
/// The track artist as a second element, or nullptr if it is the same as album artist
///
std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> MetadataAnalyzer::findOrCreateArtist( IItem& item ) const
{
    std::shared_ptr<Artist> albumArtist;
    std::shared_ptr<Artist> artist;
    static const std::string req = "SELECT * FROM " + Artist::Table::Name + " WHERE name = ?";

    auto albumArtistStr = item.meta( IItem::Metadata::AlbumArtist );
    auto artistStr = item.meta( IItem::Metadata::Artist );
    if ( albumArtistStr.empty() == true && artistStr.empty() == true )
    {
        return {m_unknownArtist, m_unknownArtist};
    }

    bool isDifferent = albumArtistStr != artistStr;
    if ( albumArtistStr.empty() == false )
    {
        albumArtist = Artist::fetch( m_ml, req, albumArtistStr );
        if ( albumArtist == nullptr )
        {
            albumArtist = m_ml->createArtist( std::move( albumArtistStr ) );
            if ( albumArtist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist" );
                return {nullptr, nullptr};
            }
            m_notifier->notifyArtistCreation( albumArtist );
        }
    }
    if ( artistStr.empty() == false && isDifferent )
    {
        artist = Artist::fetch( m_ml, req, artistStr );
        if ( artist == nullptr )
        {
            artist = m_ml->createArtist( std::move( artistStr ) );
            if ( artist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist" );
                return {nullptr, nullptr};
            }
            m_notifier->notifyArtistCreation( artist );
        }
    }
    return {albumArtist, artist};
}

/* Tracks handling */

std::shared_ptr<Media> MetadataAnalyzer::handleTrack( Album& album, IItem& item,
                                                      int64_t artistId, Genre* genre ) const
{
    assert( sqlite::Transaction::isInProgress() == true );

    auto title = item.meta( IItem::Metadata::Title );
    const auto trackNumber = toInt( item, IItem::Metadata::TrackNumber );
    const auto discNumber = toInt( item, IItem::Metadata::DiscNumber );
    auto media = std::static_pointer_cast<Media>( item.media() );
    if ( title.empty() == true )
    {
        if ( trackNumber != 0 )
        {
            title = "Track #";
            title += std::to_string( trackNumber );
        }
    }
    if ( title.empty() == false )
        media->setTitle( title, false );

    if ( album.addTrack( media, trackNumber, discNumber, artistId,
                         genre ) == false )
    {
        LOG_ERROR( "Failed to create album track" );
        return nullptr;
    }

    const auto& releaseDateStr = item.meta( IItem::Metadata::Date );
    if ( releaseDateStr.empty() == false )
    {
        time_t releaseDate = atoi( releaseDateStr.c_str() );
        unsigned int releaseYear;
        if ( releaseDate > 0 && (unsigned int)releaseDate < TimestampThreshold &&
             releaseDate >= 1900 )
        {
            struct tm t{};
            t.tm_year = releaseDate - 1900;
            t.tm_mday = 1;
            releaseYear = releaseDate;
            releaseDate = utils::date::mktime( &t );
        }
        else if ( releaseDate != 0 )
        {
            struct tm t{};
            gmtime_r(&releaseDate, &t);
            releaseYear = t.tm_year + 1900;
        }
        else
            releaseYear = 0;
        if ( media->setReleaseDate( releaseDate ) == false )
            return nullptr;
        // Let the album handle multiple dates. In order to do this properly, we need
        // to know if the date has been changed before, which can be known only by
        // using Album class internals.
        album.setReleaseYear( releaseYear, false );
    }
    return media;
}

/* Misc */

void MetadataAnalyzer::link( IItem& item, Album& album,
                             std::shared_ptr<Artist> albumArtist,
                             std::shared_ptr<Artist> artist, bool newAlbum,
                             std::shared_ptr<Thumbnail> mediaThumbnail )
{
    Media& media = static_cast<Media&>( *item.media() );

    if ( albumArtist == nullptr )
    {
        assert( artist != nullptr );
        albumArtist = artist;
    }
    assert( albumArtist != nullptr );

    if ( mediaThumbnail != nullptr )
    {
        assignThumbnails( media, album, *albumArtist, newAlbum,
                          std::move( mediaThumbnail ) );
    }

    albumArtist->addMedia( media );
    if ( artist != nullptr && albumArtist->id() != artist->id() )
        artist->addMedia( media );

    auto currentAlbumArtist = album.albumArtist();

    // If we have no main artist yet, that's easy, we need to assign one.
    if ( currentAlbumArtist == nullptr )
    {
        // We don't know if the artist was tagged as artist or albumartist, however, we simply add it
        // as the albumartist until proven we were wrong (ie. until one of the next tracks
        // has a different artist)
        album.setAlbumArtist( albumArtist );
    }
    else
    {
        // We have more than a single artist on this album, fallback to various artists
        if ( albumArtist->id() != currentAlbumArtist->id() )
        {
            std::lock_guard<compat::Mutex> lock{ m_cacheMutex };
            if ( m_variousArtists == nullptr )
                m_variousArtists = Artist::fetch( m_ml, VariousArtistID );
            // If we already switched to various artist, no need to do it again
            if ( m_variousArtists->id() != currentAlbumArtist->id() )
            {
                // All tracks from this album must now also be reflected in various
                // artist number of tracks
                // We also need to link all existing tracks to Various Artists
                auto currentAlbumTracks = album.tracks( nullptr )->all();
                for ( const auto& track : currentAlbumTracks )
                {
                    m_variousArtists->addMedia( static_cast<Media&>( *track ) );
                }
                album.setAlbumArtist( m_variousArtists );
            }
            // However we always need to bump the various artist number of tracks
            else
            {
                // Link this media with 'Various Artists' so that it gets in
                // Various Artists' tracks listing.
                m_variousArtists->addMedia( media );
            }
        }
    }

    const auto discTotal = toInt( item, IItem::Metadata::DiscTotal );
    const auto discNumber = toInt( item, IItem::Metadata::DiscNumber );
    if ( ( discTotal > 0 &&
           static_cast<uint32_t>( discTotal ) > album.nbDiscs() ) ||
         ( discNumber > 0 &&
           static_cast<uint32_t>( discNumber ) > album.nbDiscs() ) )
    {
        album.setNbDiscs( std::max( discTotal, discNumber ) );
    }
}

std::shared_ptr<Thumbnail> MetadataAnalyzer::fetchThumbnail( IItem& item,
                                                             Album* album )
{
    if ( item.media()->thumbnailStatus( ThumbnailSizeType::Thumbnail ) ==
         ThumbnailStatus::Available )
    {
        /*
         * If this media had a thumbnail because it was analyzed previously we
         * want to reuse it.
         * This can happen if the media was internal, then converted to external,
         * and re-converted to an internal media.
         */
        auto media = static_cast<Media*>( item.media().get() );
        return media->thumbnail( ThumbnailSizeType::Thumbnail );
    }

    /* First, probe the media for an embedded thumbnail */
    const auto& embeddedThumbnails = item.embeddedThumbnails();
    if ( embeddedThumbnails.empty() == false )
    {
        /*
         * If we have an embedded thumbnail, we need to compute its hash in any
         * case. Either we'll be storing it for the first time, or we need to
         * be able to compare it later.
         */
        auto thumbnail = std::make_shared<Thumbnail>( m_ml, embeddedThumbnails[0],
                ThumbnailSizeType::Thumbnail );

        auto fileSize = embeddedThumbnails[0]->size();
        thumbnail->setHash( embeddedThumbnails[0]->hash(), fileSize );
        return thumbnail;
    }
    /*
     * If the album wasn't already created and we have an embedded thumbnail
     * we don't need to verify anything else, just use that embedded thumbnail.
     * If we didn't find an embedded thumbnail, let's check the folder for a
     * cover file.
     */
    if ( album == nullptr )
        return findAlbumArtwork( item );
    return album->thumbnail( ThumbnailSizeType::Thumbnail );
}

void MetadataAnalyzer::assignThumbnails( Media& media, Album& album,
                                         Artist& albumArtist, bool newAlbum,
                                         std::shared_ptr<Thumbnail> thumbnail )
{
    assert( sqlite::Transaction::isInProgress() == true );
    assert( thumbnail != nullptr );
    assert( thumbnail->origin() == Thumbnail::Origin::Media ||
            thumbnail->origin() == Thumbnail::Origin::CoverFile );

    /* We need to assign a thumbnail to the media, album, and artist.
     * The thumbnail we are assigning either comes from an embedded artwork, or
     * a file in the album folder.
     * If it is an embedded file, it might be identical to the one already
     * assigned to the album, in which case we need to share it.
     * There might also be only a single file with embedded artwork in the
     * album, in which case we should override the album thumbnail with the
     * embedded one, as it's more likely to be relevant
     */
    if ( newAlbum == true && album.isUnknownAlbum() == false )
    {
        album.setThumbnail( thumbnail );
        media.setThumbnail( thumbnail );
    }
    else if ( album.isUnknownAlbum() == false )
    {
        assert( newAlbum == false );
        auto albumThumbnail = album.thumbnail( ThumbnailSizeType::Thumbnail );
        /* If the album has no thumbnail, we just need to assign one */
        if ( albumThumbnail == nullptr )
        {
            album.setThumbnail( thumbnail );
            /*
             * Since we just assigned a thumbnail to the album, check if
             * other tracks from this album require a thumbnail
             */
            for ( auto& t : album.cachedTracks() )
            {
                if ( t->thumbnailStatus( ThumbnailSizeType::Thumbnail ) ==
                     ThumbnailStatus::Missing )
                {
                    auto m = static_cast<Media*>( t.get() );
                    m->setThumbnail( thumbnail );
                }
            }
        }
        /*
         * If it already has a thumbnail, we need to check if the new one is a
         * better choice.
         * This can only happen if the thumbnail is embedded, if it comes from
         * a cover file, it was already assigned
         */
        else if ( thumbnail->origin() == Thumbnail::Origin::Media )
        {
            switch ( albumThumbnail->origin() )
            {
                /* If the cover was user provided, we must not override it. */
                case Thumbnail::Origin::UserProvided:
                    media.setThumbnail( thumbnail );
                    break;
                /*
                 * If we assigned the album with a thumbnail found in the album
                 * folder, we can favor the embedded one directly as it's less
                 * error prone.
                 * If we got this cover from the artist, use the newly found
                 * media one since it's most likely to be more relevant.
                 */
                case Thumbnail::Origin::CoverFile:
                case Thumbnail::Origin::AlbumArtist:
                case Thumbnail::Origin::Artist:
                    album.setThumbnail( thumbnail );
                    media.setThumbnail( thumbnail );
                    break;
                case Thumbnail::Origin::Media:
                {
                    if ( albumThumbnail->fileSize() != thumbnail->fileSize() ||
                         albumThumbnail->hash() != thumbnail->hash() )
                    {
                        /* We already had a cover from the album, coming from an
                         * embedded file and the new embedded file differs.
                         * Just keep the previous album cover, and assign the
                         * new embedded cover to the media
                         */
                        media.setThumbnail( thumbnail );
                    }
                    else
                    {
                        /* We have a new embedded cover that is identical to the
                         * new one: just share the previous one.
                         */
                        media.setThumbnail( albumThumbnail );
                    }
                    break;
                }
                default:
                    assert( !"Invalid thumbnail origin" );
            }
        }
        else
        {
            /* The cover comes from the album cover: share the album one */
            media.setThumbnail( albumThumbnail );
        }
    }
    else
    {
        assert( album.isUnknownAlbum() == true );
        /*
         * If the album is unknown and the cover came from the media, assign it
         * otherwise don't assign anything since it can be a large folder with
         * a lot of music, we don't want to use an image file that would lie
         * there while being unrelated to the various tracks in it
         */
        if ( thumbnail->origin() == Thumbnail::Origin::Media )
            media.setThumbnail( thumbnail );
    }

    // We might modify albumArtist later, hence handle thumbnails before.
    // If we have an albumArtist (meaning the track was properly tagged, we
    // can assume this artist is a correct match. We can use the thumbnail from
    // the current album for the albumArtist, if none has been set before.
    // Although we don't want to do this for unknown/various artists, as the
    // thumbnail wouldn't reflect those "special" artists
    if ( albumArtist.id() != UnknownArtistID &&
         albumArtist.id() != VariousArtistID )
    {
        auto albumArtistThumbnail = albumArtist.thumbnail( thumbnail->sizeType() );
        // If the album artist has no thumbnail, let's assign it
        if ( albumArtistThumbnail == nullptr )
        {
            albumArtist.setThumbnail( thumbnail );
        }
        else if ( albumArtistThumbnail->origin() == Thumbnail::Origin::Artist )
        {
            // We only want to change the thumbnail if it was assigned from an
            // album this artist was only featuring on
        }
    }
}

const char* MetadataAnalyzer::name() const
{
    return "Metadata";
}

void MetadataAnalyzer::onFlushing()
{
    std::lock_guard<compat::Mutex> lock{ m_cacheMutex };
    m_variousArtists = nullptr;
    m_previousAlbum = nullptr;
    m_previousFolderId = 0;
    m_flushed = true;
}

void MetadataAnalyzer::onRestarted()
{
    /*
     * Reset locally cached entities
     * If the service hasn't started yet, no need to restart anything
     */
    if ( m_ml == nullptr )
        return;
    cacheUnknownArtist();
    cacheUnknownShow();
    m_stopped.store( false );
}

Step MetadataAnalyzer::targetedStep() const
{
    return Step::MetadataAnalysis;
}

void MetadataAnalyzer::stop()
{
    m_stopped.store( true );
}

}
}
