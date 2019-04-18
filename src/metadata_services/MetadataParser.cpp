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

#include "MetadataParser.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "File.h"
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
#include "discoverer/FsDiscoverer.h"
#include "discoverer/probe/PathProbe.h"

#include <cstdlib>
#include <algorithm>

namespace medialibrary
{
namespace parser
{

MetadataAnalyzer::MetadataAnalyzer()
    : m_ml( nullptr )
    , m_previousFolderId( 0 )
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
        bool success;
        bool needRescan;
        std::tie( success, needRescan ) = refreshFile( item );
        if ( success == false )
            return Status::Fatal;
        auto file = std::static_pointer_cast<File>( item.file() );
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
        auto res = addPlaylistMedias( item );
        if ( res != Status::Success ) // playlist addition may fail due to constraint violation
            return res;

        assert( item.file() != nullptr );
        return Status::Completed;
    }

    bool isAudio;

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
             item.media()->videoTracks()->count() == 0 )
        {
            auto t = m_ml->getConn()->newTransaction();
            createTracks( static_cast<Media&>( *item.media() ), item.tracks() );
            t->commit();
        }
        isAudio = item.media()->type() == IMedia::Type::Audio;
    }
    auto media = std::static_pointer_cast<Media>( item.media() );

    if ( item.parentPlaylist() != nullptr )
        item.parentPlaylist()->add( *media, item.parentPlaylistIndex() );

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

    m_notifier->notifyMediaModification( media );
    return Status::Success;
}

/* Playlist files */

Status MetadataAnalyzer::addPlaylistMedias( IItem& item ) const
{
    const auto& mrl = item.mrl();
    LOG_DEBUG( "Try to import ", mrl, " as a playlist" );
    std::shared_ptr<Playlist> playlistPtr;
    if ( item.file() != nullptr )
    {
        // We are most likely re-scanning a file representing a playlist.
        // If a task has a file, it means the playlist & the associated file have
        // been created.
        std::string req = "SELECT * FROM " + Playlist::Table::Name +
                " WHERE file_id = ?";
        playlistPtr = Playlist::fetch( m_ml, req, item.file()->id() );
        if ( playlistPtr == nullptr )
        {
            // The playlist had to be created, something is very wrong, give up
            assert( false );
            return Status::Discarded;
        }
    }
    else
    {
        auto playlistName = item.meta( IItem::Metadata::Title );
        if ( playlistName.empty() == true )
            playlistName = utils::url::decode( utils::file::fileName( mrl ) );
        auto t = m_ml->getConn()->newTransaction();
        playlistPtr = Playlist::create( m_ml, playlistName );
        if ( playlistPtr == nullptr )
        {
            LOG_ERROR( "Failed to create playlist ", mrl, " to the media library" );
            return Status::Fatal;
        }
        m_notifier->notifyPlaylistCreation( playlistPtr );

        auto deviceFs = item.parentFolderFs()->device();
        if ( deviceFs == nullptr )
            throw fs::DeviceRemovedException{};
        auto file = playlistPtr->addFile( *item.fileFs(),
                                          item.parentFolder()->id(),
                                          deviceFs->isRemovable() );
        if ( file == nullptr )
        {
            LOG_ERROR( "Failed to add playlist file ", mrl );
            return Status::Fatal;
        }
        // Will invoke ITaskCb::updateFileId to upadte m_fileId & its
        // representation in DB
        item.setFile( std::move( file ) );
        t->commit();
    }
    // Now regardless of if the playlist is re-scanned or discovered from the
    // first time, just schedule all members for insertion. media & files will
    // be recreated if need be, and appropriate entries in PlaylistMediaRelation
    // table will be recreated to link things together.

    for ( auto i = 0u; i < item.nbSubItems(); ++i ) // FIXME: Interrupt loop if paused
        addPlaylistElement( item, playlistPtr, item.subItem( i ) );

    return Status::Success;
}

