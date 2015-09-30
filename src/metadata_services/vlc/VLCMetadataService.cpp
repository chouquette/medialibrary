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

VLCMetadataService::~VLCMetadataService()
{
    cleanup();
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
    cleanup();

    LOG_INFO( "Parsing ", file->mrl() );

    auto ctx = new Context( file );
    ctx->media = VLC::Media( m_instance, file->mrl(), VLC::Media::FromPath );

    ctx->media.eventManager().onParsedChanged([this, ctx, data](bool status) mutable {
        if ( status == false )
            return;
        auto res = handleMediaMeta( ctx->file, ctx->media );
        m_cb->done( ctx->file, res, data );
        // Delegate cleanup for a later time.
        // Context owns a media, which owns an EventManager, which owns this lambda
        // If we were to delete the context from here, the Media refcount would reach 0,
        // thus deleting the media while in use.
        // If we use a smart_ptr, then we get cyclic ownership:
        // ctx -> media -> eventmanager -> event lambda -> ctx
        // leading resources to never be released.
        std::unique_lock<std::mutex> lock( m_cleanupLock );
        m_cleanup.push_back( ctx );
    });
    ctx->media.parseAsync();
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
        album = m_ml->createAlbum( albumTitle );
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

    auto artist = media.meta( libvlc_meta_Artist );
    if ( artist.length() != 0 )
        track->setArtist( artist );
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

void VLCMetadataService::cleanup()
{
    std::unique_lock<std::mutex> lock( m_cleanupLock );
    for ( auto ctx : m_cleanup )
        delete ctx;
    m_cleanup.clear();
}
