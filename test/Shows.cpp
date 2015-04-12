#include "gtest/gtest.h"

#include "IFile.h"
#include "IMediaLibrary.h"
#include "IShow.h"
#include "IShowEpisode.h"

class Shows : public testing::Test
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

std::unique_ptr<IMediaLibrary> Shows::ml;

TEST_F( Shows, Create )
{
    auto s = ml->createShow( "show" );
    ASSERT_NE( s, nullptr );

    auto s2 = ml->show( "show" );
    ASSERT_EQ( s, s2 );
}

TEST_F( Shows, Fetch )
{
    auto s = ml->createShow( "show" );

    // Clear the cache
    SetUp();

    auto s2 = ml->show( "show" );
    // The shared pointers are expected to point to different instances
    ASSERT_NE( s, s2 );

    ASSERT_EQ( s->id(), s2->id() );
}

TEST_F( Shows, SetReleaseDate )
{
    auto s = ml->createShow( "show" );

    s->setReleaseDate( 1234 );
    ASSERT_EQ( s->releaseDate(), 1234 );

    SetUp();

    auto s2 = ml->show( "show" );
    ASSERT_EQ( s->releaseDate(), s2->releaseDate() );
}

TEST_F( Shows, SetShortSummary )
{
    auto s = ml->createShow( "show" );

    s->setShortSummary( "summary" );
    ASSERT_EQ( s->shortSummary(), "summary" );


    SetUp();

    auto s2 = ml->show( "show" );
    ASSERT_EQ( s->shortSummary(), s2->shortSummary() );
}

TEST_F( Shows, SetArtworkUrl )
{
    auto s = ml->createShow( "show" );

    s->setArtworkUrl( "artwork" );
    ASSERT_EQ( s->artworkUrl(), "artwork" );

    SetUp();

    auto s2 = ml->show( "show" );
    ASSERT_EQ( s->artworkUrl(), s2->artworkUrl() );
}

TEST_F( Shows, SetTvdbId )
{
    auto s = ml->createShow( "show" );

    s->setTvdbId( "TVDBID" );
    ASSERT_EQ( s->tvdbId(), "TVDBID" );

    SetUp();

    auto s2 = ml->show( "show" );
    ASSERT_EQ( s->tvdbId(), s2->tvdbId() );
}

////////////////////////////////////////////////////
// Episodes:
////////////////////////////////////////////////////

TEST_F( Shows, AddEpisode )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    ASSERT_NE( e, nullptr );

    ASSERT_EQ( e->episodeNumber(), 1u );
    ASSERT_EQ( e->show(), show );
    ASSERT_EQ( e->name(), "episode 1" );

    auto episodes = show->episodes();
    ASSERT_EQ( episodes.size(), 1u );
    ASSERT_EQ( episodes[0], e );
}

TEST_F( Shows, FetchShowFromEpisode )
{
    auto s = ml->createShow( "show" );
    auto e = s->addEpisode( "episode 1", 1 );
    auto f = ml->addFile( "file" );
    f->setShowEpisode( e );

    auto e2 = f->showEpisode();
    auto s2 = e2->show();
    ASSERT_NE( s2, nullptr );
    ASSERT_EQ( s, s2 );

    SetUp();

    f = ml->file( "file" );
    s2 = f->showEpisode()->show();
    ASSERT_NE( s2, nullptr );
    ASSERT_EQ( s->name(), s2->name() );
}

TEST_F( Shows, SetEpisodeArtwork )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    bool res = e->setArtworkUrl( "path-to-snapshot" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->artworkUrl(), "path-to-snapshot" );

    SetUp();

    show = ml->show( "show" );
    auto episodes = show->episodes();
    ASSERT_EQ( episodes[0]->artworkUrl(), e->artworkUrl() );
}

TEST_F( Shows, SetEpisodeSeasonNumber )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    bool res = e->setSeasonNumber( 42 );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->seasonNumber(), 42u );

    SetUp();

    show = ml->show( "show" );
    auto episodes = show->episodes();
    ASSERT_EQ( episodes[0]->seasonNumber(), e->seasonNumber() );
}

TEST_F( Shows, SetEpisodeSummary )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    bool res = e->setShortSummary( "Insert spoilers here" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->shortSummary(), "Insert spoilers here" );

    SetUp();

    show = ml->show( "show" );
    auto episodes = show->episodes();
    ASSERT_EQ( episodes[0]->shortSummary(), e->shortSummary() );
}

TEST_F( Shows, SetEpisodeTvdbId )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    bool res = e->setTvdbId( "TVDBID" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->tvdbId(), "TVDBID" );

    SetUp();

    show = ml->show( "show" );
    auto episodes = show->episodes();
    ASSERT_EQ( episodes[0]->tvdbId(), e->tvdbId() );
}

////////////////////////////////////////////////////
// Files links:
////////////////////////////////////////////////////

TEST_F( Shows, FileSetShowEpisode )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    auto f = ml->addFile( "file" );

    ASSERT_EQ( f->showEpisode(), nullptr );
    f->setShowEpisode( e );
    ASSERT_EQ( f->showEpisode(), e );

    SetUp();

    f = ml->file( "file" );
    e = f->showEpisode();
    ASSERT_NE( e, nullptr );
    ASSERT_EQ( e->name(), "episode 1" );
}

TEST_F( Shows, DeleteShowEpisode )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    auto f = ml->addFile( "file" );

    f->setShowEpisode( e );
    e->destroy();

    f = ml->file( "file" );
    ASSERT_EQ( f, nullptr );

    SetUp();

    f = ml->file( "file" );
    ASSERT_EQ( f, nullptr );
}

TEST_F( Shows, DeleteShow )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    auto f = ml->addFile( "file" );
    f->setShowEpisode( e );

    bool res = show->destroy();
    ASSERT_TRUE( res );
    f = ml->file( "file" );
    ASSERT_EQ( f, nullptr );
}
