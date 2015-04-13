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
    ASSERT_NE( f, nullptr );

    auto files = ml->files();

    ASSERT_EQ( files.size(), 2u );
    ASSERT_FALSE( files[0]->isStandAlone() );
}

TEST_F( Folders, Delete )
{
    auto f = ml->addFolder( "." );
    ASSERT_NE( f, nullptr );

    auto folderPath = f->path();

    auto files = ml->files();
    ASSERT_EQ( files.size(), 2u );

    auto filePath = files[0]->mrl();

    ml->deleteFolder( f );

    f = ml->folder( folderPath );
    ASSERT_EQ( nullptr, f );

    files = ml->files();
    ASSERT_EQ( files.size(), 0u );

    // Check the file isn't cached anymore:
    auto file = ml->file( filePath );
    ASSERT_EQ( nullptr, file );

    SetUp();

    // Recheck folder deletion from DB:
    f = ml->folder( folderPath );
    ASSERT_EQ( nullptr, f );
}

TEST_F( Folders, Load )
{
    auto f = ml->addFolder( "." );
    ASSERT_NE( f, nullptr );

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
