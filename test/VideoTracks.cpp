#include "Tests.h"

#include "IFile.h"
#include "IVideoTrack.h"

class VideoTracks : public Tests
{
};

TEST_F( VideoTracks, AddTrack )
{
    auto f = ml->addFile( "file.avi" );
    bool res = f->addVideoTrack( "H264", 1920, 1080, 29.97 );
    ASSERT_TRUE( res );
}

TEST_F( VideoTracks, FetchTracks )
{
    auto f = ml->addFile( "file.avi" );
    f->addVideoTrack( "H264", 1920, 1080, 29.97 );
    f->addVideoTrack( "VP80", 640, 480, 29.97 );

    auto ts = f->videoTracks();
    ASSERT_EQ( ts.size(), 2u );
}

TEST_F( VideoTracks, CheckUnique )
{
    auto f = ml->addFile( "file.avi" );
    f->addVideoTrack( "H264", 1920, 1080, 29.97 );

    auto f2 = ml->addFile( "file2.avi" );
    f2->addVideoTrack( "H264", 1920, 1080, 29.97 );

    auto ts = f->videoTracks();
    auto ts2 = f2->videoTracks();

    ASSERT_EQ( ts.size(), 1u );
    ASSERT_EQ( ts2.size(), 1u );

    // Check that only 1 track has been created in DB
    ASSERT_EQ( ts[0]->id(), ts2[0]->id() );
}
