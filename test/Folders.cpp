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
    auto f = ml->addFolder( "." );
    auto files = std::vector<FilePtr>{};
    bool res = ml->files( files );

    ASSERT_TRUE( res );
    ASSERT_EQ( files.size(), 2u );

    ASSERT_FALSE( files[0]->isStandAlone() );
}

TEST_F( Folders, Delete )
{
    auto f = ml->addFolder( "." );
    auto files = std::vector<FilePtr>{};
    bool res = ml->files( files );

    ASSERT_TRUE( res );
    ASSERT_EQ( files.size(), 2u );

    ml->deleteFolder( f );

    res = ml->files( files );
    ASSERT_TRUE( res );
    ASSERT_EQ( files.size(), 0u );
}

TEST_F( Folders, Load )
{
    auto f = ml->addFolder( "." );
    auto files = std::vector<FilePtr>{};
    bool res = ml->files( files );

    ASSERT_TRUE( res );
    ASSERT_EQ( files.size(), 2u );

    SetUp();

    res = ml->files( files );
    ASSERT_TRUE( res );
    ASSERT_EQ( files.size(), 2u );
    ASSERT_FALSE( files[0]->isStandAlone() );
    ASSERT_FALSE( files[1]->isStandAlone() );
}

TEST_F( Folders, InvalidPath )
{
    auto f = ml->addFolder( "/invalid/path" );
    ASSERT_EQ( f, nullptr );

    auto files = std::vector<FilePtr>{};
    bool res = ml->files( files );
    ASSERT_TRUE( res );
    ASSERT_EQ( files.size(), 0u );
}
