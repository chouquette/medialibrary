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
#include "Media.h"
#include "Show.h"
#include "utils/Filename.h"

MetadataParser::MetadataParser( DBConnection dbConnection, std::shared_ptr<factory::IFileSystem> fsFactory)
    : m_dbConn( dbConnection )
    , m_fsFactory( fsFactory )
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
    auto& vlcMedia = task.vlcMedia;
    auto& file = task.file;

    const auto tracks = vlcMedia.tracks();
    if ( tracks.size() == 0 )
    {
        LOG_ERROR( "Failed to fetch tracks" );
        return parser::Task::Status::Fatal;
    }

    auto t = m_dbConn->newTransaction();
    bool isAudio = true;
    for ( const auto& track : tracks )
    {
        auto codec = track.codec();
        std::string fcc( (const char*)&codec, 4 );
        if ( track.type() == VLC::MediaTrack::Video )
        {
            isAudio = false;
            auto fps = (float)track.fpsNum() / (float)track.fpsDen();
            media->addVideoTrack( fcc, track.width(), track.height(), fps );
        }
        else if ( track.type() == VLC::MediaTrack::Audio )
        {
            media->addAudioTrack( fcc, track.bitrate(), track.rate(), track.channels(),
                                  track.language(), track.description() );
        }
    }
    t->commit();
    if ( isAudio == true )
    {
        if ( parseAudioFile( *media, *file, vlcMedia ) == false )
            return parser::Task::Status::Fatal;
    }
    else
    {
        if (parseVideoFile( media, vlcMedia ) == false )
            return parser::Task::Status::Fatal;
    }
    auto duration = vlcMedia.duration();
    media->setDuration( duration );
    if ( media->save() == false )
        return parser::Task::Status::Error;
    return parser::Task::Status::Success;
}

/* Video files */

bool MetadataParser::parseVideoFile( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const
{
    media->setType( IMedia::Type::VideoType );
    auto title = vlcMedia.meta( libvlc_meta_Title );
    if ( title.length() == 0 )
        return true;
    auto showName = vlcMedia.meta( libvlc_meta_ShowName );
    if ( showName.length() == 0 )
    {
        auto show = m_ml->show( showName );
        if ( show == nullptr )
        {
            show = m_ml->createShow( showName );
            if ( show == nullptr )
                return false;
        }

        auto episodeIdStr = vlcMedia.meta( libvlc_meta_Episode );
        if ( episodeIdStr.length() > 0 )
        {
            size_t endpos;
            int episodeId = std::stoi( episodeIdStr, &endpos );
            if ( endpos != episodeIdStr.length() )
            {
                LOG_ERROR( "Invalid episode id provided" );
                return true;
            }
            std::shared_ptr<Show> s = std::static_pointer_cast<Show>( show );
            s->addEpisode( *media, title, episodeId );
        }
    }
    else
    {
        // How do we know if it's a movie or a random video?
    }
    return true;
}

/* Audio files */

bool MetadataParser::parseAudioFile( Media& media, File& file, VLC::Media& vlcMedia ) const
{
    media.setType( IMedia::Type::AudioType );

    auto cover = vlcMedia.meta( libvlc_meta_ArtworkURL );
    if ( cover.empty() == false )
        media.setThumbnail( cover );

    auto artists = handleArtists( vlcMedia );
    auto album = handleAlbum( media, file, vlcMedia, artists.first, artists.second );
    if ( album == nullptr )
    {
        LOG_WARN( "Failed to get/create associated album" );
        return false;
    }
    auto t = m_dbConn->newTransaction();
    auto res = link( media, album, artists.first, artists.second );
    t->commit();
    return res;
}

/* Album handling */
std::shared_ptr<Album> MetadataParser::findAlbum( File& file, VLC::Media& vlcMedia, const std::string& title, Artist* albumArtist ) const
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
            " WHERE title = ?";
    auto albums = Album::fetchAll<Album>( m_dbConn, req, title );

    if ( albums.size() == 0 )
        return nullptr;

    auto discTotalStr = vlcMedia.meta( libvlc_meta_DiscTotal );
    auto discTotal = 0u;
    if ( discTotalStr.empty() == false )
        discTotal = atoi( discTotalStr.c_str() );

    auto discNumberStr = vlcMedia.meta( libvlc_meta_DiscNumber );
    auto discNumber = 0u;
    if ( discNumberStr.empty() == false )
        discNumber = atoi( discNumberStr.c_str() );

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
        if ( discTotal > 1 || discNumber > 1 )
        {
            ++it;
            continue;
        }
        const auto tracks = a->tracks();
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
        auto trackFiles = tracks[0]->files();
        auto newFileFolder = utils::file::directory( file.mrl() );
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
        LOG_WARN( "Multiple candidates for album ", title, ". Selecting first one out of luck" );
    }
    return std::static_pointer_cast<Album>( albums[0] );
}

