#include "gtest/gtest.h"
#include <vlc/vlc.h>
#include <chrono>
#include <condition_variable>

#include "IMediaLibrary.h"
#include "IMetadataService.h"
#include "IFile.h"
#include "IAudioTrack.h"
#include "IAlbum.h"
#include "IAlbumTrack.h"
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

class VLCMetadataServices : public testing::Test
{
    public:
        static std::unique_ptr<IMediaLibrary> ml;
        static std::unique_ptr<ServiceCb> cb;
        static libvlc_instance_t* vlcInstance;

    protected:
        static void SetUpTestCase()
        {
            cb.reset( new ServiceCb );
        }

        virtual void SetUp()
        {
            cb->failed = false;
            ml.reset( MediaLibraryFactory::create() );
            if ( vlcInstance != nullptr )
                libvlc_release( vlcInstance );
            const char* args[] = {
                "-vv"
            };
            vlcInstance = libvlc_new( sizeof(args) /sizeof(args[0]), args );
            ASSERT_NE(vlcInstance, nullptr);
            auto vlcService = new VLCMetadataService( vlcInstance );

            // This takes ownership of vlcService
            ml->addMetadataService( vlcService );
            bool res = ml->initialize( "test.db" );
            ASSERT_TRUE( res );
        }

        virtual void TearDown()
        {
            libvlc_release( vlcInstance );
            vlcInstance = nullptr;
            ml.reset();
            unlink("test.db");
        }
};

std::unique_ptr<IMediaLibrary> VLCMetadataServices::ml;
std::unique_ptr<ServiceCb> VLCMetadataServices::cb;
libvlc_instance_t* VLCMetadataServices::vlcInstance;

TEST_F( VLCMetadataServices, ParseAudio )
{
    std::unique_lock<std::mutex> lock( cb->mutex );
    auto file = ml->addFile( "mr-zebra.mp3" );
    ml->parse( file, cb.get() );
    std::vector<AudioTrackPtr> tracks;
    bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [&]{
        return cb->failed == true || ( file->audioTracks( tracks ) == true && tracks.size() > 0 );
    } );

    ASSERT_TRUE( res );
    ASSERT_FALSE( cb->failed );
    SetUp();
    file = ml->file( "mr-zebra.mp3" );
    res = file->audioTracks( tracks );
    ASSERT_TRUE( res );
    ASSERT_EQ( tracks.size(), 1u );
    auto track = tracks[0];
    ASSERT_EQ( track->codec(), "mpga" );
    ASSERT_EQ( track->bitrate(), 128000u );
    ASSERT_EQ( track->sampleRate(), 44100u );
    ASSERT_EQ( track->nbChannels(), 2u );
}

TEST_F( VLCMetadataServices, ParseAlbum )
{
    std::unique_lock<std::mutex> lock( cb->mutex );
    auto file = ml->addFile( "mr-zebra.mp3" );
    ml->parse( file, cb.get() );
    bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [&]{
        return cb->failed == true || file->albumTrack() != nullptr;
    } );

    ASSERT_TRUE( res );
    SetUp();

    file = ml->file( "mr-zebra.mp3" );
    auto track = file->albumTrack();
    ASSERT_NE( track, nullptr );
    ASSERT_EQ( track->title(), "Mr. Zebra" );
    ASSERT_EQ( track->genre(), "Rock" );
    ASSERT_EQ( track->artist(), "Tori Amos" );
    auto album = track->album();
    ASSERT_NE( album, nullptr );
    ASSERT_EQ( album->title(), "Boys for Pele" );
    ASSERT_NE( album->artworkUrl().length(), 0u );

    auto album2 = ml->album( "Boys for Pele" );
    ASSERT_EQ( album, album2 );
}
