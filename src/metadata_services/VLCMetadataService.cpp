#include <iostream>

#include "VLCMetadataService.h"
#include "IFile.h"
#include "IAlbum.h"
#include "IAlbumTrack.h"

#include "File.h"

VLCMetadataService::VLCMetadataService( libvlc_instance_t* vlc )
    : m_instance( vlc )
    , m_cb( nullptr )
    , m_ml( nullptr )
{
    libvlc_retain( m_instance );
}

VLCMetadataService::~VLCMetadataService()
{
    cleanup();
    libvlc_release( m_instance );
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

bool VLCMetadataService::run( FilePtr file , void* data )
{
    cleanup();
    auto ctx = new Context;
    ctx->self = this;
    ctx->file = file;
    ctx->data = data;
    ctx->media = libvlc_media_new_path( m_instance, file->mrl().c_str() );
    ctx->m_em = libvlc_media_event_manager( ctx->media );
    ctx->mp = libvlc_media_player_new_from_media( ctx->media );
    ctx->mp_em = libvlc_media_player_event_manager( ctx->mp );

//    libvlc_audio_output_set( ctx->mp, "dummy" );
    //attach events
    libvlc_event_attach( ctx->mp_em, libvlc_MediaPlayerLengthChanged, &eventCallback, ctx );
    libvlc_event_attach( ctx->m_em, libvlc_MediaParsedChanged, &eventCallback, ctx );
    libvlc_media_parse_async( ctx->media );
    return true;
}

void VLCMetadataService::eventCallback( const libvlc_event_t* e, void* data )
{
    Context* ctx = reinterpret_cast<Context*>( data );
    switch ( e->type )
    {
        case libvlc_MediaParsedChanged:
            ctx->self->parse( ctx );
            return;
        default:
            return;
    }
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
    libvlc_media_track_t** tracks;
    auto nbTracks = libvlc_media_tracks_get( ctx->media, &tracks );
    if ( nbTracks == 0 )
    {
        std::cerr << "Failed to fetch tracks" << std::endl;
        return StatusFatal;
    }
    bool isAudio = true;
    for ( unsigned int i = 0; i < nbTracks; ++i )
    {
        std::string fcc( (const char*)(&tracks[i]->i_codec), 4 );
        if ( tracks[i]->i_type != libvlc_track_audio )
        {
            if ( tracks[i]->i_type == libvlc_track_text )
                continue ;
            isAudio = false;
            ctx->file->addVideoTrack( fcc, tracks[i]->video->i_width, tracks[i]->video->i_height, .0f );
        }
        else
        {
            ctx->file->addAudioTrack( fcc, tracks[i]->i_bitrate, tracks[i]->audio->i_rate,
                                      tracks[i]->audio->i_channels );
        }
    }
    if ( isAudio == true )
    {
        parseAudioFile( ctx );
    }
    else
    {
        parseVideoFile( ctx );
    }
    auto file = std::static_pointer_cast<File>( ctx->file );
    file->setReady();
    libvlc_media_tracks_release( tracks, nbTracks );
    return StatusSuccess;
}

void VLCMetadataService::parseAudioFile( VLCMetadataService::Context* ctx )
{
    char* albumTitle = libvlc_media_get_meta( ctx->media, libvlc_meta_Album );
    if ( albumTitle == nullptr )
        return ;
    auto album = m_ml->album( albumTitle );
    if ( album == nullptr )
        album = m_ml->createAlbum( albumTitle );
    free( albumTitle );
    if ( album == nullptr )
    {
        std::cerr << "Failed to create/get album" << std::endl;
        return;
    }

    char* trackNbStr = libvlc_media_get_meta( ctx->media, libvlc_meta_TrackNumber );
    if ( trackNbStr == nullptr )
    {
        std::cerr << "Failed to get track id" << std::endl;
        return ;
    }
    char* artwork = libvlc_media_get_meta( ctx->media, libvlc_meta_ArtworkURL );
    if ( artwork != nullptr )
    {
        album->setArtworkUrl( artwork );
        free( artwork );
    }
    char* trackTitle = libvlc_media_get_meta( ctx->media, libvlc_meta_Title );
    std::string title;
    if ( trackTitle == nullptr )
    {
        std::cerr << "Failed to compute track title" << std::endl;
        title = "Unknown track #";
        title += trackNbStr;
    }
    else
    {
        title = trackTitle;
    }
    free( trackTitle );
    unsigned int trackNb = atoi( trackNbStr );
    free( trackNbStr );
    auto track = album->addTrack( title, trackNb );
    if ( track == nullptr )
    {
        std::cerr << "Failure while creating album track" << std::endl;
        return ;
    }
    ctx->file->setAlbumTrack( track );
    char* genre = libvlc_media_get_meta( ctx->media, libvlc_meta_Genre );
    if ( genre != nullptr )
    {
        track->setGenre( genre );
        free( genre );
    }
    char* artist = libvlc_media_get_meta( ctx->media, libvlc_meta_Artist );
    if ( artist != nullptr )
    {
        track->setArtist( artist );
        free( artist );
    }
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

VLCMetadataService::Context::Context()
    : self( nullptr )
    , media( nullptr )
    , m_em( nullptr )
    , mp( nullptr )
    , mp_em( nullptr )
{
}

VLCMetadataService::Context::~Context()
{
    libvlc_event_detach( m_em, libvlc_MediaParsedChanged, &VLCMetadataService::eventCallback, this );
    libvlc_event_detach( mp_em, libvlc_MediaPlayerLengthChanged, &VLCMetadataService::eventCallback, this );
    libvlc_media_release( media );
    libvlc_media_player_release( mp );
}
