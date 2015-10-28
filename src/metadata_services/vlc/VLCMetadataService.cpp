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

#include "VLCMetadataService.h"
#include "Media.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "Show.h"

#include "Media.h"

VLCMetadataService::VLCMetadataService(const VLC::Instance& vlc, DBConnection dbConnection )
    : m_instance( vlc )
    , m_cb( nullptr )
    , m_ml( nullptr )
    , m_dbConn( dbConnection )
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

    auto ctx = new Context( file );
    std::unique_ptr<Context> ctxPtr( ctx );

    ctx->media = VLC::Media( m_instance, file->mrl(), VLC::Media::FromPath );

    std::unique_lock<std::mutex> lock( m_mutex );
    std::atomic<Status> status( Status::Unknown );

    ctx->media.eventManager().onParsedChanged([this, ctx, &status](bool parsed) {
        if ( parsed == false )
            return;
        auto s = handleMediaMeta( ctx->file, ctx->media );
        auto expected = Status::Unknown;
        while ( status.compare_exchange_weak( expected, s ) == false )
            ;
        m_cond.notify_all();
    });
    ctx->media.parseAsync();
    auto success = m_cond.wait_for( lock, std::chrono::seconds( 5 ), [&status]() { return status != Status::Unknown; } );
    if ( success == false )
        m_cb->done( ctx->file, Status::Fatal, data );
    else
        m_cb->done( ctx->file, status, data );
}

IMetadataService::Status VLCMetadataService::handleMediaMeta( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const
{
    auto tracks = vlcMedia.tracks();
    if ( tracks.size() == 0 )
    {
        LOG_ERROR( "Failed to fetch tracks" );
        return Status::Fatal;
    }
    bool isAudio = true;
    for ( auto& track : tracks )
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
    if ( media->setDuration( duration ) == false )
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

    auto artistPair = handleArtist( media, vlcMedia );
    if ( artistPair.first == nullptr && artistPair.second == true )
    {
        LOG_WARN( "Failed to create a new artist" );
        return false;
    }
    auto albumPair = handleAlbum( media, vlcMedia );
    if ( albumPair.first == nullptr && albumPair.second == true )
    {
        LOG_WARN( "Failed to create a new album" );
        return false;
    }

    return link( media, albumPair.first, artistPair.first, albumPair.second, artistPair.second );
}

/* Album handling */

std::shared_ptr<Album> VLCMetadataService::findAlbum( const std::string& title, VLC::Media& vlcMedia ) const
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
            " WHERE title = ?";
    auto albums = Album::fetchAll( m_dbConn, req, title );

    if ( albums.size() == 0 )
        return nullptr;

    if ( albums.size() == 1 )
        return std::static_pointer_cast<Album>( albums[0] );

    // FIXME: Attempt to find the proper album
    return std::static_pointer_cast<Album>( albums[0] );
}

std::pair<std::shared_ptr<Album>, bool> VLCMetadataService::handleAlbum( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const
{
    auto albumTitle = vlcMedia.meta( libvlc_meta_Album );
    if ( albumTitle.length() > 0 )
    {
        auto newAlbum = false;
        auto album = findAlbum( albumTitle, vlcMedia );

        if ( album == nullptr )
        {
            album = m_ml->createAlbum( albumTitle );
            if ( album != nullptr )
            {
                newAlbum = true;
                auto date = vlcMedia.meta( libvlc_meta_Date );
                if ( date.length() > 0 )
                    album->setReleaseYear( std::stoul( date ) );
                auto artwork = vlcMedia.meta( libvlc_meta_ArtworkURL );
                if ( artwork.length() != 0 )
                    album->setArtworkUrl( artwork );
            }
        }
        if ( album != nullptr )
        {
            auto track = handleTrack( album, media, vlcMedia );
            if ( track != nullptr )
                media->setAlbumTrack( track );
        }
        return {album, newAlbum};
    }
    return {nullptr, false};
}

/* Artists handling */

std::pair<std::shared_ptr<Artist>, bool> VLCMetadataService::handleArtist( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const
{
    assert(media != nullptr);

    std::shared_ptr<Artist> artist;
    auto newArtist = false;
    auto albumArtistName = vlcMedia.meta( libvlc_meta_AlbumArtist );
    auto artistName = vlcMedia.meta( libvlc_meta_Artist );
    if ( albumArtistName.empty() == true )
        albumArtistName = artistName;

    if ( albumArtistName.length() > 0 )
    {
        artist = std::static_pointer_cast<Artist>( m_ml->artist( albumArtistName ) );
        if ( artist == nullptr )
        {
            artist = m_ml->createArtist( albumArtistName );
            if ( artist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist ", albumArtistName );
                return {nullptr, true};
            }
            newArtist = true;
        }
    }
    else
        return {nullptr, false};

    if ( artistName.length() > 0 )
        media->setArtist( artistName );
    else if ( albumArtistName.length() > 0 )
    {
        // Always provide an artist, to avoid the user from having to fallback
        // to the album artist by himself
        media->setArtist( albumArtistName );
    }
    return {artist, newArtist};
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
        trackNb = std::stoi( trackNbStr );
    else
        trackNb = 0;
    auto track = std::static_pointer_cast<AlbumTrack>( album->addTrack( media, trackNb ) );
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
    return track;
}

/* Misc */

bool VLCMetadataService::link(std::shared_ptr<Media> media, std::shared_ptr<Album> album, std::shared_ptr<Artist> artist, bool newAlbum, bool newArtist) const
{
    // If this is either a new album or a new artist, we need to add the relationship between the two.
    if ( album != nullptr && artist != nullptr && ( newAlbum == true || newArtist == true ) )
    {
        if ( album->addArtist( artist ) == false )
            LOG_WARN( "Failed to add artist ", artist->name(), " to album ", album->title() );
        // This is the first time we have both artist & album, use this opportunity to set an artist artwork.
        if ( artist->artworkUrl().empty() == true )
            artist->setArtworkUrl( album->artworkUrl() );
    }
    if ( artist != nullptr )
        artist->addMedia( media.get() );
    else
        std::static_pointer_cast<Artist>( m_ml->unknownArtist() )->addMedia( media.get() );
    return true;
}
