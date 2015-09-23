#include <iostream>

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
    // Keeping this old comment for future reference:
    // We can't cleanup from the callback since it's called from a VLC thread.
    // If the refcounter was to reach 0 from there, it would destroy resources
    // that are still held.
    // ------------------------

    VLC::Media media( m_instance, file->mrl(), VLC::Media::FromPath );
    media.eventManager().onParsedChanged([this, file, media, data](bool status) mutable {
        //FIXME: This is probably leaking due to cross referencing
        if ( status == false )
            return;
        auto res = handleMediaMeta( file, media );
        m_cb->done( file, res, data );
    });
    media.parseAsync();
    return true;
}

ServiceStatus VLCMetadataService::handleMediaMeta( FilePtr file, VLC::Media& media ) const
{
    auto tracks = media.tracks();
    if ( tracks.size() == 0 )
    {
        std::cerr << "Failed to fetch tracks" << std::endl;
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
    auto f = std::static_pointer_cast<File>( file );
    f->setReady();
    return StatusSuccess;
}

bool VLCMetadataService::parseAudioFile( FilePtr file, VLC::Media& media ) const
{
    auto albumTitle = media.meta( libvlc_meta_Album );
    if ( albumTitle.length() == 0 )
        return true;
    auto album = m_ml->album( albumTitle );
    if ( album == nullptr )
        album = m_ml->createAlbum( albumTitle );
    if ( album == nullptr )
    {
        std::cerr << "Failed to create/get album" << std::endl;
        return false;
    }

    auto trackNbStr = media.meta( libvlc_meta_TrackNumber );
    if ( trackNbStr.length() == 0 )
    {
        std::cerr << "Failed to get track id" << std::endl;
        return false;
    }
    auto artwork = media.meta( libvlc_meta_ArtworkURL );
    if ( artwork.length() != 0 )
        album->setArtworkUrl( artwork );

    auto title = media.meta( libvlc_meta_Title );
    if ( title.length() == 0 )
    {
        std::cerr << "Failed to compute track title" << std::endl;
        title = "Unknown track #";
        title += trackNbStr;
    }
    unsigned int trackNb = std::stoi( trackNbStr );
    auto track = album->addTrack( title, trackNb );
    if ( track == nullptr )
    {
        std::cerr << "Failure while creating album track" << std::endl;
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
                std::cerr << "Invalid episode id provided" << std::endl;
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
