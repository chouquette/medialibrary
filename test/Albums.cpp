#include "gtest/gtest.h"

#include "IAlbum.h"
#include "IAlbumTrack.h"
#include "IFile.h"
#include "IMediaLibrary.h"

class Albums : public testing::Test
{
    public:
        static IMediaLibrary* ml;

    protected:
        virtual void SetUp()
        {
            ml = MediaLibraryFactory::create();
            bool res = ml->initialize( "test.db" );
            ASSERT_TRUE( res );
        }

        virtual void TearDown()
        {
            delete ml;
            ml = nullptr;
            unlink("test.db");
        }
};

IMediaLibrary* Albums::ml;

TEST_F( Albums, Create )
{
    auto a = ml->createAlbum( "mytag" );
    ASSERT_NE( a, nullptr );

    auto a2 = ml->album( "mytag" );
    ASSERT_EQ( a, a2 );
}

TEST_F( Albums, Fetch )
{
    auto a = ml->createAlbum( "album" );

    // Clear the cache
    delete ml;
    SetUp();

    auto a2 = ml->album( "album" );
    // The shared pointer are expected to point to a different instance
    ASSERT_NE( a, a2 );

    ASSERT_EQ( a->id(), a2->id() );
}

TEST_F( Albums, AddTrack )
{
    auto a = ml->createAlbum( "albumtag" );
    auto track = a->addTrack( "track", 10 );
    ASSERT_NE( track, nullptr );

    std::vector<AlbumTrackPtr> tracks;
    bool res = a->tracks( tracks );
    ASSERT_TRUE( res );
    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0], track );

    delete ml;
    SetUp();

    a->tracks( tracks );
    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0]->title(), track->title() );
}

TEST_F( Albums, AssignTrack )
{
    auto f = ml->addFile( "file" );
    auto a = ml->createAlbum( "album" );
    auto t = a->addTrack( "track", 1 );

    ASSERT_EQ( f->albumTrack(), nullptr );
    bool res = f->setAlbumTrack( t );
    ASSERT_TRUE( res );
    ASSERT_NE( f->albumTrack(), nullptr );
    ASSERT_EQ( f->albumTrack(), t );

    delete ml;
    SetUp();

    f = ml->file( "file" );
    t = f->albumTrack();
    ASSERT_NE( t, nullptr );
    ASSERT_EQ( t->title(), "track" );
}

TEST_F( Albums, DeleteTrack )
{
    auto f = ml->addFile( "file" );
    auto a = ml->createAlbum( "album" );
    auto t = a->addTrack( "track", 1 );
    f->setAlbumTrack( t );

    bool res = t->destroy();
    ASSERT_TRUE( res );

    auto f2 = ml->file( "file" );
    ASSERT_EQ( f2, nullptr );
}

TEST_F( Albums, SetGenre )
{
    auto a = ml->createAlbum( "album" );
    auto t = a->addTrack( "track", 1 );

    t->setGenre( "happy underground post progressive death metal" );
    ASSERT_EQ( t->genre(), "happy underground post progressive death metal" );

    delete ml;
    SetUp();

    std::vector<AlbumTrackPtr> tracks;
    a->tracks( tracks );
    auto t2 = tracks[0];
    ASSERT_EQ( t->genre(), t2->genre() );
}

TEST_F( Albums, SetName )
{
    auto a = ml->createAlbum( "album" );

    a->setName( "albumname" );
    ASSERT_EQ( a->name(), "albumname" );

    delete ml;
    SetUp();

    auto a2 = ml->album( "album" );
    ASSERT_EQ( a->name(), a2->name() );
}

TEST_F( Albums, SetReleaseDate )
{
    auto a = ml->createAlbum( "album" );

    a->setReleaseDate( 1234 );
    ASSERT_EQ( a->releaseDate(), 1234 );

    delete ml;
    SetUp();

    auto a2 = ml->album( "album" );
    ASSERT_EQ( a->releaseDate(), a2->releaseDate() );
}

TEST_F( Albums, SetShortSummary )
{
    auto a = ml->createAlbum( "album" );

    a->setShortSummary( "summary" );
    ASSERT_EQ( a->shortSummary(), "summary" );

    delete ml;
    SetUp();

    auto a2 = ml->album( "album" );
    ASSERT_EQ( a->shortSummary(), a2->shortSummary() );
}

TEST_F( Albums, SetArtworkUrl )
{
    auto a = ml->createAlbum( "album" );

    a->setArtworkUrl( "artwork" );
    ASSERT_EQ( a->artworkUrl(), "artwork" );

    delete ml;
    SetUp();

    auto a2 = ml->album( "album" );
    ASSERT_EQ( a->artworkUrl(), a2->artworkUrl() );
}

TEST_F( Albums, FetchAlbumFromTrack )
{
    {
        auto a = ml->createAlbum( "album" );
        a->setName( "album" );
        auto f = ml->addFile( "file" );
        auto t = a->addTrack( "track 1", 1 );
        f->setAlbumTrack( t );
    }
    delete ml;
    SetUp();

    auto f = ml->file( "file" );
    auto t = f->albumTrack();
    auto a = t->album();
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->name(), "album" );
}
