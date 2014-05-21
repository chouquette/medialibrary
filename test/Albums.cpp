#include "gtest/gtest.h"

#include "IMediaLibrary.h"
#include "IFile.h"

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

