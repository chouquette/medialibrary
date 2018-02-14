/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#include "gtest/gtest.h"

#include "utils/Filename.h"

using namespace medialibrary;

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

TEST( FsUtils, directoryName )
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

TEST( FsUtils, fileName )
{
    ASSERT_EQ( "d.e", utils::file::fileName( "/a/b/c/d.e" ) );
    ASSERT_EQ( "noextfile", utils::file::fileName( "/a/b/noextfile" ) );
    ASSERT_EQ( "file.test", utils::file::fileName( "file.test" ) );
}

TEST( FsUtils, firstFolder )
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

TEST( FsUtils, removePath )
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
    ASSERT_EQ( "/f00", utils::file::removePath( "/f00", "/path/not/found" ) );
}

TEST( FsUtils, parentFolder )
{
    ASSERT_EQ( "/a/b/", utils::file::parentDirectory( "/a/b/c/" ) );
    ASSERT_EQ( "/a/b/", utils::file::parentDirectory( "/a/b/c" ) );
    ASSERT_EQ( "", utils::file::parentDirectory( "" ) );
#ifdef _WIN32
    ASSERT_EQ( "C:\\a/b/", utils::file::parentDirectory( "C:\\a/b/c" ) );
    ASSERT_EQ( "C:/a/b/", utils::file::parentDirectory( "C:/a/b/c\\" ) );
    ASSERT_EQ( "C:\\a\\b\\", utils::file::parentDirectory( "C:\\a\\b\\c\\" ) );
    ASSERT_EQ( "C:\\a\\b\\", utils::file::parentDirectory( "C:\\a\\b\\c" ) );
#endif
}

TEST( FsUtils, toLocalPath )
{
    ASSERT_EQ( "/a/b/c/movie.avi", utils::file::toLocalPath( "file:///a/b/c/movie.avi" ) );
    ASSERT_EQ( "/yea /sp ace", utils::file::toLocalPath( "file:///yea%20/sp%20ace" ) );
    ASSERT_EQ( "/tést/ßóíú/file", utils::file::toLocalPath( "file:///t%C3%A9st/%C3%9F%C3%B3%C3%AD%C3%BA/file" ) );
    ASSERT_EQ( "/&/#/~", utils::file::toLocalPath( "file:///%26/%23/%7E" ) );
}

TEST( FsUtils, toPath )
{
  ASSERT_EQ( "road/to/raw.pcm", utils::file::toPath( "https://road/to/raw.pcm" ) );
  ASSERT_EQ( "space cowboy", utils::file::toPath( "bebop://space%20cowboy" ) );
  ASSERT_EQ( "/colt/caßeras", utils::file::toPath( "France:///colt/ca%C3%9Feras" ) );
  ASSERT_EQ( "", utils::file::toPath( "boom://" ) );
  ASSERT_EQ( "/", utils::file::toPath( "boop:///" ) );
}

TEST( FsUtils, stripScheme )
{
  ASSERT_EQ( "space%20marine", utils::file::stripScheme( "sc2://space%20marine" ) );
  ASSERT_EQ( "bl%40bla", utils::file::stripScheme( "bl%40bla" ) );
  ASSERT_EQ( "", utils::file::stripScheme( "vlc://" ) );
  ASSERT_EQ( "leaf/ern/%C3%A7a/pak.one", utils::file::stripScheme( "bteam://leaf/ern/%C3%A7a/pak.one" ) );
  ASSERT_EQ( "/I", utils::file::stripScheme( "file:///I" ) );
}

TEST( FsUtils, scheme )
{
  ASSERT_EQ( "scheme://", utils::file::scheme( "scheme://on/them/33.spy" ) );
  ASSERT_EQ( "file://", utils::file::scheme( "file:///l/z/4/" ) );
  ASSERT_EQ( "miel://", utils::file::scheme( "miel://nuage.mkv" ) );
  ASSERT_EQ( "://", utils::file::scheme( ":////\\//" ) );
}

TEST( FsUtils, schemeIs )
{
  ASSERT_TRUE( utils::file::schemeIs( "attachment://", "attachment://" ) );
  ASSERT_TRUE( utils::file::schemeIs( "attachment://", "attachment://picture0.jpg" ) );
  ASSERT_FALSE( utils::file::schemeIs( "boboop://", "/path/to/spaces%20here" ) );
}

TEST( FsUtils, splitPath )
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

TEST( FsUtils, stripExtension )
{
    ASSERT_EQ( "seaOtter", utils::file::stripExtension( "seaOtter.mkv" ) );
    ASSERT_EQ( "", utils::file::stripExtension( "" ) );
    ASSERT_EQ( "dummy", utils::file::stripExtension( "dummy" ) );
    ASSERT_EQ( "test.with.dot", utils::file::stripExtension( "test.with.dot.ext" ) );
}
