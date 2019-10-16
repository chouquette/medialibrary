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
#include "AlbumTrack.h"
#include "Artist.h"
#include "File.h"
#include "medialibrary/filesystem/Errors.h"
#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "Folder.h"
#include "Genre.h"
#include "Media.h"
#include "Playlist.h"
#include "Show.h"
#include "ShowEpisode.h"
#include "Movie.h"
#include "utils/Directory.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "utils/ModificationsNotifier.h"
#include "utils/TitleAnalyzer.h"
#include "discoverer/FsDiscoverer.h"
#include "discoverer/probe/PathProbe.h"
#include "Device.h"
#include "VideoTrack.h"
#include "AudioTrack.h"
#include "SubtitleTrack.h"

#include <cstdlib>
#include <algorithm>

namespace medialibrary
{
namespace parser
{

MetadataAnalyzer::MetadataAnalyzer()
    : m_ml( nullptr )
    , m_previousFolderId( 0 )
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

bool MetadataAnalyzer::initialize( IMediaLibrary* ml )
{
    m_ml = static_cast<MediaLibrary*>( ml );
    m_notifier = m_ml->getNotifier();
    return cacheUnknownArtist();
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
        catch( std::logic_error& ex)
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
        file->updateFsInfo( item.fileFs()->lastModificationDate(),
                            item.fileFs()->size() );
        if ( needRescan == false )
            return Status::Success;
    }
    auto nbSubitem = item.nbSubItems();
    // Assume that file containing subitem(s) is a Playlist
    if ( nbSubitem > 0 )
    {
        auto res = parsePlaylist( item );
        if ( res != Status::Success ) // playlist addition may fail due to constraint violation
            return res;

        assert( item.file() != nullptr || item.isRestore() == true );
        return Status::Completed;
    }

    bool isAudio;
    bool isFileModified = false;

