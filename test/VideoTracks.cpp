#include "gtest/gtest.h"

#include "IFile.h"
#include "IVideoTrack.h"

class VideoTracks : public testing::Test
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

std::unique_ptr<IMediaLibrary> VideoTracks::ml;

TEST_F( VideoTracks, AddTrack )
{
    auto f = ml->addFile( "file" );
    bool res = f->addVideoTrack( "H264", 1920, 1080, 29.97 );
    ASSERT_TRUE( res );
}

TEST_F( VideoTracks, FetchTracks )
{
    auto f = ml->addFile( "file" );
    f->addVideoTrack( "H264", 1920, 1080, 29.97 );
    f->addVideoTrack( "VP80", 640, 480, 29.97 );

    std::vector<VideoTrackPtr> ts;
    bool res = f->videoTracks( ts );
    ASSERT_TRUE( res );
    ASSERT_EQ( ts.size(), 2u );
}

TEST_F( VideoTracks, CheckUnique )
{
    auto f = ml->addFile( "file" );
    f->addVideoTrack( "H264", 1920, 1080, 29.97 );

    auto f2 = ml->addFile( "file2" );
    f2->addVideoTrack( "H264", 1920, 1080, 29.97 );

    std::vector<VideoTrackPtr> ts;
    f->videoTracks( ts );

    std::vector<VideoTrackPtr> ts2;
    f2->videoTracks( ts2 );

    ASSERT_EQ( ts.size(), 1u );
    ASSERT_EQ( ts2.size(), 1u );

    // Check that only 1 track has been created in DB
    ASSERT_EQ( ts[0]->id(), ts2[0]->id() );
}