void MetadataAnalyzer::addPlaylistElement( IItem& item,
                                         std::shared_ptr<Playlist> playlistPtr,
                                         const IItem& subitem ) const
{
    const auto& mrl = subitem.mrl();
    const auto& playlistMrl = item.mrl();
    LOG_DEBUG( "Try to add ", mrl, " to the playlist ", playlistMrl );
    auto media = m_ml->media( mrl );
    if ( media != nullptr )
    {
        LOG_DEBUG( "Media for ", mrl, " already exists, adding it to the playlist ", playlistMrl );
        auto res = playlistPtr->add( *media, subitem.parentPlaylistIndex() );
        if ( res == false )
            LOG_ERROR( "Failed to insert ", mrl, " in playlist ", playlistMrl );
        return;
    }
    // Create Media, etc.
    auto fsFactory = m_ml->fsFactoryForMrl( mrl );

    if ( fsFactory == nullptr ) // Media not supported by any FsFactory, registering it as external
    {
        auto t2 = m_ml->getConn()->newTransaction();
        auto externalMedia = Media::create( m_ml, IMedia::Type::External, 0, 0,
                                            subitem.meta( IItem::Metadata::Title ) );
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
        playlistPtr->add( *externalMedia, subitem.parentPlaylistIndex() );
        t2->commit();
        return;
    }
    bool isDirectory;
    try
    {
        isDirectory = utils::fs::isDirectory( utils::file::toLocalPath( mrl ) );
    }
    catch ( std::system_error& ex )
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
                       isDirectory, std::move( playlistPtr ), parentFolder,
                       utils::file::toLocalPath( directoryMrl ), subitem.parentPlaylistIndex(), true } );
        FsDiscoverer discoverer( fsFactory, m_ml, nullptr, std::move( probePtr ) );
        discoverer.reload( entryPoint );
        return;
    }
    auto probePtr = std::unique_ptr<prober::PathProbe>(
                new prober::PathProbe{ utils::file::toLocalPath( mrl ),
                   isDirectory, std::move( playlistPtr ), parentFolder,
                   utils::file::toLocalPath( directoryMrl ), subitem.parentPlaylistIndex(), false } );
    FsDiscoverer discoverer( fsFactory, m_ml, nullptr, std::move( probePtr ) );
    if ( parentKnown == false )
    {
        discoverer.discover( entryPoint );
        auto entryFolder = Folder::fromMrl( m_ml, entryPoint );
        if ( entryFolder != nullptr )
            Folder::excludeEntryFolder( m_ml, entryFolder->id() );
        return;
    }
    discoverer.reload( directoryMrl );
}

/* Video files */

