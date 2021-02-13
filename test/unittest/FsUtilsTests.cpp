/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "UnitTests.h"

#include "utils/Filename.h"
#include "medialibrary/filesystem/Errors.h"

using namespace medialibrary;

struct FsUtilsTests
{
    void SetUp() {}
    void TearDown() {}
};

static void extension( FsUtilsTests* )
{
    ASSERT_EQ( "ext", utils::file::extension( "file.ext" ) );
    ASSERT_EQ( "", utils::file::extension( "file." ) );
    ASSERT_EQ( "ext2", utils::file::extension( "file.ext.ext2" ) );
    ASSERT_EQ( "", utils::file::extension( "" ) );
    ASSERT_EQ( "", utils::file::extension( "file.ext." ) );
}

static void directory( FsUtilsTests* )
{
    ASSERT_EQ( "/a/b/c/", utils::file::directory( "/a/b/c/d.e" ) );
    ASSERT_EQ( "/a/b/c/", utils::file::directory( "/a/b/c/" ) );
    ASSERT_EQ( "/a/b/", utils::file::directory( "/a/b/c" ) );
    ASSERT_EQ( "", utils::file::directory( "" ) );
    ASSERT_EQ( "", utils::file::directory( "file.test" ) );
}

static void directoryName( FsUtilsTests* )
{
    ASSERT_EQ( "dé", utils::file::directoryName( "/a/b/c/dé/" ) );
    ASSERT_EQ( ".cache", utils::file::directoryName( "/a/b/c/.cache/" ) );
    ASSERT_EQ( "p17", utils::file::directoryName( "/c/p/p17" ) );
    ASSERT_EQ( ".ssh", utils::file::directoryName( "~/.ssh" ) );
    ASSERT_EQ( "emacs.d", utils::file::directoryName( "/home/blob/emacs.d" ) );
    ASSERT_EQ( "zef", utils::file::directoryName( "zef" ) );
    ASSERT_EQ( "home", utils::file::directoryName( "/home" ) );
    ASSERT_EQ( "", utils::file::directoryName( "/" ) );
    ASSERT_EQ( "", utils::file::directoryName( "" ) );
    ASSERT_EQ( "kill", utils::file::directoryName( "/kill/" ) );
    ASSERT_EQ( "bill", utils::file::directoryName( "bill/" ) );
}

static void fileName( FsUtilsTests* )
{
    ASSERT_EQ( "d.e", utils::file::fileName( "/a/b/c/d.e" ) );
    ASSERT_EQ( "noextfile", utils::file::fileName( "/a/b/noextfile" ) );
    ASSERT_EQ( "file.test", utils::file::fileName( "file.test" ) );
}

static void firstFolder( FsUtilsTests* )
{
    ASSERT_EQ( "f00", utils::file::firstFolder( "f00/bar/" ) );
    ASSERT_EQ( "f00", utils::file::firstFolder( "/f00/bar" ) );
    ASSERT_EQ( "f00", utils::file::firstFolder( "////f00/bar" ) );
    ASSERT_EQ( "f00", utils::file::firstFolder( "/f00/" ) );
    ASSERT_EQ( "f00", utils::file::firstFolder( "f00/" ) );
    ASSERT_EQ( "", utils::file::firstFolder( "/f00" ) );
    ASSERT_EQ( "", utils::file::firstFolder( "" ) );
    ASSERT_EQ( "", utils::file::firstFolder( "/" ) );
    ASSERT_EQ( "", utils::file::firstFolder( "/foo.bar" ) );
}

