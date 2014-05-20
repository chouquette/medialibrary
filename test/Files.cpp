#include "gtest/gtest.h"

#include "IMediaLibrary.h"
#include "IFile.h"

class Files : public testing::Test
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

IMediaLibrary* Files::ml;

TEST_F( Files, Init )
{
    // only test for correct test fixture behavior
}

TEST_F( Files, Create )
{
    auto f = ml->addFile( "/dev/null" );
    ASSERT_TRUE( f != NULL );

    ASSERT_EQ( f->playCount(), 0 );
    ASSERT_TRUE( f->albumTrack() == NULL );
    ASSERT_TRUE( f->showEpisode() == NULL );

    std::vector<std::shared_ptr<IFile>> files;
    bool success = ml->files( files );
    ASSERT_TRUE( success );
    ASSERT_EQ( files.size(), 1u );
    ASSERT_EQ( files[0]->mrl(), f->mrl() );
}

TEST_F( Files, Fetch )
{
    auto f = ml->addFile( "/dev/null" );
    auto f2 = ml->file( "/dev/null" );
    ASSERT_EQ( f->mrl(), f2->mrl() );
    // Basic caching test:
    ASSERT_EQ( f, f2 );
}

TEST_F( Files, Delete )
{
    auto f = ml->addFile( "/dev/loutre" );
    auto f2 = ml->file( "/dev/loutre" );

    ASSERT_EQ( f, f2 );

    ml->deleteFile( f );
    f2 = ml->file( "/dev/loutre" );
    ASSERT_EQ( f2, nullptr );
}

TEST_F( Files, Duplicate )
{
    auto f = ml->addFile( "/dev/moulaf" );
    auto f2 = ml->addFile( "/dev/moulaf" );

    ASSERT_NE( f, nullptr );
    ASSERT_EQ( f2, nullptr );

    f2 = ml->file( "/dev/moulaf" );
    ASSERT_EQ( f, f2 );
}
