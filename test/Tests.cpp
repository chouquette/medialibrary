#include "gtest/gtest.h"

#include "IMediaLibrary.h"

TEST( MediaLibary, Init )
{
    IMediaLibrary* ml = MediaLibraryFactory::create();
    ASSERT_TRUE( ml->initialize( "test.db" ) );
}
