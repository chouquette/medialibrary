#include "Tests.h"

#include "IFile.h"
#include "IAudioTrack.h"

class AudioTracks : public Tests
{
};

TEST_F( AudioTracks, AddTrack )
{
    auto f = ml->addFile( "file.mp3" );
    bool res = f->addAudioTrack( "PCM", 44100, 128, 2 );
    ASSERT_TRUE( res );
}

TEST_F( AudioTracks, FetchTracks )
{
    auto f = ml->addFile( "file.mp3" );
    f->addAudioTrack( "PCM", 44100, 128, 2 );
    f->addAudioTrack( "WMA", 48000, 128, 2 );

    auto ts = f->audioTracks();
    ASSERT_EQ( ts.size(), 2u );
}

TEST_F( AudioTracks, CheckUnique )
{
    auto f = ml->addFile( "file.mp3" );
    f->addAudioTrack( "PCM", 44100, 128, 2 );

    auto f2 = ml->addFile( "file2.mp3" );
    f2->addAudioTrack( "PCM", 44100, 128, 2 );

    auto ts = f->audioTracks();

    auto ts2 = f2->audioTracks();

    ASSERT_EQ( ts.size(), 1u );
    ASSERT_EQ( ts2.size(), 1u );

    // Check that only 1 track has been created in DB
    ASSERT_EQ( ts[0]->id(), ts2[0]->id() );
}