    if ( item.file() == nullptr )
    {
        Status status;

        std::tie(status, isAudio) = createFileAndMedia( item );
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
        if ( item.media()->audioTracks()->count() == 0 &&
             item.media()->videoTracks()->count() == 0 &&
             item.media()->subtitleTracks()->count() == 0 )
        {
            try
            {
                auto t = m_ml->getConn()->newTransaction();
                createTracks( static_cast<Media&>( *item.media() ), item.tracks() );
                t->commit();
            }
            catch ( const sqlite::errors::ConstraintForeignKey& ex )
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
        isAudio = item.media()->type() == IMedia::Type::Audio;
    }
    auto media = std::static_pointer_cast<Media>( item.media() );

    if ( isAudio == true )
    {
        if ( parseAudioFile( item ) == false )
            return Status::Fatal;
    }
    else
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
        try
        {
            auto t = m_ml->getConn()->newTransaction();
            playlistPtr = Playlist::create( m_ml, playlistName );
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
                auto file = playlistPtr->addFile( *item.fileFs(),
                                                  item.parentFolder()->id(),
                                                  deviceFs->isRemovable() );
                if ( file == nullptr )
                {
                    LOG_ERROR( "Failed to add playlist file ", mrl );
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
            assert( sqlite::Transaction::transactionInProgress() == false );
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

    for ( auto i = 0u; i < item.nbSubItems(); ++i )
    {
        if ( m_stopped.load() == true )
            break;
        addPlaylistElement( item, playlistPtr, item.subItem( i ) );
    }

    return Status::Success;
}

void MetadataAnalyzer::addPlaylistElement( IItem& item,
                                           std::shared_ptr<Playlist> playlistPtr,
                                           const IItem& subitem ) const
{
    const auto& mrl = subitem.mrl();
    const auto& playlistMrl = item.mrl();
    LOG_DEBUG( "Try to add ", mrl, " to the playlist ", playlistMrl, " at index ", subitem.linkExtra() );
    // Create Media, etc.
    auto fsFactory = m_ml->fsFactoryForMrl( mrl );

    if ( fsFactory == nullptr ) // Media not supported by any FsFactory, registering it as external
    {
        auto t2 = m_ml->getConn()->newTransaction();
        // Check if the file was already created before. Beware that the MRL here
        // is not the item's one. The mrl we're interested in is the one from the
        // subitem, ie. the playlist item we're adding.
        if ( File::exists( m_ml, mrl ) == false )
        {
            auto externalMedia = Media::create( m_ml, IMedia::Type::External, 0, 0,
                                                subitem.meta( IItem::Metadata::Title ), -1 );
            if ( externalMedia == nullptr )
            {
                LOG_ERROR( "Failed to create external media for ", mrl, " in the playlist ", playlistMrl );
                return;
            }
            // Assuming that external mrl present in playlist file is a main media resource
            auto externalFile = externalMedia->addExternalMrl( mrl, IFile::Type::Main );
            if ( externalFile == nullptr )
            {
                LOG_ERROR( "Failed to create external file for ", mrl, " in the playlist ", playlistMrl );
                return;
            }
        }
        try
        {
            if ( Task::createLinkTask( m_ml, mrl, playlistPtr->id(),
                                       Task::LinkType::Playlist,
                                       subitem.linkExtra() ) == nullptr )
            {
                return;
            }
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
        t2->commit();
        return;
    }
    bool isDirectory;
    try
    {
        isDirectory = utils::fs::isDirectory( utils::file::toLocalPath( mrl ) );
    }
    catch ( const fs::errors::UnhandledScheme& ex )
    {
        LOG_ERROR( "Can't check if ", mrl, " is a directory: ", ex.what() );
        return;
    }
    catch ( const fs::errors::System& ex )
    {
        LOG_ERROR( "Failed to check if ", mrl, " was a directory: ", ex.what() );
        return;
    }
    LOG_DEBUG( "Importing ", isDirectory ? "folder " : "file ", mrl, " in the playlist ", playlistMrl );
    auto directoryMrl = utils::file::directory( mrl );
    auto parentFolder = Folder::fromMrl( m_ml, directoryMrl );
    bool parentKnown = parentFolder != nullptr;

    // The minimal entrypoint must be a device mountpoint
    auto device = fsFactory->createDeviceFromMrl( mrl );
    if ( device == nullptr )
    {
        LOG_ERROR( "Can't add a local folder with unknown storage device. ");
        return;
    }
    auto entryPoint = device->mountpoint();
    if ( parentKnown == false && Folder::fromMrl( m_ml, entryPoint ) != nullptr )
    {
        auto probePtr = std::unique_ptr<prober::PathProbe>(
                    new prober::PathProbe{ utils::file::toLocalPath( mrl ),
                                           isDirectory, parentFolder,
                                           utils::file::toLocalPath( directoryMrl ),
                                           playlistPtr->id(), subitem.linkExtra(), true } );
        FsDiscoverer discoverer( fsFactory, m_ml, nullptr, std::move( probePtr ) );
        discoverer.reload( entryPoint, *this );
        return;
    }
    auto probePtr = std::unique_ptr<prober::PathProbe>(
                new prober::PathProbe{ utils::file::toLocalPath( mrl ),
                                    isDirectory, parentFolder,
                                    utils::file::toLocalPath( directoryMrl ),
                                    playlistPtr->id(), subitem.linkExtra(), false } );
    FsDiscoverer discoverer( fsFactory, m_ml, nullptr, std::move( probePtr ) );
    if ( parentKnown == false )
    {
        discoverer.discover( entryPoint, *this );
        auto entryFolder = Folder::fromMrl( m_ml, entryPoint );
        if ( entryFolder != nullptr )
        {
            sqlite::Tools::withRetries( 3, [this, entryFolder]() {
                Folder::excludeEntryFolder( m_ml, entryFolder->id() );
            });
        }
        return;
    }
    discoverer.reload( directoryMrl, *this );
}

/* Video files */

bool MetadataAnalyzer::parseVideoFile( IItem& item ) const
{
    auto media = static_cast<Media*>( item.media().get() );
    // Even though the title is usually the same as the filename when dealing
    // with a video file, we might be refreshing that media, and it might already
    // have a title, so let's analyse the filename again instead.
#ifndef _WIN32
    auto title = utils::title::sanitize( item.media()->fileName() );
#else
    auto title = item.media()->fileName();
#endif

    const auto& showName = item.meta( IItem::Metadata::ShowName );
    const auto& artworkMrl = item.meta( IItem::Metadata::ArtworkUrl );

    auto res = sqlite::Tools::withRetries( 3, [this, &showName, &title, media, &item, &artworkMrl]() {
        auto t = m_ml->getConn()->newTransaction();
        media->setTitleBuffered( title );

        if ( artworkMrl.empty() == false )
            media->setThumbnail( artworkMrl, Thumbnail::Origin::Media,
                                 ThumbnailSizeType::Thumbnail, false );

        if ( showName.length() != 0 )
        {
            const std::string req = "SELECT * FROM " + Show::Table::Name +
                    " WHERE title = ?";

            auto shows = Show::fetchAll<Show>( m_ml, req, showName );
            std::shared_ptr<Show> show;
            if ( shows.empty() == true )
            {
                show = m_ml->createShow( showName );
                if ( show == nullptr )
                    return false;
            }
            else
            {
                //FIXME: Discriminate amongst shows
                LOG_WARN( "Defaulting to first matching show" );
                show = shows[0];
            }
            auto episode = toInt( item, IItem::Metadata::Episode );
            if ( episode != 0 )
            {
                std::shared_ptr<Show> s = std::static_pointer_cast<Show>( show );
                s->addEpisode( *media, episode );
            }
        }
        else
        {
            // How do we know if it's a movie or a random video?
        }
        media->save();
        t->commit();
        return true;
    });
    if ( res == false )
        return false;
    auto thumbnail = media->thumbnail( ThumbnailSizeType::Thumbnail );
    // before relocating the thumbnail of a video media, bear in mind that the
    // thumbnailer thread might be processing the same media, and can be generating
    // a thumbnail right now
    if ( thumbnail != nullptr && thumbnail->isValid() &&
         thumbnail->isOwned() == false )
    {
        thumbnail->relocate();
    }
    return true;
}

std::tuple<Status, bool> MetadataAnalyzer::createFileAndMedia( IItem& item ) const
{
    assert( item.media() == nullptr );
    // Try to create Media & File
    auto mrl = item.mrl();
    const auto& tracks = item.tracks();

    if ( tracks.empty() == true )
    {
        LOG_WARN( "Failed to analyze ", item.mrl(), ": no tracks found" );
        return std::make_tuple( Status::Fatal, false );
    }
    auto isAudio = std::find_if( begin( tracks ), end( tracks ), [](const IItem::Track& t) {
        return t.type == IItem::Track::Type::Video;
    }) == end( tracks );
    auto t = m_ml->getConn()->newTransaction();
    auto file = File::fromExternalMrl( m_ml, mrl );
    // Check if this media was already added before as an external media
    if ( file != nullptr && file->type() == IFile::Type::Main )
    {
        auto media = file->media();
        if ( media == nullptr )
        {
            assert( !"External file must have an associated media" );
            return std::make_tuple( Status::Fatal, false );
        }
        if ( media->type() == IMedia::Type::External )
        {
            auto res = overrideExternalMedia( item, media, file, isAudio );
            // IItem::setFile will update the task in db, so run it as part of
            // the transation
            item.setFile( std::move( file ) );
            t->commit();
            item.setMedia( std::move( media ) );
            return std::make_tuple( res, isAudio );
        }
    }
    LOG_DEBUG( "Adding ", mrl );
    std::shared_ptr<Media> m;
    try
    {
        auto folder = static_cast<Folder*>( item.parentFolder().get() );
        m = Media::create( m_ml, isAudio ? IMedia::Type::Audio : IMedia::Type::Video,
                                folder->deviceId(), folder->id(),
                                utils::url::decode( utils::file::fileName( mrl ) ),
                                item.duration() );
        if ( m == nullptr )
        {
            LOG_ERROR( "Failed to add media ", mrl, " to the media library" );
            return std::make_tuple( Status::Fatal, false );
        }
        auto deviceFs = item.parentFolderFs()->device();
        if ( deviceFs == nullptr )
            throw fs::errors::DeviceRemoved{};
        // For now, assume all media are made of a single file
        file = m->addFile( *item.fileFs(), item.parentFolder()->id(),
                           deviceFs->isRemovable(), File::Type::Main );
        if ( file == nullptr )
        {
            LOG_ERROR( "Failed to add file ", mrl, " to media #", m->id() );
            return std::make_tuple( Status::Fatal, false );
        }
    }
    catch ( const sqlite::errors::ConstraintForeignKey& ex )
    {
        LOG_INFO( "Failed to add a media: ", ex.what(), ". Assuming the "
                  "containing folder got removed concurrently" );
        return std::make_tuple( Status::Discarded, false );
    }
    createTracks( *m, tracks );

    item.setMedia( std::move( m ) );
    item.setFile( std::move( file ) );

    t->commit();
    m_notifier->notifyMediaCreation( item.media() );
    return std::make_tuple( Status::Success, isAudio );
}

Status MetadataAnalyzer::overrideExternalMedia( IItem& item, std::shared_ptr<Media> media,
                                                std::shared_ptr<File> file, bool isAudio ) const
{
    // If the file is on a removable device, we need to update its mrl
    assert( sqlite::Transaction::transactionInProgress() == true );
    auto fsDir = item.parentFolderFs();
    auto deviceFs = fsDir->device();
    auto device = Device::fromUuid( m_ml, deviceFs->uuid() );
    if ( device == nullptr )
        return Status::Fatal;
    if ( file->update( *item.fileFs(), item.parentFolder()->id(),
                       deviceFs->isRemovable() ) == false )
        return Status::Fatal;
    media->setType( isAudio == true ? IMedia::Type::Audio : IMedia::Type::Video );
    media->setDuration( item.duration() );
    media->setDeviceId( device->id() );
    media->setFolderId( item.parentFolder()->id() );
    media->save();
    return Status::Success;
}

void MetadataAnalyzer::createTracks( Media& m, const std::vector<IItem::Track>& tracks ) const
{
    assert( sqlite::Transaction::transactionInProgress() == true );
    for ( const auto& track : tracks )
    {
        if ( track.type == IItem::Track::Type::Video )
        {
            m.addVideoTrack( track.codec, track.v.width, track.v.height,
                                  track.v.fpsNum, track.v.fpsDen, track.bitrate,
                                  track.v.sarNum, track.v.sarDen,
                                  track.language, track.description );
        }
        else if ( track.type == IItem::Track::Type::Audio )
        {
            m.addAudioTrack( track.codec, track.bitrate,
                                       track.a.rate, track.a.nbChannels,
                                       track.language, track.description );
        }
        else
        {
            assert( track.type == IItem::Track::Type::Subtitle );
            m.addSubtitleTrack( track.codec, track.language, track.description,
                                track.s.encoding );
        }
    }
}

std::tuple<bool, bool> MetadataAnalyzer::refreshFile( IItem& item ) const
{
    assert( item.file() != nullptr );

    auto file = item.file();

    switch ( file->type() )
    {
        case IFile::Type::Main:
            return refreshMedia( item );
        case IFile::Type::Playlist:
            return refreshPlaylist( item );
        case IFile::Type::Part:
        case IFile::Type::Soundtrack:
        case IFile::Type::Subtitles:
        case IFile::Type::Disc:
        case IFile::Type::Unknown:
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
    assert( media != nullptr );

    if ( media->duration() != item.duration() )
        media->setDuration( item.duration() );

    auto tracks = item.tracks();
    auto isAudio = std::find_if( begin( tracks ), end( tracks ), [](const IItem::Track& t) {
        return t.type == IItem::Track::Type::Video;
    }) == end( tracks );

    if ( isAudio == false )
    {
        auto newTitle = utils::title::sanitize( media->fileName() );
        media->setTitleBuffered( newTitle );
    }

    if ( isAudio == true && media->type() == IMedia::Type::Video )
        media->setType( IMedia::Type::Audio );
    else if ( isAudio == false && media->type() == IMedia::Type::Audio )
        media->setType( IMedia::Type::Video );

    auto t = m_ml->getConn()->newTransaction();
    if ( VideoTrack::removeFromMedia( m_ml, media->id() ) == false ||
         AudioTrack::removeFromMedia( m_ml, media->id() ) == false ||
         SubtitleTrack::removeFromMedia( m_ml, media->id() ) == false )
    {
        return std::make_tuple( false, false );
    }
    try
    {
        createTracks( *media, tracks );
    }
    catch ( const sqlite::errors::ConstraintForeignKey& ex )
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
                auto albumTrack = std::static_pointer_cast<AlbumTrack>( media->albumTrack() );
                if ( albumTrack == nullptr )
                {
                    LOG_ERROR( "Can't fetch album track associated with media ", media->id() );
                    assert( false );
                    return std::make_tuple( false, false );
                }
                auto album = std::static_pointer_cast<Album>( albumTrack->album() );
                if ( album == nullptr )
                {
                    LOG_ERROR( "Can't fetch album associated to album track ",
                               albumTrack->id(), "(media ", media->id(), ")" );
                    assert( false );
                    return std::make_tuple( false, false );
                }
                // No need to decrement the number of tracks for the artist
                // Removing the AlbumTrack will cause the 'delete_album_track'
                // trigger to fire, which will update nb_tracks/duration/presence
                // Additionally, it might delete the album if no more songs are
                // contained in it
                // However, that trigger isn't run for unknown/various artists
                // FIXME: Run the trigger for unknown/various artists, but do
                // not delete the artist when they reach 0 tracks themselves.
                if ( albumTrack->artistId() == UnknownArtistID ||
                     albumTrack->artistId() == VariousArtistID )
                {
                    auto artist = std::static_pointer_cast<Artist>( albumTrack->artist() );
                    artist->updateNbTrack( -1 );
                }

                // The album artist however will not be affected by this trigger
                // so we need to decrement its tracks count manually
                // FIXME: Add a trigger to react upon MediaArtistRelation
                // deletion & insertion.
                auto albumArtist = std::static_pointer_cast<Artist>( album->albumArtist() );
                if ( albumArtist && albumArtist->id() != albumTrack->artistId() )
                    albumArtist->updateNbTrack( -1 );

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

                album->removeTrack( *media, *albumTrack );
                AlbumTrack::destroy( m_ml, albumTrack->id() );
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
                break;
            }
            case IMedia::SubType::Unknown:
                assert( !"Unreachable" );
                return std::make_tuple( false, false );
        }
        media->setSubType( IMedia::SubType::Unknown );
    }

    if ( media->save() == false )
        return std::make_tuple( false, false );
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

/* Audio files */

bool MetadataAnalyzer::parseAudioFile( IItem& item )
{
    auto media = static_cast<Media*>( item.media().get() );

    auto genre = handleGenre( item );
    auto artists = findOrCreateArtist( item );
    if ( artists.first == nullptr && artists.second == nullptr )
        return false;
    auto album = findAlbum( item, artists.first, artists.second );
    auto newAlbum = album == nullptr;

    /*
     * Check for a cover file out of the transaction/retry scope
     * We only check for album artwork when the album is about to be created
     */
    std::shared_ptr<Thumbnail> mediaThumbnail;

    auto artworkMrl = item.meta( IItem::Metadata::ArtworkUrl );

    if ( artworkMrl.empty() == false )
    {
        // Here we expect the thumbnail mrl to be an attachment. Any xxx.jpg
        // should have been found by findAlbumArtwork
        // We don't insert the thumbnail in DB yet, to process everything
        // as part of the transaction

        // If this task is a refresh task, we already dropped the thumbnail
        // in case we had an artwork mrl.
        // If this is a new media, we won't have a thumbnail at all
        assert( media->thumbnail( ThumbnailSizeType::Thumbnail ) == nullptr );
        mediaThumbnail = std::make_shared<Thumbnail>( m_ml, artworkMrl,
                                                 Thumbnail::Origin::Media,
                                                 ThumbnailSizeType::Thumbnail,
                                                 false );
    }

    /*
     * Any entity that might be modified in the retry scope needs to be passed
     * as parameter, so that we only modify a copy of it.
     * For instance, if we create the album, and fail later on, we would rollback
     * the transaction and retry, however the album instance wouldn't be nullptr
     * anymore, and we wouldn't recreate it
     */
    auto res = sqlite::Tools::withRetries( 3,
            [this, &item, &artists, media, &mediaThumbnail, &genre, newAlbum]
            ( std::shared_ptr<Album> album ) {

        auto t = m_ml->getConn()->newTransaction();

        if ( mediaThumbnail != nullptr )
        {
            if ( mediaThumbnail->id() == 0 )
                mediaThumbnail->insert();
            // Don't rely on the same thumbnail as the one for the album, the
            // user can always update the media specific thumbnail, and we don't
            // want that to propagate to the album
            media->setThumbnail( mediaThumbnail );
        }
        if ( album == nullptr )
        {
            const auto& albumName = item.meta( IItem::Metadata::Album );
            album = m_ml->createAlbum( albumName );
            if ( album == nullptr )
                return false;
            if ( mediaThumbnail != nullptr )
                album->setThumbnail( mediaThumbnail );
        }
        // TODO: Use embedded artwork for the album

        try
        {
            // If we know a track artist, specify it, otherwise, fallback to the album/unknown artist
            handleTrack( album, item, artists.second ? artists.second : artists.first,
                         genre.get() );
        }
        catch ( const sqlite::errors::ConstraintForeignKey& ex )
        {
            LOG_INFO( "Failed to insert album track: ", ex.what(), ". "
                      "Assuming the media was deleted concurrently" );
            return false;
        }

        link( item, *album, artists.first, artists.second, mediaThumbnail );
        media->save();
        t->commit();

        /* We won't retry anymore, so send any notification now, as the outer
         * scope doesn't know we modified the passed instance */
        if ( newAlbum == true )
            m_notifier->notifyAlbumCreation( album );

        return true;
    }, std::move( album ) );

    if ( res == false )
        return false;

    if ( mediaThumbnail != nullptr )
    {
        assert( mediaThumbnail->isValid() );
        mediaThumbnail->relocate();
    }
    return true;
}

std::shared_ptr<Genre> MetadataAnalyzer::handleGenre( IItem& item ) const
{
    const auto& genreStr = item.meta( IItem::Metadata::Genre );
    if ( genreStr.length() == 0 )
        return nullptr;
    auto genre = Genre::fromName( m_ml, genreStr );
    if ( genre == nullptr )
    {
        genre = Genre::create( m_ml, genreStr );
        if ( genre == nullptr )
            LOG_ERROR( "Failed to get/create Genre", genreStr );
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

/* Album handling */

std::shared_ptr<Album> MetadataAnalyzer::findAlbum( IItem& item, std::shared_ptr<Artist> albumArtist,
                                                    std::shared_ptr<Artist> trackArtist )
{
    const auto& albumName = item.meta( IItem::Metadata::Album );
    if ( albumName.empty() == true )
    {
        if ( albumArtist != nullptr )
            return albumArtist->unknownAlbum();
        if ( trackArtist != nullptr )
            return trackArtist->unknownAlbum();
        return m_unknownArtist->unknownAlbum();
    }

    auto file = static_cast<File*>( item.file().get() );
    if ( m_previousAlbum != nullptr && albumName == m_previousAlbum->title() &&
         m_previousFolderId != 0 && file->folderId() == m_previousFolderId )
        return m_previousAlbum;
    m_previousAlbum.reset();
    m_previousFolderId = 0;

    // Album matching depends on the difference between artist & album artist.
    // Specificaly pass the albumArtist here.
    static const std::string req = "SELECT * FROM " + Album::Table::Name +
            " WHERE title = ?";
    auto albums = Album::fetchAll<Album>( m_ml, req, albumName );

    if ( albums.size() == 0 )
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
        if ( tracks.size() == 0 )
        {
            ++it;
            continue;
        }

        auto multiDisc = false;
        auto multipleArtists = false;
        int64_t previousArtistId = trackArtist != nullptr ? trackArtist->id() : 0;
        for ( auto& t : tracks )
        {
            auto at = t->albumTrack();
            assert( at != nullptr );
            if ( at == nullptr )
                continue;
            if ( at->discNumber() > 1 )
                multiDisc = true;
            if ( previousArtistId != 0 && previousArtistId != at->artist()->id() )
                multipleArtists = true;
            previousArtistId = at->artist()->id();
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
    if ( albums.size() == 0 )
        return nullptr;
    if ( albums.size() > 1 )
    {
        LOG_WARN( "Multiple candidates for album ", albumName, ". Selecting first one out of luck" );
    }
    m_previousFolderId = file->folderId();
    m_previousAlbum = albums[0];
    return albums[0];
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

    const auto& albumArtistStr = item.meta( IItem::Metadata::AlbumArtist );
    const auto& artistStr = item.meta( IItem::Metadata::Artist );
    if ( albumArtistStr.empty() == true && artistStr.empty() == true )
    {
        return {m_unknownArtist, m_unknownArtist};
    }

    if ( albumArtistStr.empty() == false )
    {
        albumArtist = Artist::fetch( m_ml, req, albumArtistStr );
        if ( albumArtist == nullptr )
        {
            albumArtist = m_ml->createArtist( albumArtistStr );
            if ( albumArtist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist ", albumArtistStr );
                return {nullptr, nullptr};
            }
            m_notifier->notifyArtistCreation( albumArtist );
        }
    }
    if ( artistStr.empty() == false && artistStr != albumArtistStr )
    {
        artist = Artist::fetch( m_ml, req, artistStr );
        if ( artist == nullptr )
        {
            artist = m_ml->createArtist( artistStr );
            if ( artist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist ", artistStr );
                return {nullptr, nullptr};
            }
            m_notifier->notifyArtistCreation( artist );
        }
    }
    return {albumArtist, artist};
}

/* Tracks handling */

std::shared_ptr<AlbumTrack> MetadataAnalyzer::handleTrack( std::shared_ptr<Album> album, IItem& item,
                                                         std::shared_ptr<Artist> artist, Genre* genre ) const
{
    assert( sqlite::Transaction::transactionInProgress() == true );

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
        media->setTitleBuffered( title );

    auto track = std::static_pointer_cast<AlbumTrack>( album->addTrack( media, trackNumber,
                                                                        discNumber, artist->id(),
                                                                        genre ) );
    if ( track == nullptr )
    {
        LOG_ERROR( "Failed to create album track" );
        return nullptr;
    }

    const auto& releaseDate = item.meta( IItem::Metadata::Date );
    if ( releaseDate.empty() == false )
    {
        auto releaseYear = atoi( releaseDate.c_str() );
        media->setReleaseDate( releaseYear );
        // Let the album handle multiple dates. In order to do this properly, we need
        // to know if the date has been changed before, which can be known only by
        // using Album class internals.
        album->setReleaseYear( releaseYear, false );
    }
    return track;
}

/* Misc */

void MetadataAnalyzer::link( IItem& item, Album& album,
                             std::shared_ptr<Artist> albumArtist,
                             std::shared_ptr<Artist> artist,
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
        auto albumThumbnail = album.thumbnail( mediaThumbnail->sizeType() );

        // We might modify albumArtist later, hence handle thumbnails before.
        // If we have an albumArtist (meaning the track was properly tagged, we
        // can assume this artist is a correct match. We can use the thumbnail from
        // the current album for the albumArtist, if none has been set before.
        // Although we don't want to do this for unknown/various artists, as the
        // thumbnail wouldn't reflect those "special" artists
        if ( albumArtist->id() != UnknownArtistID &&
             albumArtist->id() != VariousArtistID &&
             albumThumbnail != nullptr )
        {
            auto albumArtistThumbnail = albumArtist->thumbnail( mediaThumbnail->sizeType() );
            // If the album artist has no thumbnail, let's assign it
            if ( albumArtistThumbnail == nullptr )
            {
                albumArtist->setThumbnail( mediaThumbnail, Thumbnail::Origin::AlbumArtist );
            }
            else if ( albumArtistThumbnail->origin() == Thumbnail::Origin::Artist )
            {
                // We only want to change the thumbnail if it was assigned from an
                // album this artist was only featuring on
            }
        }
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
        // Always add the album artist as an artist
        // Always update the album artist number of tracks.
        // The artist might be different, and will be handled a few lines below
        albumArtist->updateNbTrack( 1 );
        if ( artist != nullptr )
        {
            // If the album artist is not the artist, we need to update the
            // album artist track count as well.
            if ( albumArtist->id() != artist->id() )
                artist->updateNbTrack( 1 );
        }
    }
    else
    {
        // We have more than a single artist on this album, fallback to various artists
        if ( albumArtist->id() != currentAlbumArtist->id() )
        {
            if ( m_variousArtists == nullptr )
                m_variousArtists = Artist::fetch( m_ml, VariousArtistID );
            // If we already switched to various artist, no need to do it again
            if ( m_variousArtists->id() != currentAlbumArtist->id() )
            {
                // All tracks from this album must now also be reflected in various
                // artist number of tracks
                m_variousArtists->updateNbTrack( album.nbTracks() );
                album.setAlbumArtist( m_variousArtists );
            }
            // However we always need to bump the various artist number of tracks
            else
            {
                m_variousArtists->updateNbTrack( 1 );
            }
        }
        if ( artist != nullptr && artist->id() != albumArtist->id() )
           artist->updateNbTrack( 1 );
        albumArtist->updateNbTrack( 1 );
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

const char* MetadataAnalyzer::name() const
{
    return "Metadata";
}

void MetadataAnalyzer::onFlushing()
{
    m_variousArtists = nullptr;
    m_previousAlbum = nullptr;
    m_previousFolderId = 0;
}

void MetadataAnalyzer::onRestarted()
{
    // Reset locally cached entities
    cacheUnknownArtist();
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

bool MetadataAnalyzer::isInterrupted() const
{
    return m_stopped.load();
}

}
}
