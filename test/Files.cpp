#include "Tests.h"

#include "IMediaLibrary.h"
#include "IFile.h"

class Files : public Tests
{
};


TEST_F( Files, Init )
{
    // only test for correct test fixture behavior
}

TEST_F( Files, Create )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_NE( f, nullptr );

    ASSERT_EQ( f->playCount(), 0 );
    ASSERT_EQ( f->albumTrack(), nullptr );
    ASSERT_EQ( f->showEpisode(), nullptr );
    ASSERT_TRUE( f->isStandAlone() );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 1u );
    ASSERT_EQ( files[0]->mrl(), f->mrl() );
}

TEST_F( Files, Fetch )
{
    auto f = ml->addFile( "media.avi", nullptr );
    auto f2 = ml->file( "media.avi" );
    ASSERT_EQ( f->mrl(), f2->mrl() );
    ASSERT_EQ( f, f2 );

    // Flush cache and fetch from DB
    Reload();

    f2 = ml->file( "media.avi" );
    ASSERT_EQ( f->mrl(), f2->mrl() );
    ASSERT_TRUE( f2->isStandAlone() );
}

TEST_F( Files, Delete )
{
    auto f = ml->addFile( "media.avi", nullptr );
    auto f2 = ml->file( "media.avi" );

    ASSERT_EQ( f, f2 );

    ml->deleteFile( f );
    f2 = ml->file( "media.avi" );
    ASSERT_EQ( f2, nullptr );
}

TEST_F( Files, Duplicate )
{
    auto f = ml->addFile( "media.avi", nullptr );
    auto f2 = ml->addFile( "media.avi", nullptr );

    ASSERT_NE( f, nullptr );
    ASSERT_EQ( f2, nullptr );

    f2 = ml->file( "media.avi" );
    ASSERT_EQ( f, f2 );
}

TEST_F( Files, LastModificationDate )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_NE( 0u, f->lastModificationDate() );

    Reload();
    auto f2 = ml->file( "media.avi" );
    ASSERT_EQ( f->lastModificationDate(), f2->lastModificationDate() );
}

TEST_F( Files, Duration )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_EQ( f->duration(), -1 );

    // Use a value that checks we're using a 64bits value
    int64_t d = int64_t(1) << 40;

    f->setDuration( d );
    ASSERT_EQ( f->duration(), d );

    Reload();

    f = ml->file( "media.avi" );
    ASSERT_EQ( f->duration(), d );
}

TEST_F( Files, Snapshot )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_EQ( f->snapshot(), "" );

    std::string newSnapshot( "/path/to/snapshot" );

    f->setSnapshot( newSnapshot );
    ASSERT_EQ( f->snapshot(), newSnapshot );

    Reload();

    f = ml->file( "media.avi" );
    ASSERT_EQ( f->snapshot(), newSnapshot );
}
