#include "Tests.h"
#include <vlc/vlc.h>
#include <chrono>
#include <condition_variable>

#include "IMediaLibrary.h"
#include "IMetadataService.h"
#include "IFile.h"
#include "IAudioTrack.h"
#include "IAlbum.h"
#include "IAlbumTrack.h"
#include "IVideoTrack.h"
#include "metadata_services/VLCMetadataService.h"

class ServiceCb : public IParserCb
{
    public:
        std::condition_variable waitCond;
        std::mutex mutex;
        volatile bool failed;

        ServiceCb()
            : failed(false)
        {
        }

        ~ServiceCb()
        {
        }

        virtual void onServiceDone( FilePtr, ServiceStatus status )
        {
            if ( status != StatusSuccess )
                failed = true;
            waitCond.notify_all();
        }

        virtual void onFileDone( FilePtr )
        {
        }
};

class VLCMetadataServices : public Tests
{
    protected:
        static std::unique_ptr<ServiceCb> cb;

    protected:
        static void SetUpTestCase()
        {
            cb.reset( new ServiceCb );
        }

        virtual void SetUp() override
        {
            Tests::SetUp();

            cb->failed = false;
            const char* args[] = {
                "-vv"
            };
            auto vlcInstance = VLC::Instance::create( sizeof(args) / sizeof(args[0]), args );
            ASSERT_TRUE( vlcInstance.isValid() );
            auto vlcService = new VLCMetadataService( vlcInstance );

            // This takes ownership of vlcService
            ml->addMetadataService( vlcService );
        }
};

std::unique_ptr<ServiceCb> VLCMetadataServices::cb;

TEST_F( VLCMetadataServices, ParseAudio )
{
    std::unique_lock<std::mutex> lock( cb->mutex );
    auto file = ml->addFile( "mr-zebra.mp3" );
    ml->parse( file, cb.get() );
    bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [&]{
        return cb->failed == true || ( file->audioTracks().size() > 0 );
    } );

    ASSERT_TRUE( res );
    ASSERT_FALSE( cb->failed );
    Reload();
    file = ml->file( "mr-zebra.mp3" );
    auto tracks = file->audioTracks();
    ASSERT_EQ( tracks.size(), 1u );
    auto track = tracks[0];
    ASSERT_EQ( track->codec(), "mpga" );
    ASSERT_EQ( track->bitrate(), 128000u );
    ASSERT_EQ( track->sampleRate(), 44100u );
    ASSERT_EQ( track->nbChannels(), 2u );
}

TEST_F( VLCMetadataServices, ParseAlbum )
{
    auto file = ml->addFile( "mr-zebra.mp3" );
    std::unique_lock<std::mutex> lock( cb->mutex );
    ml->parse( file, cb.get() );
    bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [&]{
        return cb->failed == true || file->albumTrack() != nullptr;
    } );

    ASSERT_TRUE( res );
    Reload();

    file = ml->file( "mr-zebra.mp3" );
    auto track = file->albumTrack();
    ASSERT_NE( track, nullptr );
    ASSERT_EQ( track->title(), "Mr. Zebra" );
    ASSERT_EQ( track->genre(), "Rock" );
    ASSERT_EQ( track->artist(), "Tori Amos" );

    auto album = track->album();
    ASSERT_NE( album, nullptr );
    ASSERT_EQ( album->title(), "Boys for Pele" );
//    ASSERT_NE( album->artworkUrl().length(), 0u );

    auto album2 = ml->album( "Boys for Pele" );
    ASSERT_EQ( album, album2 );
}

TEST_F( VLCMetadataServices, ParseVideo )
{
    std::unique_lock<std::mutex> lock( cb->mutex );
    auto file = ml->addFile( "mrmssmith.mp4" );
    ml->parse( file, cb.get() );
    bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [file]{
        return cb->failed == true || (file->videoTracks().size() != 0);
    } );

    ASSERT_TRUE( res );
    Reload();

    file = ml->file( "mrmssmith.mp4" );

    ASSERT_EQ( file->showEpisode(), nullptr );

    auto tracks = file->videoTracks();
    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0]->codec(), "h264" );
    ASSERT_EQ( tracks[0]->width(), 320u );
    ASSERT_EQ( tracks[0]->height(), 176u );
    ASSERT_EQ( tracks[0]->fps(), 25 );

    auto audioTracks = file->audioTracks();
    ASSERT_EQ( audioTracks.size(), 1u );
    ASSERT_EQ( audioTracks[0]->codec(), "mp4a" );
    ASSERT_EQ( audioTracks[0]->sampleRate(), 44100u );
    ASSERT_EQ( audioTracks[0]->nbChannels(), 2u );
}