bool MetadataAnalyzer::parseVideoFile( IItem& item ) const
{
    auto media = static_cast<Media*>( item.media().get() );
    const auto& title = item.meta( IItem::Metadata::Title );
    if ( title.length() == 0 )
        return true;

    const auto& showName = item.meta( IItem::Metadata::ShowName );
    const auto& artworkMrl = item.meta( IItem::Metadata::ArtworkUrl );

    return sqlite::Tools::withRetries( 3, [this, &showName, &title, media, &item, &artworkMrl]() {
        auto t = m_ml->getConn()->newTransaction();
        media->setTitleBuffered( title );

        if ( artworkMrl.empty() == false )
            media->setThumbnail( artworkMrl, Thumbnail::Origin::Media, false );

        if ( showName.length() != 0 )
        {
            const std::string req = "SELECT * FROM " + Show::Table::Name +
                    " WHERE name = ?";

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
    try
    {
        auto isAudio = std::find_if( begin( tracks ), end( tracks ), [](const Task::Item::Track& t) {
            return t.type == Task::Item::Track::Type::Video;
        }) == end( tracks );
        auto t = m_ml->getConn()->newTransaction();
        LOG_DEBUG( "Adding ", mrl );
        auto folder = static_cast<Folder*>( item.parentFolder().get() );
        auto m = Media::create( m_ml, isAudio ? IMedia::Type::Audio : IMedia::Type::Video,
                                folder->deviceId(), folder->id(),
                                utils::url::decode( utils::file::fileName( mrl ) ) );
        if ( m == nullptr )
        {
            LOG_ERROR( "Failed to add media ", mrl, " to the media library" );
            return std::make_tuple( Status::Fatal, false );
        }
        auto deviceFs = item.parentFolderFs()->device();
        if ( deviceFs == nullptr )
            throw fs::DeviceRemovedException{};
        // For now, assume all media are made of a single file
        auto file = m->addFile( *item.fileFs(),
                                item.parentFolder()->id(),
                                deviceFs->isRemovable(),
                                File::Type::Main );
        if ( file == nullptr )
        {
            LOG_ERROR( "Failed to add file ", mrl, " to media #", m->id() );
            return std::make_tuple( Status::Fatal, false );
        }
        createTracks( *m, tracks );
        m->setDuration( item.duration() );

        item.setMedia( std::move( m ) );
        // Will invoke ITaskCb::updateFileId to upadte m_fileId & its
        // representation in DB
        item.setFile( std::move( file ) );

        t->commit();
        m_notifier->notifyMediaCreation( item.media() );
        return std::make_tuple( Status::Success, isAudio );
    }
    // Voluntarily trigger an exception for a valid, but less common case, to avoid database overhead
    catch ( sqlite::errors::ConstraintViolation& ex )
    {
        LOG_WARN( "Creation of Media & File failed because ", ex.what(),
                  ". Assuming this task is a duplicate" );
        // continue out of the exception handler
    }
    // Try to retrieve file & Media from database
    auto fileInDB = File::fromMrl( m_ml, mrl );
    if ( fileInDB == nullptr ) // The file is no longer present in DB, gracefully delete task
    {
        LOG_ERROR( "File ", mrl, " no longer present in DB, aborting");
        return std::make_tuple( Status::Fatal, false );
    }
    // Now that we ran some basic sanity check, if this was not a playlist
    // insertion, there's nothing much left to do anyway
    if ( item.parentPlaylist() == nullptr )
        return std::make_tuple( Status::Discarded, false );
    auto media = fileInDB->media();
    if ( media == nullptr )
    {
        LOG_ERROR( "File ", mrl, " doesn't have an associated media: aborting" );
        return std::make_tuple( Status::Fatal, false );
    }
    // The media was already inserted but this task still exists, with a playlist
    // index, so we need to ensure the media was already inserted into the playlist
    auto playlist = static_cast<Playlist*>( item.parentPlaylist().get() );
    if ( playlist->contains( media->id(), item.parentPlaylistIndex() ) == false )
    {
        auto success = item.parentPlaylist()->add( *media, item.parentPlaylistIndex() );
        if ( success == false )
            return std::make_tuple( Status::Discarded, false );
    }
    // In any case, there is nothing else to do for this task, return Completed to avoid
    // the task itself being deleted & to prevent the analysis to continue.
    // In this case, isAudio will be ignored
    return std::make_tuple( Status::Completed, false );
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
    assert( item.media() == nullptr );
    assert( item.file() != nullptr );

    auto file = item.file();

    switch ( file->type() )
    {
        case IFile::Type::Main:
            return refreshMedia( item );
        case IFile::Type::Playlist:
        {
            auto playlist = Playlist::fromFile( m_ml, file->id() );
            if ( playlist == nullptr )
            {
                LOG_WARN( "Failed to find playlist associated to modified playlist file ",
                          item.mrl() );
                return std::make_tuple( false, false );
            }
            LOG_DEBUG( "Reloading playlist ", playlist->name(), " on ", item.mrl() );
            playlist->clearContent();
            return std::make_tuple( true, true );
        }
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
    auto media = file->media();
    assert( media != nullptr );

    if ( media->duration() != item.duration() )
        media->setDuration( item.duration() );

    auto newTitle = item.meta( IItem::Metadata::Title );
    if ( media->title() != newTitle )
        media->setTitleBuffered( newTitle );

    auto tracks = item.tracks();
    auto isAudio = std::find_if( begin( tracks ), end( tracks ), [](const Task::Item::Track& t) {
        return t.type == Task::Item::Track::Type::Video;
    }) == end( tracks );

    if ( isAudio == true && media->type() == IMedia::Type::Video )
        media->setType( IMedia::Type::Audio );
    else if ( isAudio == false && media->type() == IMedia::Type::Audio )
        media->setType( IMedia::Type::Video );

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
                    assert( false );
                    LOG_ERROR( "Can't fetch album track associated with media ", media->id() );
                    break;
                }
                auto album = std::static_pointer_cast<Album>( albumTrack->album() );
                if ( album == nullptr )
                {
                    assert( false );
                    LOG_ERROR( "Can't fetch album associated to album track ",
                               albumTrack->id(), "(media ", media->id(), ")" );
                }
                album->removeTrack( *media, *albumTrack );
                AlbumTrack::destroy( m_ml, albumTrack->id() );
                break;
            }
            case IMedia::SubType::Movie:
            {
                auto movie = media->movie();
                if ( movie == nullptr )
                {
                    assert( false );
                    LOG_ERROR( "Failed to fetch movie associated with media ", media->id() );
                    break;
                }
                Movie::destroy( m_ml, movie->id() );
                break;
            }
            case IMedia::SubType::ShowEpisode:
            {
                auto episode = std::static_pointer_cast<ShowEpisode>( media->showEpisode() );
                if ( episode == nullptr )
                {
                    assert( false );
                    LOG_ERROR( "Failed to fetch show episode associated with media ", media->id() );
                    break;
                }
                ShowEpisode::destroy( m_ml, episode->id() );
                break;
            }
            case IMedia::SubType::Unknown:
                assert( !"Unreachable" );
                break;
        }
        media->setSubType( IMedia::SubType::Unknown );
    }

    if ( media->save() == false )
        return std::make_tuple( false, false );
    item.setMedia( std::move( media ) );
    return std::make_tuple( true, needRescan );
}