static void removePath( FsUtilsTests* )
{
    ASSERT_EQ( "bar/", utils::file::removePath( "f00/bar/", "f00" ) );
    ASSERT_EQ( "bar/", utils::file::removePath( "/f00/bar/", "/f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "f00/bar", "f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "/f00/bar", "/f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "////f00/bar", "/f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "/f00///bar", "/f00" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "/f00///bar", "/f00/" ) );
    ASSERT_EQ( "bar", utils::file::removePath( "bar", "" ) );
    ASSERT_EQ( "", utils::file::removePath( "bar/", "bar" ) );
    ASSERT_EQ( "", utils::file::removePath( "/f00/", "/f00/" ) );
    ASSERT_EQ( "", utils::file::removePath( "/f00", "/f00" ) );
    ASSERT_EQ( "", utils::file::removePath( "/f00////", "/f00" ) );
    ASSERT_EQ( "/f00", utils::file::removePath( "/f00", "/path/not/found" ) );
    ASSERT_EQ( "/f00", utils::file::removePath( "/f00", "/loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooongstring/" ) );
}

static void parentFolder( FsUtilsTests* )
{
    ASSERT_EQ( "/a/b/", utils::file::parentDirectory( "/a/b/c/" ) );
    ASSERT_EQ( "/a/b/", utils::file::parentDirectory( "/a/b/c" ) );
    ASSERT_EQ( "", utils::file::parentDirectory( "" ) );
#ifdef _WIN32
    ASSERT_EQ( "C:\\a/b/", utils::file::parentDirectory( "C:\\a/b/c" ) );
    ASSERT_EQ( "C:/a/b/", utils::file::parentDirectory( "C:/a/b/c\\" ) );
    ASSERT_EQ( "C:\\a\\b\\", utils::file::parentDirectory( "C:\\a\\b\\c\\" ) );
    ASSERT_EQ( "C:\\a\\b\\", utils::file::parentDirectory( "C:\\a\\b\\c" ) );
    ASSERT_EQ( "", utils::file::parentDirectory( "C:\\" ) );
    ASSERT_EQ( "", utils::file::parentDirectory( "C:/" ) );
#endif
}

static void toMrl( FsUtilsTests* )
{
#ifndef _WIN32
    ASSERT_EQ( "file:///media/file.mkv", utils::file::toMrl( "/media/file.mkv" ) );
    ASSERT_EQ( "file://", utils::file::toMrl( "" ) );
    ASSERT_EQ( "file:///path%20with%20spaces/file%20.mkv", utils::file::toMrl( "/path with spaces/file .mkv" ) );
#else
    ASSERT_EQ( "file://", utils::file::toMrl( "" ) );
    ASSERT_EQ( "file:///C:/path/to/file.mkv", utils::file::toMrl( "C:\\path\\to/file.mkv" ) );
    ASSERT_EQ( "file:///C:/path/to%3Aa/file%20with%20spaces.mkv", utils::file::toMrl( "C:\\path\\to:a\\file with spaces.mkv" ) );
#endif
}

static void splitPath( FsUtilsTests* )
{
  std::stack<std::string> st_file;

  st_file.push( "[ MACHiN ] 2001 nice movie!.mkv" );
  st_file.push( "films & séries" );
  st_file.push( "léà" );
  st_file.push( "home" );

  auto split = utils::file::splitPath( "/home/léà/films & séries/[ MACHiN ] 2001 nice movie!.mkv", false );
  ASSERT_TRUE( st_file == split );

  std::stack<std::string> st_folder;

  st_folder.push( "Русские песни" );
  st_folder.push( "~" );

  split = utils::file::splitPath( "~/Русские песни/", true );
  ASSERT_TRUE( st_folder == split );
}

static void stripExtension( FsUtilsTests* )
{
    ASSERT_EQ( "seaOtter", utils::file::stripExtension( "seaOtter.mkv" ) );
    ASSERT_EQ( "", utils::file::stripExtension( "" ) );
    ASSERT_EQ( "dummy", utils::file::stripExtension( "dummy" ) );
    ASSERT_EQ( "test.with.dot", utils::file::stripExtension( "test.with.dot.ext" ) );
}

static void ToFolderPath( FsUtilsTests* )
{
    const std::string i1{ "/path/to/folder" };
    auto res = utils::file::toFolderPath( i1 );
    ASSERT_EQ( "/path/to/folder/", res );

    std::string i2{ "/path/to/folder" };
    utils::file::toFolderPath( i2 );
    ASSERT_EQ( "/path/to/folder/", i2 );

#ifdef _WIN32
    const std::string i3{ "/path/to/folder\\" };
    res = utils::file::toFolderPath( i3 );
    ASSERT_EQ( "/path/to/folder/", res );

    std::string i4{ "/path/to/folder\\" };
    utils::file::toFolderPath( i4 );
    ASSERT_EQ( "/path/to/folder/", i4 );
#endif
}

int main( int ac, char** av )
{
    INIT_TESTS_C( FsUtilsTests );

    ADD_TEST( extension );
    ADD_TEST( directory );
    ADD_TEST( directoryName );
    ADD_TEST( fileName );
    ADD_TEST( firstFolder );
    ADD_TEST( removePath );
    ADD_TEST( parentFolder );
    ADD_TEST( toMrl );
    ADD_TEST( splitPath );
    ADD_TEST( stripExtension );
    ADD_TEST( ToFolderPath );

    END_TESTS
}
