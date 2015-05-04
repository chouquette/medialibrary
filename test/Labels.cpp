#include "Tests.h"

#include "IMediaLibrary.h"
#include "IFile.h"
#include "ILabel.h"

class Labels : public Tests
{
};

TEST_F( Labels, Add )
{
    auto f = ml->addFile( "/dev/null" );
    auto l1 = ml->createLabel( "sea otter" );
    auto l2 = ml->createLabel( "cony the cone" );

    ASSERT_NE( l1, nullptr);
    ASSERT_NE( l2, nullptr);

    auto labels = f->labels();
    ASSERT_EQ( labels.size(), 0u );

    f->addLabel( l1 );
    f->addLabel( l2 );

    labels = f->labels();

    ASSERT_EQ( labels.size(), 2u );
    ASSERT_EQ( labels[0]->name(), "sea otter" );
    ASSERT_EQ( labels[1]->name(), "cony the cone" );
}

TEST_F( Labels, Remove )
{
    auto f = ml->addFile( "/dev/null" );
    auto l1 = ml->createLabel( "sea otter" );
    auto l2 = ml->createLabel( "cony the cone" );

    f->addLabel( l1 );
    f->addLabel( l2 );

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

TEST_F( Labels, Files )
{
    auto f = ml->addFile( "/dev/null" );
    auto f2 = ml->addFile( "/dev/moulaf" );
    auto f3 = ml->addFile( "/sea/otter" );

    auto l1 = ml->createLabel( "label1" );
    auto l2 = ml->createLabel( "label2" );

    f->addLabel( l1 );
    f2->addLabel( l2 );
    f3->addLabel( l1 );

    auto label1Files = l1->files();
    auto label2Files = l2->files();

    ASSERT_EQ( label1Files.size(), 2u );
    ASSERT_EQ( label2Files.size(), 1u );

    ASSERT_EQ( label2Files[0], f2 );

    for (auto labelFile : label1Files )
    {
        ASSERT_TRUE( labelFile == f || labelFile == f3 );
    }
}

TEST_F( Labels, Delete )
{
    auto f = ml->addFile( "/dev/null" );
    auto l1 = ml->createLabel( "sea otter" );
    auto l2 = ml->createLabel( "cony the cone" );

    f->addLabel( l1 );
    f->addLabel( l2 );

    auto labels = f->labels();
    ASSERT_EQ( labels.size(), 2u );

    ml->deleteLabel( "sea otter" );
    labels = f->labels();
    ASSERT_EQ( labels.size(), 1u );

    ml->deleteLabel( l2 );
    labels = f->labels();
    ASSERT_EQ( labels.size(), 0u );

    // Nothing to delete anymore, this should fail gracefuly
    bool res = ml->deleteLabel( l1 );
    ASSERT_FALSE( res );
}

