#include "gtest/gtest.h"

#include "Utils.h"

TEST( FsUtils, extension )
{
    ASSERT_EQ( "ext", utils::file::extension( "file.ext" ) );
    ASSERT_EQ( "", utils::file::extension( "file." ) );
    ASSERT_EQ( "ext2", utils::file::extension( "file.ext.ext2" ) );
    ASSERT_EQ( "", utils::file::extension( "" ) );
    ASSERT_EQ( "", utils::file::extension( "file.ext." ) );
}

TEST( FsUtils, directory )
{
    ASSERT_EQ( "/a/b/c/", utils::file::directory( "/a/b/c/d.e" ) );
    ASSERT_EQ( "", utils::file::directory( "" ) );
    ASSERT_EQ( "", utils::file::directory( "file.test" ) );
}

TEST( FsUtils, fileName )
{
    ASSERT_EQ( "d.e", utils::file::fileName( "/a/b/c/d.e" ) );
    ASSERT_EQ( "noextfile", utils::file::fileName( "/a/b/noextfile" ) );
    ASSERT_EQ( "file.test", utils::file::fileName( "file.test" ) );
}
