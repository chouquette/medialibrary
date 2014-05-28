#include "gtest/gtest.h"

#include "IFile.h"
#include "IAudioTrack.h"

class AudioTracks : public testing::Test
{
    public:
        static std::unique_ptr<IMediaLibrary> ml;

    protected:
        virtual void SetUp()
        {
            ml.reset( MediaLibraryFactory::create() );
            bool res = ml->initialize( "test.db" );
            ASSERT_TRUE( res );
        }

        virtual void TearDown()
        {
            ml.reset();
            unlink("test.db");
        }
};

std::unique_ptr<IMediaLibrary> AudioTracks::ml;

TEST_F( AudioTracks, AddTrack )
{
    auto f = ml->addFile( "file" );
    bool res = f->addAudioTrack( "PCM", 44100 );
    ASSERT_TRUE( res );
}

TEST_F( AudioTracks, FetchTracks )
{
    auto f = ml->addFile( "file" );
    f->addAudioTrack( "PCM", 44100 );
    f->addAudioTrack( "WMA", 48000 );

    std::vector<AudioTrackPtr> ts;
    bool res = f->audioTracks( ts );
    ASSERT_TRUE( res );
    ASSERT_EQ( ts.size(), 2u );
}

TEST_F( AudioTracks, CheckUnique )
{
    auto f = ml->addFile( "file" );
    f->addAudioTrack( "PCM", 44100 );

    auto f2 = ml->addFile( "file2" );
    f2->addAudioTrack( "PCM", 44100 );

    std::vector<AudioTrackPtr> ts;
    f->audioTracks( ts );

    std::vector<AudioTrackPtr> ts2;
    f2->audioTracks( ts2 );

    ASSERT_EQ( ts.size(), 1u );
    ASSERT_EQ( ts2.size(), 1u );

    // Check that only 1 track has been created in DB
    ASSERT_EQ( ts[0]->id(), ts2[0]->id() );
}

