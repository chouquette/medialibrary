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

#include <chrono>

#include "VLCMetadataService.h"
#include "Media.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "Show.h"
#include "utils/Filename.h"

#include "Media.h"

VLCMetadataService::VLCMetadataService(const VLC::Instance& vlc, DBConnection dbConnection, std::shared_ptr<factory::IFileSystem> fsFactory )
    : m_instance( vlc )
    , m_cb( nullptr )
    , m_ml( nullptr )
    , m_dbConn( dbConnection )
    , m_fsFactory( fsFactory )
{
}

bool VLCMetadataService::initialize(IMetadataServiceCb* callback, MediaLibrary* ml )
{
    m_cb = callback;
    m_ml = ml;
    return true;
}

unsigned int VLCMetadataService::priority() const
{
    return 100;
}

void VLCMetadataService::run( std::shared_ptr<Media> file, void* data )
{
    if ( file->duration() != -1 )
    {
        LOG_INFO( file->mrl(), " was already parsed" );
        m_cb->done( file, Status::Success, data );
        return;
    }
    LOG_INFO( "Parsing ", file->mrl() );
    auto chrono = std::chrono::steady_clock::now();

    auto ctx = new Context( file );
    std::unique_ptr<Context> ctxPtr( ctx );

    ctx->media = VLC::Media( m_instance, file->mrl(), VLC::Media::FromPath );

    std::unique_lock<std::mutex> lock( m_mutex );
    bool done = false;

    ctx->media.eventManager().onParsedChanged([this, ctx, &done, &chrono](bool parsed) {
        if ( parsed == false )
            return;
        auto duration = std::chrono::steady_clock::now() - chrono;
        LOG_DEBUG("VLC parsing done in ", std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
        std::lock_guard<std::mutex> lock( m_mutex );
        done = true;
        m_cond.notify_all();
    });
    ctx->media.parseAsync();
    auto success = m_cond.wait_for( lock, std::chrono::seconds( 5 ), [&done]() { return done == true; } );
    if ( success == false )
        m_cb->done( ctx->file, Status::Fatal, data );
    else
    {
        auto status = handleMediaMeta( ctx->file, ctx->media );
        m_cb->done( ctx->file, status, data );
    }
    auto duration = std::chrono::steady_clock::now() - chrono;
    LOG_DEBUG( "Parsed ", file->mrl(), " in ", std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count(), "ms" );
}

IMetadataService::Status VLCMetadataService::handleMediaMeta( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const
{
    const auto tracks = vlcMedia.tracks();
    if ( tracks.size() == 0 )
    {
        LOG_ERROR( "Failed to fetch tracks" );
        return Status::Fatal;
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
        if ( parseAudioFile( media, vlcMedia ) == false )
            return Status::Fatal;
    }
    else
    {
        if (parseVideoFile( media, vlcMedia ) == false )
            return Status::Fatal;
    }
    auto duration = vlcMedia.duration();
    media->setDuration( duration );
    if ( media->save() == false )
        return Status::Error;
    return Status::Success;
}

/* Video files */

bool VLCMetadataService::parseVideoFile( std::shared_ptr<Media> file, VLC::Media& media ) const
{
    file->setType( IMedia::Type::VideoType );
    auto title = media.meta( libvlc_meta_Title );
    if ( title.length() == 0 )
        return true;
    auto showName = media.meta( libvlc_meta_ShowName );
    if ( showName.length() == 0 )
    {
        auto show = m_ml->show( showName );
        if ( show == nullptr )
        {
            show = m_ml->createShow( showName );
            if ( show == nullptr )
                return false;
        }

        auto episodeIdStr = media.meta( libvlc_meta_Episode );
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
            s->addEpisode( title, episodeId );
        }
    }
    else
    {
        // How do we know if it's a movie or a random video?
    }
    return true;
}

/* Audio files */

bool VLCMetadataService::parseAudioFile( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const
{
    media->setType( IMedia::Type::AudioType );

    auto cover = vlcMedia.meta( libvlc_meta_ArtworkURL );
    if ( cover.empty() == false )
        media->setSnapshot( cover );

    auto artists = handleArtists( media, vlcMedia );
    auto album = handleAlbum( media, vlcMedia, artists.first.get(), artists.second.get() );
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
std::shared_ptr<Album> VLCMetadataService::findAlbum( Media* media, VLC::Media& vlcMedia, const std::string& title, Artist* albumArtist ) const
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
        auto candidateFolder = utils::file::directory( tracks[0]->mrl() );
        auto newFileFolder = utils::file::directory( media->mrl() );
        if ( candidateFolder != newFileFolder )
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

std::shared_ptr<Album> VLCMetadataService::handleAlbum( std::shared_ptr<Media> media, VLC::Media& vlcMedia, Artist* albumArtist, Artist* artist ) const
{
    auto albumTitle = vlcMedia.meta( libvlc_meta_Album );
    std::shared_ptr<Album> album;
    if ( albumTitle.length() > 0 )
    {
        album = findAlbum( media.get(), vlcMedia, albumTitle, albumArtist );

        if ( album == nullptr )
        {
            album = m_ml->createAlbum( albumTitle );
            if ( album != nullptr )
            {
                auto artwork = vlcMedia.meta( libvlc_meta_ArtworkURL );
                if ( artwork.length() != 0 )
                    album->setArtworkUrl( artwork );
            }
        }
    }
    else
    {
        if ( albumArtist != nullptr )
            album = albumArtist->unknownAlbum();
        else if ( artist != nullptr )
            album = artist->unknownAlbum();
        else
        {
            //FIXME: We might fetch the unknown artist twice while parsing the same file, that sucks.
            auto unknownArtist = Artist::fetch( m_dbConn, medialibrary::UnknownArtistID );
            album = unknownArtist->unknownAlbum();
        }
    }
    if ( album == nullptr )
        return nullptr;
    auto track = handleTrack( album, media, vlcMedia );
    if ( track != nullptr )
        media->setAlbumTrack( track );
    return album;
}

/* Artists handling */

std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> VLCMetadataService::handleArtists( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const
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
    if ( artistName.empty() == false )
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

    if ( artistName.length() > 0 )
        media->setArtist( artistName );
    else if ( albumArtistName.length() > 0 )
    {
        // Always provide an artist, to avoid the user from having to fallback
        // to the album artist by himself
        media->setArtist( albumArtistName );
    }
    return {albumArtist, artist};
}

/* Tracks handling */

std::shared_ptr<AlbumTrack> VLCMetadataService::handleTrack(std::shared_ptr<Album> album, std::shared_ptr<Media> media, VLC::Media& vlcMedia) const
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
        media->setTitle( title );
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

bool VLCMetadataService::link( std::shared_ptr<Media> media, std::shared_ptr<Album> album,
                               std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist ) const
{
    if ( albumArtist == nullptr && artist == nullptr )
    {
        albumArtist = Artist::fetch( m_dbConn, medialibrary::UnknownArtistID );
    }

    // We might modify albumArtist later, hence handle snapshots before.
    // If we have an albumArtist (meaning the track was properly tagged, we
    // can assume this artist is a correct match. We can use the thumbnail from
    // the current album for the albumArtist, if none has been set before.
    if ( albumArtist != nullptr && albumArtist->artworkUrl().empty() == true && album != nullptr )
        albumArtist->setArtworkUrl( album->artworkUrl() );

    if ( albumArtist != nullptr )
        albumArtist->addMedia( media.get() );
    if ( artist != nullptr && ( albumArtist == nullptr || albumArtist->id() != artist->id() ) )
        artist->addMedia( media.get() );

    auto currentAlbumArtist = album->albumArtist();

    // If we have no main artist yet, that's easy, we need to assign one.
    if ( currentAlbumArtist == nullptr )
    {
        // If the track was properly tagged, that's even better.
        if ( albumArtist != nullptr )
        {
            album->setAlbumArtist( albumArtist.get() );
            // Always add the album artist as an artist
            album->addArtist( albumArtist );
            if ( artist != nullptr )
                album->addArtist( artist );
        }
        // If however we only have an artist, as opposed to an album artist, we
        // add it, until proven we were wrong (ie. until one of the next tracks has a different artist)
        else
        {
            // Guaranteed to be true since we need at least an artist to enter here.
            assert( artist != nullptr );
            album->setAlbumArtist( artist.get() );
        }
    }
    else
    {
        if ( albumArtist == nullptr )
        {
            // Fallback to artist, since the same logic would apply anyway
            albumArtist = artist;
        }
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
