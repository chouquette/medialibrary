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
#include "filesystem/IDevice.h"
#include "filesystem/IDirectory.h"
#include "Folder.h"
#include "Genre.h"
#include "Media.h"
#include "Playlist.h"
#include "Show.h"
#include "utils/Directory.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "utils/ModificationsNotifier.h"
#include "discoverer/FsDiscoverer.h"
#include "discoverer/probe/PathProbe.h"

#include <cstdlib>

namespace medialibrary
{

MetadataParser::MetadataParser()
    : m_previousFolderId( 0 )
{
}

bool MetadataParser::initialize()
{
    m_unknownArtist = Artist::fetch( m_ml, UnknownArtistID );
    if ( m_unknownArtist == nullptr )
        LOG_ERROR( "Failed to cache unknown artist" );
    return m_unknownArtist != nullptr;
}

int MetadataParser::toInt( VLC::Media& vlcMedia, libvlc_meta_t meta, const char* name )
{
    auto str = vlcMedia.meta( meta );
    if ( str.empty() == false )
    {
        try
        {
            return std::stoi( str );
        }
        catch( std::logic_error& ex)
        {
            LOG_WARN( "Invalid ", name, " provided (", str, "): ", ex.what() );
        }
    }
    return 0;
}

parser::Task::Status MetadataParser::run( parser::Task& task )
{
    bool alreadyInParser = false;
    int nbSubitem = task.vlcMedia.subitems()->count();
    // Assume that file containing subitem(s) is a Playlist
    if ( nbSubitem > 0 )
    {
        auto res = addPlaylistMedias( task, nbSubitem );
        if ( res == false ) // playlist addition may fail due to constraint violation
            return parser::Task::Status::Fatal;

        assert( task.file != nullptr );
        task.markStepCompleted( parser::Task::ParserStep::Completed );
        task.file->saveParserStep();
        return parser::Task::Status::Success;
    }

    if ( task.file == nullptr )
    {
        assert( task.media == nullptr );
        // Try to create Media & File
        try
        {
            auto t = m_ml->getConn()->newTransaction();
            LOG_INFO( "Adding ", task.mrl );
            task.media = Media::create( m_ml, IMedia::Type::Unknown, utils::file::fileName( task.mrl ) );
            if ( task.media == nullptr )
            {
                LOG_ERROR( "Failed to add media ", task.mrl, " to the media library" );
                return parser::Task::Status::Fatal;
            }
            // For now, assume all media are made of a single file
            task.file = task.media->addFile( *task.fileFs, task.parentFolder->id(),
                                             task.parentFolderFs->device()->isRemovable(),
                                             File::Type::Main );
            if ( task.file == nullptr )
            {
                LOG_ERROR( "Failed to add file ", task.mrl, " to media #", task.media->id() );
                return parser::Task::Status::Fatal;
            }
            t->commit();
            // Synchronize file step tracker with task
            task.markStepCompleted( task.step );
        }
        // Voluntarily trigger an exception for a valid, but less common case, to avoid database overhead
        catch ( sqlite::errors::ConstraintViolation& ex )
        {
            LOG_INFO( "Creation of Media & File failed because ", ex.what(),
                      ". Assuming this task is a duplicate" );
            // Try to retrieve file & Media from database
            auto fileInDB = File::fromMrl( m_ml, task.mrl );
            if ( fileInDB == nullptr ) // The file is no longer present in DB, gracefully delete task
            {
                LOG_ERROR( "File ", task.mrl, " no longer present in DB, aborting");
                return parser::Task::Status::Fatal;
            }
            task.file = fileInDB;
            task.media = fileInDB->media();
            if ( task.media == nullptr ) // Without a media, we cannot go further
                return parser::Task::Status::Fatal;
            alreadyInParser = true;
        }
    }

    if ( task.parentPlaylist != nullptr )
        task.parentPlaylist->add( task.media->id(), task.parentPlaylistIndex );

    if ( alreadyInParser == true )
    {
        task.step = parser::Task::ParserStep::Completed; // Let the worker drop this duplicate task
        return parser::Task::Status::Success;
    }

    const auto& tracks = task.vlcMedia.tracks();

    // If we failed to extract any tracks, don't make any assumption and forward to the
    // thumbnailer. Since it starts an actual playback, it will have more information.
    // Since the metadata steps won't be marked, it will run again once the thumbnailer has completed.
    if ( tracks.empty() == true )
    {
        // However, if the file is not unknown anymore, it means the thumbnailer has already processed it
        if ( task.media->type() == Media::Type::Unknown )
        {
            // In case the thumbnailer ran before, but the application exited, we would skip the
            // thumbnailer execution, coming back here, and delegating again to the thumbnailer
            // over and over again. We need to ensure the thumbnailer will run, even partially, up to
            // the point the playback started.
            task.markStepUncompleted( parser::Task::ParserStep::Thumbnailer );
            LOG_INFO( "Skipping metadata parsing for file with unknown type: ", task.file->mrl() );
            return parser::Task::Status::Success;
        }
        // In that case, stop trying to do something with this file.
        return parser::Task::Status::Fatal;
    }

    bool isAudio = true;
    {
        using TracksT = decltype( tracks );
        sqlite::Tools::withRetries( 3, [this, &isAudio, &task]( TracksT tracks ) {
            auto t = m_ml->getConn()->newTransaction();
            for ( const auto& track : tracks )
            {
                auto codec = track.codec();
                std::string fcc( reinterpret_cast<const char*>( &codec ), 4 );
                if ( track.type() == VLC::MediaTrack::Type::Video )
                {
                    task.media->addVideoTrack( fcc, track.width(), track.height(),
                                          static_cast<float>( track.fpsNum() ) / static_cast<float>( track.fpsDen() ),
                                          track.language(), track.description() );
                    isAudio = false;
                }
                else if ( track.type() == VLC::MediaTrack::Type::Audio )
                {
                    task.media->addAudioTrack( fcc, track.bitrate(), track.rate(), track.channels(),
                                          track.language(), track.description() );
                }
            }
            task.media->setDuration( task.vlcMedia.duration() );
            t->commit();
        }, std::move( tracks ) );
    }
    if ( isAudio == true )
    {
        if ( parseAudioFile( task ) == false )
            return parser::Task::Status::Fatal;
    }
    else
    {
        if (parseVideoFile( task ) == false )
            return parser::Task::Status::Fatal;
    }

    if ( task.file->isDeleted() == true || task.media->isDeleted() == true )
        return parser::Task::Status::Fatal;

    task.markStepCompleted( parser::Task::ParserStep::MetadataAnalysis );
    // Save ourselves from the useless processing of a thumbnail later if
    // we're analyzing an audio file
    if ( isAudio == true && utils::file::schemeIs( "attachment://", task.media->thumbnail() ) == false )
        task.markStepCompleted( parser::Task::ParserStep::Thumbnailer );
    if ( task.file->saveParserStep() == false )
        return parser::Task::Status::Fatal;
    m_notifier->notifyMediaCreation( task.media );
    return parser::Task::Status::Success;
}

/* Playlist files */

bool MetadataParser::addPlaylistMedias( parser::Task& task, int nbSubitem ) const
{
    auto t = m_ml->getConn()->newTransaction();
    LOG_INFO( "Try to import ", task.mrl, " as a playlist" );
    auto playlistName = task.vlcMedia.meta( libvlc_meta_Title );
    if ( playlistName.empty() == true )
        playlistName = utils::url::decode( utils::file::fileName( task.mrl ) );
    auto playlistPtr = Playlist::create( m_ml, playlistName );
    if ( playlistPtr == nullptr )
    {
        LOG_ERROR( "Failed to create playlist ", task.mrl, " to the media library" );
        return false;
    }
    task.file = playlistPtr->addFile( *task.fileFs, task.parentFolder->id(), task.parentFolderFs->device()->isRemovable() );
    if ( task.file == nullptr )
    {
        LOG_ERROR( "Failed to add playlist file ", task.mrl );
        return false;
    }
    t->commit();
    auto subitems = task.vlcMedia.subitems();
    for ( int i = 0; i < nbSubitem; ++i ) // FIXME: Interrupt loop if paused
        addPlaylistElement( task, playlistPtr, subitems->itemAtIndex( i ), static_cast<unsigned int>( i ) + 1 );

    return true;
}

void MetadataParser::addPlaylistElement( parser::Task& task, const std::shared_ptr<Playlist>& playlistPtr,
                                         VLC::MediaPtr subitem, unsigned int index ) const
{
    if ( subitem == nullptr )
        return;
    const auto& mrl = subitem->mrl();
    LOG_INFO( "Try to add ", mrl, " to the playlist ", task.mrl );
    auto media = m_ml->media( mrl );
    if ( media != nullptr )
    {
        LOG_INFO( "Media for ", mrl, " already exists, adding it to the playlist ", task.mrl );
        playlistPtr->add( media->id(), index );
        return;
    }
    // Create Media, etc.
    auto fsFactory = m_ml->fsFactoryForMrl( mrl );

    if ( fsFactory == nullptr ) // Media not supported by any FsFactory, registering it as external
    {
        auto t2 = m_ml->getConn()->newTransaction();
        auto externalMedia = Media::create( m_ml, IMedia::Type::Unknown, utils::url::encode(
                subitem->meta( libvlc_meta_Title ) ) );
        if ( externalMedia == nullptr )
        {
            LOG_ERROR( "Failed to create external media for ", mrl, " in the playlist ", task.mrl );
            return;
        }
        // Assuming that external mrl present in playlist file is a main media resource
        auto externalFile = externalMedia->addExternalMrl( mrl, IFile::Type::Main );
        if ( externalFile == nullptr )
            LOG_ERROR( "Failed to create external file for ", mrl, " in the playlist ", task.mrl );
        playlistPtr->add( externalMedia->id(), index );
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
        LOG_ERROR( ex.what() );
        return;
    }
    LOG_INFO( "Importing ", isDirectory ? "folder " : "file ", mrl, " in the playlist ", task.mrl );
    auto directoryMrl = utils::file::directory( mrl );
    auto parentFolder = Folder::fromMrl( m_ml, directoryMrl );
    bool parentKnown = parentFolder != nullptr;

    auto entryPoint = utils::file::scheme( mrl ) + '/' + utils::file::firstFolder( utils::file::stripScheme( mrl ) );
    if ( parentKnown == false && Folder::fromMrl( m_ml, entryPoint ) != nullptr )
    {
        auto probePtr = std::unique_ptr<prober::PathProbe>( new prober::PathProbe{ utils::file::stripScheme( mrl ),
                                                                                   isDirectory, playlistPtr, parentFolder,
                                                                                   utils::file::stripScheme( directoryMrl ), index, true } );
        auto discoverer = FsDiscoverer( fsFactory, m_ml, nullptr, std::move( probePtr ) );
        discoverer.reload( entryPoint );
        return;
    }
    auto probePtr = std::unique_ptr<prober::PathProbe>( new prober::PathProbe{ utils::file::stripScheme( mrl ),
                                                                               isDirectory, playlistPtr, parentFolder,
                                                                               utils::file::stripScheme( directoryMrl ), index, false } );
    auto discoverer = FsDiscoverer( fsFactory, m_ml, nullptr, std::move( probePtr ) );
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

bool MetadataParser::parseVideoFile( parser::Task& task ) const
{
    auto media = task.media.get();
    media->setType( IMedia::Type::Video );
    const auto& title = task.vlcMedia.meta( libvlc_meta_Title );
    if ( title.length() == 0 )
        return true;

    const auto& showName = task.vlcMedia.meta( libvlc_meta_ShowName );
    if ( showName.length() == 0 )
    {
        return sqlite::Tools::withRetries( 3, [this, &showName, &title, &task]() {
            auto t = m_ml->getConn()->newTransaction();
            task.media->setTitleBuffered( title );

            auto show = m_ml->show( showName );
            if ( show == nullptr )
            {
                show = m_ml->createShow( showName );
                if ( show == nullptr )
                    return false;
            }
            auto episode = toInt( task.vlcMedia, libvlc_meta_Episode, "episode number" );
            if ( episode != 0 )
            {
                std::shared_ptr<Show> s = std::static_pointer_cast<Show>( show );
                s->addEpisode( *task.media, title, episode );
            }
            task.media->save();
            t->commit();
            return true;
        });
    }
    else
    {
        // How do we know if it's a movie or a random video?
    }
    return true;
}

/* Audio files */

bool MetadataParser::parseAudioFile( parser::Task& task )
{
    task.media->setType( IMedia::Type::Audio );

    auto artworkMrl = task.vlcMedia.meta( libvlc_meta_ArtworkURL );
    if ( artworkMrl.empty() == false )
    {
        task.media->setThumbnail( artworkMrl );
        // Don't use an attachment as default artwork for album/artists
        if ( utils::file::schemeIs( "attachment", artworkMrl ) )
            artworkMrl.clear();
    }


    auto genre = handleGenre( task );
    auto artists = findOrCreateArtist( task );
    if ( artists.first == nullptr && artists.second == nullptr )
        return false;
    auto album = findAlbum( task, artists.first, artists.second );
    return sqlite::Tools::withRetries( 3, [this, &task, &artists]( std::string artworkMrl,
                                                  std::shared_ptr<Album> album, std::shared_ptr<Genre> genre ) {
        auto t = m_ml->getConn()->newTransaction();
        if ( album == nullptr )
        {
            const auto& albumName = task.vlcMedia.meta( libvlc_meta_Album );
            album = m_ml->createAlbum( albumName, artworkMrl );
            if ( album == nullptr )
                return false;
            m_notifier->notifyAlbumCreation( album );
        }
        // If we know a track artist, specify it, otherwise, fallback to the album/unknown artist
        auto track = handleTrack( album, task, artists.second ? artists.second : artists.first,
                                  genre.get() );

        auto res = link( *task.media, album, artists.first, artists.second );
        task.media->save();
        t->commit();
        return res;
    }, std::move( artworkMrl ), std::move( album ), std::move( genre ) );
}

std::shared_ptr<Genre> MetadataParser::handleGenre( parser::Task& task ) const
{
    const auto& genreStr = task.vlcMedia.meta( libvlc_meta_Genre );
    if ( genreStr.length() == 0 )
        return nullptr;
    auto genre = Genre::fromName( m_ml, genreStr );
    if ( genre == nullptr )
    {
        genre = Genre::create( m_ml, genreStr );
        if ( genre == nullptr )
            LOG_ERROR( "Failed to get/create Genre", genreStr );
    }
    return genre;
}

/* Album handling */

std::shared_ptr<Album> MetadataParser::findAlbum( parser::Task& task, std::shared_ptr<Artist> albumArtist,
                                                    std::shared_ptr<Artist> trackArtist )
{
    const auto& albumName = task.vlcMedia.meta( libvlc_meta_Album );
    if ( albumName.empty() == true )
    {
        std::shared_ptr<Artist> artist = albumArtist;
        if ( albumArtist != nullptr )
            return albumArtist->unknownAlbum();
        else if ( trackArtist != nullptr )
            return trackArtist->unknownAlbum();
        return m_unknownArtist->unknownAlbum();
    }

    if ( m_previousAlbum != nullptr && albumName == m_previousAlbum->title() &&
         m_previousFolderId != 0 && task.file->folderId() == m_previousFolderId )
        return m_previousAlbum;
    m_previousAlbum.reset();
    m_previousFolderId = 0;

    // Album matching depends on the difference between artist & album artist.
    // Specificaly pass the albumArtist here.
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
            " WHERE title = ?";
    auto albums = Album::fetchAll<Album>( m_ml, req, albumName );

    if ( albums.size() == 0 )
        return nullptr;

    const auto discTotal = toInt( task.vlcMedia, libvlc_meta_DiscTotal, "disc total" );
    const auto discNumber = toInt( task.vlcMedia, libvlc_meta_DiscNumber, "disc number" );
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
        if ( albumArtist != nullptr )
        {
            // We assume that an album without album artist is a positive match.
            // At the end of the day, without proper tags, there's only so much we can do.
            auto candidateAlbumArtist = a->albumArtist();
            if ( candidateAlbumArtist != nullptr && candidateAlbumArtist->id() != albumArtist->id() )
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
        for ( auto& t : tracks )
        {
            auto at = t->albumTrack();
            assert( at != nullptr );
            if ( at != nullptr && at->discNumber() > 1 )
            {
                multiDisc = true;
                break;
            }
        }
        if ( multiDisc )
        {
            ++it;
            continue;
        }

        // Assume album files will be in the same folder.
        auto newFileFolder = utils::file::directory( task.file->mrl() );
        auto trackFiles = tracks[0]->files();
        bool excluded = false;
        for ( auto& f : trackFiles )
        {
            auto candidateFolder = utils::file::directory( f->mrl() );
            if ( candidateFolder != newFileFolder )
            {
                excluded = true;
                break;
            }
        }
        if ( excluded == true )
        {
            it = albums.erase( it );
            continue;
        }
        ++it;
    }
    if ( albums.size() == 0 )
        return nullptr;
    if ( albums.size() > 1 )
    {
        LOG_WARN( "Multiple candidates for album ", albumName, ". Selecting first one out of luck" );
    }
    m_previousFolderId = task.file->folderId();
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
std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> MetadataParser::findOrCreateArtist( parser::Task& task ) const
{
    std::shared_ptr<Artist> albumArtist;
    std::shared_ptr<Artist> artist;
    static const std::string req = "SELECT * FROM " + policy::ArtistTable::Name + " WHERE name = ?";

    const auto& albumArtistStr = task.vlcMedia.meta( libvlc_meta_AlbumArtist );
    const auto& artistStr = task.vlcMedia.meta( libvlc_meta_Artist );
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
            m_notifier->notifyArtistCreation( albumArtist );
        }
    }
    return {albumArtist, artist};
}

/* Tracks handling */

std::shared_ptr<AlbumTrack> MetadataParser::handleTrack( std::shared_ptr<Album> album, parser::Task& task,
                                                         std::shared_ptr<Artist> artist, Genre* genre ) const
{
    auto title = task.vlcMedia.meta( libvlc_meta_Title );
    const auto trackNumber = toInt( task.vlcMedia, libvlc_meta_TrackNumber, "track number" );
    const auto discNumber = toInt( task.vlcMedia, libvlc_meta_DiscNumber, "disc number" );
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
        task.media->setTitleBuffered( title );

    auto track = std::static_pointer_cast<AlbumTrack>( album->addTrack( task.media, trackNumber,
                                                                        discNumber, artist->id(),
                                                                        genre ) );
    if ( track == nullptr )
    {
        LOG_ERROR( "Failed to create album track" );
        return nullptr;
    }

    const auto& releaseDate = task.vlcMedia.meta( libvlc_meta_Date );
    if ( releaseDate.empty() == false )
    {
        auto releaseYear = atoi( releaseDate.c_str() );
        task.media->setReleaseDate( releaseYear );
        // Let the album handle multiple dates. In order to do this properly, we need
        // to know if the date has been changed before, which can be known only by
        // using Album class internals.
        album->setReleaseYear( releaseYear, false );
    }
    m_notifier->notifyAlbumTrackCreation( track );
    return track;
}

/* Misc */

bool MetadataParser::link( Media& media, std::shared_ptr<Album> album,
                               std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist )
{
    if ( albumArtist == nullptr )
    {
        assert( artist != nullptr );
        albumArtist = artist;
    }

    // We might modify albumArtist later, hence handle thumbnails before.
    // If we have an albumArtist (meaning the track was properly tagged, we
    // can assume this artist is a correct match. We can use the thumbnail from
    // the current album for the albumArtist, if none has been set before.
    if ( albumArtist != nullptr && albumArtist->artworkMrl().empty() == true &&
         album != nullptr && album->artworkMrl().empty() == false )
        albumArtist->setArtworkMrl( album->artworkMrl() );

    if ( albumArtist != nullptr )
        albumArtist->addMedia( media );
    if ( artist != nullptr && ( albumArtist == nullptr || albumArtist->id() != artist->id() ) )
        artist->addMedia( media );

    auto currentAlbumArtist = album->albumArtist();

    // If we have no main artist yet, that's easy, we need to assign one.
    if ( currentAlbumArtist == nullptr )
    {
        // We don't know if the artist was tagged as artist or albumartist, however, we simply add it
        // as the albumartist until proven we were wrong (ie. until one of the next tracks
        // has a different artist)
        album->setAlbumArtist( albumArtist );
        // Always add the album artist as an artist
        album->addArtist( albumArtist );
        if ( artist != nullptr )
            album->addArtist( artist );
    }
    else
    {
        if ( albumArtist->id() != currentAlbumArtist->id() )
        {
            // We have more than a single artist on this album, fallback to various artists
            if ( m_variousArtists == nullptr )
                m_variousArtists = Artist::fetch( m_ml, VariousArtistID );
            album->setAlbumArtist( m_variousArtists );
            // Add those two artists as "featuring".
            album->addArtist( albumArtist );
        }
        if ( artist != nullptr && artist->id() != albumArtist->id() )
        {
            if ( albumArtist->id() != artist->id() )
               album->addArtist( artist );
        }
    }

    return true;
}

const char* MetadataParser::name() const
{
    return "Metadata";
}

uint8_t MetadataParser::nbThreads() const
{
//    auto nbProcs = std::thread::hardware_concurrency();
//    if ( nbProcs == 0 )
//        return 1;
//    return nbProcs;
    // Let's make this code thread-safe first :)
    return 1;
}

bool MetadataParser::isCompleted( const parser::Task& task ) const
{
    // We always need to run this task if the metadata extraction isn't completed
    return ( static_cast<uint8_t>( task.step ) &
            static_cast<uint8_t>( parser::Task::ParserStep::MetadataAnalysis ) ) != 0;
}

}