/* Audio files */

bool MetadataAnalyzer::parseAudioFile( IItem& item )
{
    auto media = static_cast<Media*>( item.media().get() );

    auto artworkMrl = item.meta( IItem::Metadata::ArtworkUrl );
    if ( artworkMrl.empty() == false )
    {
        media->setThumbnail( artworkMrl, Thumbnail::Origin::Media, false );
        // Don't use an attachment as default artwork for album/artists
        if ( utils::file::schemeIs( "attachment", artworkMrl ) )
            artworkMrl.clear();
    }


    auto genre = handleGenre( item );
    auto artists = findOrCreateArtist( item );
    if ( artists.first == nullptr && artists.second == nullptr )
        return false;
    auto album = findAlbum( item, artists.first, artists.second );
    return sqlite::Tools::withRetries( 3, [this, &item, &artists, media]( std::string artworkMrl,
                                                  std::shared_ptr<Album> album, std::shared_ptr<Genre> genre ) {
        auto t = m_ml->getConn()->newTransaction();
        if ( album == nullptr )
        {
            const auto& albumName = item.meta( IItem::Metadata::Album );
            int64_t thumbnailId = 0;
            if ( artworkMrl.empty() == false )
            {
                auto thumbnail = Thumbnail::create( m_ml, artworkMrl,
                                                    Thumbnail::Origin::Album, false );
                if ( thumbnail != nullptr )
                    thumbnailId = thumbnail->id();
            }
            album = m_ml->createAlbum( albumName, thumbnailId );
            if ( album == nullptr )
                return false;
            m_notifier->notifyAlbumCreation( album );
        }
        // If we know a track artist, specify it, otherwise, fallback to the album/unknown artist
        auto track = handleTrack( album, item, artists.second ? artists.second : artists.first,
                                  genre.get() );

        auto res = link( item, *album, artists.first, artists.second );
        media->save();
        t->commit();
        m_notifier->notifyAlbumModification( album );
        if ( genre != nullptr )
            m_notifier->notifyGenreModification( genre );
        if ( artists.first != nullptr )
            m_notifier->notifyArtistModification( artists.first );
        if ( artists.second != nullptr )
            m_notifier->notifyArtistModification( artists.second );
        return res;
    }, std::move( artworkMrl ), std::move( album ), std::move( genre ) );
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
        LOG_WARN( "Failed to get track title" );
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

bool MetadataAnalyzer::link( IItem& item, Album& album,
                               std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist )
{
    Media& media = static_cast<Media&>( *item.media() );

    if ( albumArtist == nullptr )
    {
        assert( artist != nullptr );
        albumArtist = artist;
    }
    assert( albumArtist != nullptr );

    auto albumThumbnail = album.thumbnail();

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
        auto albumArtistThumbnail = albumArtist->thumbnail();
        // If the album artist has no thumbnail, let's assign it
        if ( albumArtistThumbnail == nullptr )
        {
            albumArtist->setArtworkMrl( albumThumbnail->mrl(),
                                        Thumbnail::Origin::AlbumArtist, false );
        }
        else if ( albumArtistThumbnail->origin() == Thumbnail::Origin::Artist )
        {
            // We only want to change the thumbnail if it was assigned from an
            // album this artist was only featuring on
        }
    }

    // Until we have a better artwork extraction/assignation, simply do the same
    // for artists
    if ( artist != nullptr && artist->id() != UnknownArtistID &&
         artist->id() != VariousArtistID &&
         albumThumbnail != nullptr && artist->thumbnail() == nullptr )
    {
        artist->setArtworkMrl( album.thumbnailMrl(), Thumbnail::Origin::Artist, false );
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

    return true;
}

const char* MetadataAnalyzer::name() const
{
    return "Metadata";
}

uint8_t MetadataAnalyzer::nbThreads() const
{
//    auto nbProcs = std::thread::hardware_concurrency();
//    if ( nbProcs == 0 )
//        return 1;
//    return nbProcs;
    // Let's make this code thread-safe first :)
    return 1;
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
}

Step MetadataAnalyzer::targetedStep() const
{
    return Step::MetadataAnalysis;
}

}
}
