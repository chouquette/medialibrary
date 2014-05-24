#include "gtest/gtest.h"

#include "IFile.h"
#include "IMediaLibrary.h"
#include "IShow.h"

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
