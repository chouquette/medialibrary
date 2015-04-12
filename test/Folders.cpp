#include "gtest/gtest.h"

#include "IFile.h"
#include "IFolder.h"
#include "IMediaLibrary.h"

class Folders : public testing::Test
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

std::unique_ptr<IMediaLibrary> Folders::ml;

TEST_F( Folders, Add )
{
    ml->addFolder( "." );
    auto files = ml->files();

    ASSERT_EQ( files.size(), 2u );
    ASSERT_FALSE( files[0]->isStandAlone() );
}

TEST_F( Folders, Delete )
{
    auto f = ml->addFolder( "." );
    auto files = ml->files();

    ASSERT_EQ( files.size(), 2u );

    ml->deleteFolder( f );

    files = ml->files();
    ASSERT_EQ( files.size(), 0u );
}

TEST_F( Folders, Load )
{
    auto f = ml->addFolder( "." );
    auto files = ml->files();

    ASSERT_EQ( files.size(), 2u );

    SetUp();

    files = ml->files();
    ASSERT_EQ( files.size(), 2u );
    ASSERT_FALSE( files[0]->isStandAlone() );
    ASSERT_FALSE( files[1]->isStandAlone() );
}

TEST_F( Folders, InvalidPath )
{
    auto f = ml->addFolder( "/invalid/path" );
    ASSERT_EQ( f, nullptr );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 0u );
}
