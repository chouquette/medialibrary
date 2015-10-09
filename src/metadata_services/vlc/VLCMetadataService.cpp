#include <iostream>

//debug
#include <chrono>
#include <thread>

#include "VLCMetadataService.h"
#include "IFile.h"
#include "IAlbum.h"
#include "IAlbumTrack.h"
#include "IShow.h"

#include "File.h"

VLCMetadataService::VLCMetadataService( const VLC::Instance& vlc )
    : m_instance( vlc )
    , m_cb( nullptr )
    , m_ml( nullptr )
{
}

bool VLCMetadataService::initialize( IMetadataServiceCb* callback, IMediaLibrary* ml )
{
    m_cb = callback;
    m_ml = ml;
    return true;
}

unsigned int VLCMetadataService::priority() const
{
    return 100;
}

bool VLCMetadataService::run( FilePtr file, void* data )
{
    LOG_INFO( "Parsing ", file->mrl() );

    auto ctx = new Context( file );
    ctx->media = VLC::Media( m_instance, file->mrl(), VLC::Media::FromPath );

    std::unique_lock<std::mutex> lock( m_mutex );
    bool parsed;

    ctx->media.eventManager().onParsedChanged([this, ctx, data, &parsed](bool status) mutable {
        if ( status == false )
            return;
        auto res = handleMediaMeta( ctx->file, ctx->media );
        m_cb->done( ctx->file, res, data );

        std::unique_lock<std::mutex> lock( m_mutex );
        parsed = true;
        m_cond.notify_all();
    });
    ctx->media.parseAsync();
    m_cond.wait( lock, [&parsed]() { return parsed == true; } );
    return true;
}

ServiceStatus VLCMetadataService::handleMediaMeta( FilePtr file, VLC::Media& media ) const
{
    auto tracks = media.tracks();
    if ( tracks.size() == 0 )
    {
        LOG_ERROR( "Failed to fetch tracks" );
        return StatusFatal;
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
            file->addVideoTrack( fcc, track.width(), track.height(), fps );
        }
        else if ( track.type() == VLC::MediaTrack::Audio )
        {
            file->addAudioTrack( fcc, track.bitrate(), track.rate(),
                                      track.channels() );
        }
    }
    auto duration = media.duration();
    file->setDuration( duration );
    if ( isAudio == true )
    {
        if ( parseAudioFile( file, media ) == false )
            return StatusFatal;
    }
    else
    {
        if (parseVideoFile( file, media ) == false )
            return StatusFatal;
    }
    return StatusSuccess;
}

bool VLCMetadataService::parseAudioFile( FilePtr file, VLC::Media& media ) const
{
    file->setType( IFile::Type::AudioType );
    auto albumTitle = media.meta( libvlc_meta_Album );
    if ( albumTitle.length() == 0 )
        return true;
    auto album = m_ml->album( albumTitle );
    if ( album == nullptr )
    {
        album = m_ml->createAlbum( albumTitle );
        if ( album != nullptr )
        {
            auto date = media.meta( libvlc_meta_Date );
            if ( date.length() > 0 )
                album->setReleaseDate( std::stoul( date ) );
        }
    }
    if ( album == nullptr )
    {
        LOG_ERROR( "Failed to create/get album" );
        return false;
    }

    auto trackNbStr = media.meta( libvlc_meta_TrackNumber );
    if ( trackNbStr.length() == 0 )
    {
        LOG_ERROR( "Failed to get track id" );
        return false;
    }
    auto artwork = media.meta( libvlc_meta_ArtworkURL );
    if ( artwork.length() != 0 )
        album->setArtworkUrl( artwork );

    auto title = media.meta( libvlc_meta_Title );
    if ( title.length() == 0 )
    {
        LOG_ERROR( "Failed to compute track title" );
        title = "Unknown track #";
        title += trackNbStr;
    }
    unsigned int trackNb = std::stoi( trackNbStr );
    auto track = album->addTrack( title, trackNb );
    if ( track == nullptr )
    {
        LOG_ERROR( "Failure while creating album track" );
        return false;
    }
    file->setAlbumTrack( track );

    auto genre = media.meta( libvlc_meta_Genre );
    if ( genre.length() != 0 )
        track->setGenre( genre );

    auto artistName = media.meta( libvlc_meta_Artist );
    if ( artistName.length() != 0 )
    {
        auto artist = m_ml->artist( artistName );
        if ( artist == nullptr )
        {
            artist = m_ml->createArtist( artistName );
            if ( artist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist ", artistName );
                // Consider this a minor failure and go on nevertheless
                return true;
            }
        }
        track->addArtist( artist );
    }
    return true;
}

bool VLCMetadataService::parseVideoFile( FilePtr file, VLC::Media& media ) const
{
    file->setType( IFile::Type::VideoType );
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
            show->addEpisode( title, episodeId );
        }
    }
    else
    {
        // How do we know if it's a movie or a random video?
    }
    return true;
}
