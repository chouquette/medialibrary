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

#include "MetadataParser.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "File.h"
#include "Genre.h"
#include "Media.h"
#include "Show.h"
#include "utils/Filename.h"

MetadataParser::MetadataParser( DBConnection dbConnection )
    : m_dbConn( dbConnection )
{
}

bool MetadataParser::initialize()
{
    m_ml = mediaLibrary();
    m_unknownArtist = Artist::fetch( m_dbConn, medialibrary::UnknownArtistID );
    if ( m_unknownArtist == nullptr )
        LOG_ERROR( "Failed to cache unknown artist" );
    return m_unknownArtist != nullptr;
}

parser::Task::Status MetadataParser::run( parser::Task& task )
{
    auto& media = task.media;

    auto t = m_dbConn->newTransaction();
    bool isAudio = task.videoTracks.empty();
    for ( const auto& t : task.videoTracks )
    {
        media->addVideoTrack( t.fcc, t.width, t.height, t.fps );
    }
    for ( const auto& t : task.audioTracks )
    {
        media->addAudioTrack( t.fcc, t.bitrate, t.samplerate, t.nbChannels,
                              t.language, t.description );
    }
    t->commit();
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
    auto duration = task.duration;
    media->setDuration( duration );
    if ( media->save() == false )
        return parser::Task::Status::Error;
    return parser::Task::Status::Success;
}

/* Video files */

bool MetadataParser::parseVideoFile( parser::Task& task ) const
{
    auto media = task.media.get();
    media->setType( IMedia::Type::VideoType );
    if ( task.title.length() == 0 )
        return true;

    if ( task.showName.length() == 0 )
    {
        auto show = m_ml->show( task.showName );
        if ( show == nullptr )
        {
            show = m_ml->createShow( task.showName );
            if ( show == nullptr )
                return false;
        }

        if ( task.episode != 0 )
        {
            std::shared_ptr<Show> s = std::static_pointer_cast<Show>( show );
            s->addEpisode( *media, task.title, task.episode );
        }
    }
    else
    {
        // How do we know if it's a movie or a random video?
    }
    return true;
}

/* Audio files */

bool MetadataParser::parseAudioFile( parser::Task& task ) const
{
    task.media->setType( IMedia::Type::AudioType );

    if ( task.artworkMrl.empty() == false )
        task.media->setThumbnail( task.artworkMrl );

    auto artists = handleArtists( task );
    auto album = handleAlbum( task, artists.first, artists.second );
    if ( album == nullptr )
    {
        LOG_WARN( "Failed to get/create associated album" );
        return false;
    }
    auto t = m_dbConn->newTransaction();
    auto res = link( *task.media, album, artists.first, artists.second );
    t->commit();
    return res;
}

