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
    auto ctx = new Context( this, file, data );
//    libvlc_audio_output_set( ctx->mp, "dummy" );
    //attach events
    ctx->media.parseAsync();
    return true;
}

void VLCMetadataService::parse(VLCMetadataService::Context* ctx)
{
    auto status = handleMediaMeta( ctx );
    ctx->self->m_cb->done( ctx->file, status, ctx->data );

    std::lock_guard<std::mutex> lock( m_lock );
    m_cleanup.push_back( ctx );
}

ServiceStatus VLCMetadataService::handleMediaMeta( VLCMetadataService::Context* ctx )
{
    auto tracks = ctx->media.tracks();
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
            ctx->file->addVideoTrack( fcc, track.width(), track.height(), .0f );
        }
        else if ( track.type() == VLC::MediaTrack::Audio )
        {
            ctx->file->addAudioTrack( fcc, track.bitrate(), track.rate(),
                                      track.channels() );
        }
    }
    if ( isAudio == true )
    {
        if ( parseAudioFile( ctx ) == false )
            return StatusFatal;
    }
    else
    {
        parseVideoFile( ctx );
    }
    auto file = std::static_pointer_cast<File>( ctx->file );
    file->setReady();
    return StatusSuccess;
}

void VLCMetadataService::parseAudioFile( VLCMetadataService::Context* ctx )
{
    auto albumTitle = ctx->media.meta( libvlc_meta_Album );
    if ( albumTitle.length() == 0 )
        return true;
    auto album = m_ml->album( albumTitle );
    if ( album == nullptr )
        album = m_ml->createAlbum( albumTitle );
    if ( album == nullptr )
    {
        std::cerr << "Failed to create/get album" << std::endl;
        return;
    }

    auto trackNbStr = ctx->media.meta( libvlc_meta_TrackNumber );
    if ( trackNbStr.length() == 0 )
    {
        std::cerr << "Failed to get track id" << std::endl;
        return ;
    }
    auto artwork = ctx->media.meta( libvlc_meta_ArtworkURL );
    if ( artwork.length() != 0 )
        album->setArtworkUrl( artwork );

    auto title = ctx->media.meta( libvlc_meta_Title );
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
        return ;
    }
    ctx->file->setAlbumTrack( track );

    auto genre = ctx->media.meta( libvlc_meta_Genre );
    if ( genre.length() != 0 )
        track->setGenre( genre );

    auto artist = ctx->media.meta( libvlc_meta_Artist );
    if ( artist.length() != 0 )
        track->setArtist( artist );
    return true;
}

void VLCMetadataService::parseVideoFile( VLCMetadataService::Context* )
{

}

void VLCMetadataService::cleanup()
{
    std::lock_guard<std::mutex> lock( m_lock );
    while ( m_cleanup.empty() == false )
    {
        delete m_cleanup.back();
        m_cleanup.pop_back();
    }
}

VLCMetadataService::Context::Context( VLCMetadataService* self, FilePtr file, void* data )
    : self( self )
    , file( file )
    , data( data )
    , media( VLC::Media::fromPath( self->m_instance, file->mrl() ) )
{
    media.eventManager().attach( libvlc_MediaParsedChanged, this );
}

VLCMetadataService::Context::~Context()
{
    media.eventManager().detach( libvlc_MediaParsedChanged, this );
}

void VLCMetadataService::Context::parsedChanged(bool status)
{
    if ( status == true )
        self->parse( this );
}
