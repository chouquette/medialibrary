#include "gtest/gtest.h"

#include "IMediaLibrary.h"
#include "IAlbum.h"

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