/* Album handling */
std::shared_ptr<Album> MetadataParser::findAlbum( parser::Task& task, Artist* albumArtist ) const
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
            " WHERE title = ?";
    auto albums = Album::fetchAll<Album>( m_dbConn, req, task.albumName );

    if ( albums.size() == 0 )
        return nullptr;

    /*
     * Even if we get only 1 album, we need to filter out invalid matches.
     * For instance, if we have already inserted an album "A" by an artist "john"
     * but we are now trying to handle an album "A" by an artist "doe", not filtering
     * candidates would yield the only "A" album we know, while we should return
     * nullptr, so handleAlbum can create a new one.
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
        if ( task.discTotal > 1 || task.discNumber > 1 )
        {
            ++it;
            continue;
        }
        const auto tracks = a->cachedTracks();
        assert( tracks.size() > 0 );

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
        LOG_WARN( "Multiple candidates for album ", task.albumName, ". Selecting first one out of luck" );
    }
    return std::static_pointer_cast<Album>( albums[0] );
}

std::shared_ptr<Album> MetadataParser::handleAlbum( parser::Task& task, std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> trackArtist ) const
{
    std::shared_ptr<Album> album;
    std::shared_ptr<Artist> artist = albumArtist;
    if ( artist == nullptr )
    {
        if ( trackArtist != nullptr )
            artist = trackArtist;
        else
        {
            artist = m_unknownArtist;
        }
    }

    if ( task.albumName.length() > 0 )
    {
        // Album matching depends on the difference between artist & album artist.
        // Specificaly pass the albumArtist here.
        album = findAlbum( task, albumArtist.get() );

        if ( album == nullptr )
        {
            album = m_ml->createAlbum( task.albumName );
            if ( album != nullptr )
            {
                if ( task.artworkMrl.length() != 0 )
                    album->setArtworkMrl( task.artworkMrl );
            }
        }
    }
    else
        album = artist->unknownAlbum();

    if ( album == nullptr )
        return nullptr;
    // If we know a track artist, specify it, otherwise, fallback to the album/unknown artist
    auto track = handleTrack( album, task, trackArtist ? trackArtist : artist );
    return album;
}

///
/// \brief MetadataParser::handleArtists Returns Artist's involved on a track
/// \param media The track to analyze
/// \param vlcMedia VLC's media
/// \return A pair containing:
/// The album artist as a first element
/// The track artist as a second element, or nullptr if it is the same as album artist
///
std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> MetadataParser::handleArtists( parser::Task& task ) const
{
    std::shared_ptr<Artist> albumArtist;
    std::shared_ptr<Artist> artist;

    if ( task.albumArtist.empty() == true && task.artist.empty() == true )
    {
        return {nullptr, nullptr};
    }

    if ( task.albumArtist.empty() == false )
    {
        albumArtist = std::static_pointer_cast<Artist>( m_ml->artist( task.albumArtist ) );
        if ( albumArtist == nullptr )
        {
            albumArtist = m_ml->createArtist( task.albumArtist );
            if ( albumArtist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist ", task.albumArtist );
                return {nullptr, nullptr};
            }
        }
    }
    if ( task.artist.empty() == false && task.artist != task.albumArtist )
    {
        artist = std::static_pointer_cast<Artist>( m_ml->artist( task.artist ) );
        if ( artist == nullptr )
        {
            artist = m_ml->createArtist( task.artist );
            if ( artist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist ", task.artist );
                return {nullptr, nullptr};
            }
        }
    }
    return {albumArtist, artist};
}

/* Tracks handling */

std::shared_ptr<AlbumTrack> MetadataParser::handleTrack( std::shared_ptr<Album> album, parser::Task& task, std::shared_ptr<Artist> artist ) const
{
    auto title = task.title;
    if ( title.empty() == true )
    {
        LOG_WARN( "Failed to get track title" );
        if ( task.trackNumber != 0 )
        {
            title = "Track #";
            title += std::to_string( task.trackNumber );
        }
    }
    if ( title.empty() == false )
        task.media->setTitle( title );

    auto track = std::static_pointer_cast<AlbumTrack>( album->addTrack( task.media, task.trackNumber, task.discNumber ) );
    if ( track == nullptr )
    {
        LOG_ERROR( "Failed to create album track" );
        return nullptr;
    }
    if ( artist != nullptr )
        track->setArtist( artist );

    if ( task.genre.length() != 0 )
    {
        auto genre = Genre::fromName( m_dbConn, task.genre );
        if ( genre == nullptr )
        {
            genre = Genre::create( m_dbConn, task.genre );
            if ( genre == nullptr )
            {
                LOG_ERROR( "Failed to create a genre in database" );
                return nullptr;
            }
        }
        track->setGenre( genre );
    }
    if ( task.releaseDate.empty() == false )
    {
        auto releaseYear = atoi( task.releaseDate.c_str() );
        track->setReleaseYear( releaseYear );
        // Let the album handle multiple dates. In order to do this properly, we need
        // to know if the date has been changed before, which can be known only by
        // using Album class internals.
        album->setReleaseYear( releaseYear, false );
    }
    return track;
}

/* Misc */

bool MetadataParser::link( Media& media, std::shared_ptr<Album> album,
                               std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist ) const
{
    if ( albumArtist == nullptr && artist == nullptr )
        albumArtist = m_unknownArtist;
    else
    {
        if ( albumArtist == nullptr )
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
        album->setAlbumArtist( albumArtist.get() );
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
            auto variousArtists = Artist::fetch( m_dbConn, medialibrary::VariousArtistID );
            album->setAlbumArtist( variousArtists.get() );
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
