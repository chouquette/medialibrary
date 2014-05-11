#include "gtest/gtest.h"

#include "IMediaLibrary.h"

class MLTest : public testing::Test
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
            unlink("test.db");
        }
};

IMediaLibrary* MLTest::ml;

TEST_F( MLTest, Init )
{
    // only test for correct test fixture behavior
}

TEST_F( MLTest, InsertFile )
{
    IFile* f = ml->addFile( "/dev/null" );
    ASSERT_TRUE( f != NULL );

    std::vector<IFile*> files = ml->files();
    ASSERT_EQ( files.size(), 1u );
    ASSERT_EQ( files[0]->mrl(), f->mrl() );
}
