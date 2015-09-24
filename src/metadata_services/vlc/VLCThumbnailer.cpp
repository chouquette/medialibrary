#include "VLCThumbnailer.h"

#include <cstring>

#include "IFile.h"

VLCThumbnailer::VLCThumbnailer(const VLC::Instance &vlc)
    : m_instance( vlc )
{
}

bool VLCThumbnailer::initialize(IMetadataServiceCb *callback, IMediaLibrary *ml)
{
    m_cb = callback;
    m_ml = ml;
    return true;
}

unsigned int VLCThumbnailer::priority() const
{
    // This needs to be lower than the VLCMetadataService, since we want to know the file type.
    return 50;
}

bool VLCThumbnailer::run( FilePtr file, void *data )
{
    if ( file->type() == IFile::Type::UnknownType )
    {
        // If we don't know the file type yet, it actually looks more like a bug
        // since this should run after file type deduction, and not run in case
        // that step fails.
        m_cb->done( file, StatusError, data );
        return false;
    }
    else if ( file->type() != IFile::Type::VideoType )
    {
        // There's no point in generating a thumbnail for a non-video file.
        m_cb->done( file, StatusSuccess, data );
        return true;
    }

    VLC::Media media( m_instance, file->mrl(), VLC::Media::FromPath );
    media.addOption( ":no-audio" );
    media.addOption( ":avcodec-hw=none" );
    VLC::MediaPlayer mp( media );

    setupVout( mp );

    if ( startPlayback( file, mp, data ) == false )
        return false;

    // Seek ahead to have a significant preview
    if ( seekAhead( file, mp, data ) == false )
        return false;

    std::string path( "/tmp/test.jpg" );
    if ( mp.takeSnapshot( 0, path, 320, 240 ) == false )
    {
        m_cb->done( file, StatusError, data );
        return false;
    }
    m_cb->done( file, StatusSuccess, data );
    file->setSnapshot( path );
    return true;
}

bool VLCThumbnailer::startPlayback( FilePtr file, VLC::MediaPlayer &mp, void* data )
{
    bool failed = true;
    int nbVout = 0;

    std::unique_lock<std::mutex> lock( m_mutex );

    // We need a valid vout to call takeSnapshot. Let's also
    // assume the media player is playing when a vout is created.
    mp.eventManager().onVout([this, &failed, &nbVout](int nb) {
        std::unique_lock<std::mutex> lock( m_mutex );
        failed = false;
        nbVout = nb;
        m_cond.notify_all();
    });
    mp.eventManager().onEncounteredError([this]() {
        std::unique_lock<std::mutex> lock( m_mutex );
        m_cond.notify_all();
    });
    mp.play();
    bool success = m_cond.wait_for( lock, std::chrono::seconds( 3 ), [&mp, &nbVout]() {
        return (mp.state() == libvlc_Playing && nbVout > 0) ||
                mp.state() == libvlc_Error;
    });
    if ( success == false || failed == true )
    {
        // In case of timeout or error, don't go any further
        m_cb->done( file, StatusError, data );
        return false;
    }
    return true;
}

bool VLCThumbnailer::seekAhead( FilePtr file, VLC::MediaPlayer& mp, void* data )
{
    std::unique_lock<std::mutex> lock( m_mutex );
    float pos = .0f;
    auto event = mp.eventManager().onPositionChanged([this, &pos](float p) {
        std::unique_lock<std::mutex> lock( m_mutex );
        pos = p;
        m_cond.notify_all();
    });
    mp.setPosition( .3f );
    bool success = m_cond.wait_for( lock, std::chrono::seconds( 3 ), [&pos]() {
        return pos >= .1f;
    });
    // Since we're locking a mutex for each position changed, let's unregister ASAP
    event->unregister();
    if ( success == false )
    {
        m_cb->done( file, StatusError, data );
        return false;
    }
    return true;
}

void VLCThumbnailer::setupVout( VLC::MediaPlayer& )
{
    return;

    // Let's use this ugly base in the future, but so far, takeSnapshot() will do.

//    if ( m_buff == nullptr )
//        m_buff.reset( new uint8_t[320 * 240 * 4] );
//    mp.setVideoFormatCallbacks(
//    // Setup
//    [](char* chroma, unsigned int* width, unsigned int *height, unsigned int *pitches, unsigned int *lines) {
//        strcpy( chroma, "RV32" );
//        *width = 320;
//        *height = 240;
//        *pitches = 240 * 4;
//        *lines = 240;
//        return 1;
//    },
//    // Cleanup
//    nullptr);
//    mp.setVideoCallbacks(
//    // Lock
//    [this](void** pp_buff) {
//        *pp_buff = m_buff.get();
//        return nullptr;
//    },
//    //unlock, display
//    nullptr, nullptr);
}
