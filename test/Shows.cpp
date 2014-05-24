#include "gtest/gtest.h"

#include "IFile.h"
#include "IMediaLibrary.h"
#include "IShow.h"
#include "IShowEpisode.h"

class Shows : public testing::Test
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

IMediaLibrary* Shows::ml;

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
    delete ml;
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

    delete ml;
    SetUp();

    auto s2 = ml->show( "show" );
    ASSERT_EQ( s->releaseDate(), s2->releaseDate() );
}

TEST_F( Shows, SetShortSummary )
{
    auto s = ml->createShow( "show" );

    s->setShortSummary( "summary" );
    ASSERT_EQ( s->shortSummary(), "summary" );

    delete ml;
    SetUp();

    auto s2 = ml->show( "show" );
    ASSERT_EQ( s->shortSummary(), s2->shortSummary() );
}

TEST_F( Shows, SetArtworkUrl )
{
    auto s = ml->createShow( "show" );

    s->setArtworkUrl( "artwork" );
    ASSERT_EQ( s->artworkUrl(), "artwork" );

    delete ml;
    SetUp();

    auto s2 = ml->show( "show" );
    ASSERT_EQ( s->artworkUrl(), s2->artworkUrl() );
}

TEST_F( Shows, SetTvdbId )
{
    auto s = ml->createShow( "show" );

    s->setTvdbId( "TVDBID" );
    ASSERT_EQ( s->tvdbId(), "TVDBID" );

    delete ml;
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

    std::vector<ShowEpisodePtr> episodes;
    bool res = show->episodes( episodes );
    ASSERT_TRUE( res );
    ASSERT_EQ( episodes.size(), 1u );
    ASSERT_EQ( episodes[0], e );
}

TEST_F( Shows, SetEpisodeArtwork )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    bool res = e->setArtworkUrl( "path-to-snapshot" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->artworkUrl(), "path-to-snapshot" );

    delete ml;
    SetUp();

    show = ml->show( "show" );
    std::vector<ShowEpisodePtr> episodes;
    show->episodes( episodes );
    ASSERT_EQ( episodes[0]->artworkUrl(), e->artworkUrl() );
}

TEST_F( Shows, SetEpisodeSeasonNumber )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    bool res = e->setSeasonNumber( 42 );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->seasonNumber(), 42 );

    delete ml;
    SetUp();

    show = ml->show( "show" );
    std::vector<ShowEpisodePtr> episodes;
    show->episodes( episodes );
    ASSERT_EQ( episodes[0]->seasonNumber(), e->seasonNumber() );
}

TEST_F( Shows, SetEpisodeSummary )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    bool res = e->setShortSummary( "Insert spoilers here" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->shortSummary(), "Insert spoilers here" );

    delete ml;
    SetUp();

    show = ml->show( "show" );
    std::vector<ShowEpisodePtr> episodes;
    show->episodes( episodes );
    ASSERT_EQ( episodes[0]->shortSummary(), e->shortSummary() );
}

TEST_F( Shows, SetEpisodeTvdbId )
{
    auto show = ml->createShow( "show" );
    auto e = show->addEpisode( "episode 1", 1 );
    bool res = e->setTvdbId( "TVDBID" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->tvdbId(), "TVDBID" );

    delete ml;
    SetUp();

    show = ml->show( "show" );
    std::vector<ShowEpisodePtr> episodes;
    show->episodes( episodes );
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

    delete ml;
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

    delete ml;
    SetUp();

    f = ml->file( "file" );
    ASSERT_EQ( f, nullptr );
}