std::shared_ptr<Album> MetadataParser::handleAlbum( Media& media, File& file, VLC::Media& vlcMedia, std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> trackArtist ) const
{
    auto albumTitle = vlcMedia.meta( libvlc_meta_Album );
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

    if ( albumTitle.length() > 0 )
    {
        // Album matching depends on the difference between artist & album artist.
        // Specificaly pass the albumArtist here.
        album = findAlbum( file, vlcMedia, albumTitle, albumArtist.get() );

        if ( album == nullptr )
        {
            album = m_ml->createAlbum( albumTitle );
            if ( album != nullptr )
            {
                auto artwork = vlcMedia.meta( libvlc_meta_ArtworkURL );
                if ( artwork.length() != 0 )
                    album->setArtworkMrl( artwork );
            }
        }
    }
    else
        album = artist->unknownAlbum();

    if ( album == nullptr )
        return nullptr;
    // If we know a track artist, specify it, otherwise, fallback to the album/unknown artist
    auto track = handleTrack( album, media, vlcMedia, trackArtist ? trackArtist : artist );
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
std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> MetadataParser::handleArtists( VLC::Media& vlcMedia ) const
{
    std::shared_ptr<Artist> albumArtist;
    std::shared_ptr<Artist> artist;
    auto albumArtistName = vlcMedia.meta( libvlc_meta_AlbumArtist );
    auto artistName = vlcMedia.meta( libvlc_meta_Artist );

    if ( albumArtistName.empty() == true && artistName.empty() == true )
    {
        return {nullptr, nullptr};
    }

    if ( albumArtistName.empty() == false )
    {
        albumArtist = std::static_pointer_cast<Artist>( m_ml->artist( albumArtistName ) );
        if ( albumArtist == nullptr )
        {
            albumArtist = m_ml->createArtist( albumArtistName );
            if ( albumArtist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist ", albumArtistName );
                return {nullptr, nullptr};
            }
        }
    }
    if ( artistName.empty() == false && artistName != albumArtistName )
    {
        artist = std::static_pointer_cast<Artist>( m_ml->artist( artistName ) );
        if ( artist == nullptr )
        {
            artist = m_ml->createArtist( artistName );
            if ( artist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist ", artistName );
                return {nullptr, nullptr};
            }
        }
    }
    return {albumArtist, artist};
}

/* Tracks handling */

std::shared_ptr<AlbumTrack> MetadataParser::handleTrack( std::shared_ptr<Album> album, Media& media, VLC::Media& vlcMedia, std::shared_ptr<Artist> artist ) const
{
    auto trackNbStr = vlcMedia.meta( libvlc_meta_TrackNumber );

    auto title = vlcMedia.meta( libvlc_meta_Title );
    if ( title.empty() == true )
    {
        LOG_WARN( "Failed to get track title" );
        if ( trackNbStr.empty() == false )
        {
            title = "Track #";
            title += trackNbStr;
        }
    }
    if ( title.empty() == false )
        media.setTitle( title );
    unsigned int trackNb;
    if ( trackNbStr.empty() == false )
        trackNb = atoi( trackNbStr.c_str() );
    else
        trackNb = 0;

    auto discNumberStr = vlcMedia.meta( libvlc_meta_DiscNumber );
    auto discNumber = 0;
    if ( discNumberStr.empty() == false )
        discNumber = atoi( discNumberStr.c_str() );

    auto track = std::static_pointer_cast<AlbumTrack>( album->addTrack( media, trackNb, discNumber ) );
    if ( track == nullptr )
    {
        LOG_ERROR( "Failed to create album track" );
        return nullptr;
    }
    if ( artist != nullptr )
        track->setArtist( artist );
    auto genre = vlcMedia.meta( libvlc_meta_Genre );
    if ( genre.length() != 0 )
    {
        track->setGenre( genre );
    }
    auto releaseYearStr = vlcMedia.meta( libvlc_meta_Date );
    if ( releaseYearStr.empty() == false )
    {
        auto releaseYear = atoi( releaseYearStr.c_str() );
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
