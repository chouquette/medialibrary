#include "gtest/gtest.h"

#include "IMediaLibrary.h"
#include "ILabel.h"
#include "IFile.h"

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

TEST_F( MLTest, FetchFile )
{
    auto f = ml->addFile( "/dev/null" );
    auto f2 = ml->file( "/dev/null" );
    ASSERT_EQ( f->mrl(), f2->mrl() );
    // Basic caching test:
    ASSERT_EQ( f, f2 );
}

TEST_F( MLTest, AddLabel )
{
    auto f = ml->addFile( "/dev/null" );
    auto l1 = f->addLabel( "sea otter" );
    auto l2 = f->addLabel( "cony the cone" );

    ASSERT_TRUE( l1 != NULL );
    ASSERT_TRUE( l2 != NULL );

    auto labels = f->labels();
    ASSERT_EQ( labels.size(), 2u );
    ASSERT_EQ( labels[0]->name(), "sea otter" );
    ASSERT_EQ( labels[1]->name(), "cony the cone" );
}

TEST_F( MLTest, RemoveLabel )
{
    auto f = ml->addFile( "/dev/null" );
    auto l1 = f->addLabel( "sea otter" );
    auto l2 = f->addLabel( "cony the cone" );

    auto labels = f->labels();
    ASSERT_EQ( labels.size(), 2u );

    bool res = f->removeLabel( l1 );
    ASSERT_TRUE( res );

    // Check for existing file first
    labels = f->labels();
    ASSERT_EQ( labels.size(), 1u );
    ASSERT_EQ( labels[0]->name(), "cony the cone" );

    // And now clean fetch another instance of the file & check again for DB replication
    auto f2 = ml->file( f->mrl() );
    labels = f2->labels();
    ASSERT_EQ( labels.size(), 1u );
    ASSERT_EQ( labels[0]->name(), "cony the cone" );

    // Remove a non-linked label
    res = f->removeLabel( l1 );
    ASSERT_FALSE( res );

    // Remove the last label
    res = f->removeLabel( l2 );
    ASSERT_TRUE( res );

    labels = f->labels();
    ASSERT_EQ( labels.size(), 0u );

    // Check again for DB replication
    f2 = ml->file( f->mrl() );
    labels = f2->labels();
    ASSERT_EQ( labels.size(), 0u );
}
