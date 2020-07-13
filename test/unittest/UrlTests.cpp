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

#include "gtest/gtest.h"

#include "utils/Url.h"
#include "medialibrary/filesystem/Errors.h"

using namespace medialibrary;

TEST( UrlUtils, encode )
{
    ASSERT_EQ( "meow", utils::url::encode( "meow" ) );
    ASSERT_EQ( "plain%20space", utils::url::encode(  "plain space" ) );
    ASSERT_EQ( "/%C3%A1%C3%A9%C3%BA%C3%AD%C3%B3/f00/%C3%9Far", utils::url::encode( "/áéúíó/f00/ßar" ) );
    ASSERT_EQ( "/file/with%23sharp", utils::url::encode( "/file/with#sharp" ) );
    ASSERT_EQ( "file:///file%20with%20spaces/test.mp4",
               utils::url::encode( "file:///file with spaces/test.mp4" ) );
    ASSERT_EQ( "http://foo:bar@examples.com:1234/h%40ck3rz%3A%20episode2.avi",
               utils::url::encode( "http://foo:bar@examples.com:1234/h@ck3rz: episode2.avi" ) );
    ASSERT_EQ( "http://justhost", utils::url::encode( "http://justhost" ) );
    ASSERT_EQ( "http://@1.2.3.4", utils::url::encode( "http://@1.2.3.4" ) );
    ASSERT_EQ( "http:///invalid.url", utils::url::encode( "http:///invalid.url" ) );
    ASSERT_EQ( "file://%40encodeme%3A/file.mkv", utils::url::encode( "file://@encodeme:/file.mkv" ) );
#ifdef _WIN32
    ASSERT_EQ( "file:///C:/file%3Atest.mkv", utils::url::encode( "file:///C:/file:test.mkv" ) );
    ASSERT_EQ( "file://", utils::url::encode( "file://" ) );
#endif
}

TEST( UrlUtils, stripScheme )
{
  ASSERT_EQ( "space%20marine", utils::url::stripScheme( "sc2://space%20marine" ) );
  ASSERT_THROW( utils::url::stripScheme( "bl%40bla" ), fs::errors::UnhandledScheme );
  ASSERT_EQ( "", utils::url::stripScheme( "vlc://" ) );
  ASSERT_EQ( "leaf/ern/%C3%A7a/pak.one", utils::url::stripScheme( "bteam://leaf/ern/%C3%A7a/pak.one" ) );
  ASSERT_EQ( "/I", utils::url::stripScheme( "file:///I" ) );
}

TEST( UrlUtils, scheme )
{
  ASSERT_EQ( "scheme://", utils::url::scheme( "scheme://on/them/33.spy" ) );
  ASSERT_EQ( "file://", utils::url::scheme( "file:///l/z/4/" ) );
  ASSERT_EQ( "miel://", utils::url::scheme( "miel://nuage.mkv" ) );
  ASSERT_EQ( "://", utils::url::scheme( ":////\\//" ) );
}

TEST( UrlUtils, schemeIs )
{
  ASSERT_TRUE( utils::url::schemeIs( "attachment://", "attachment://" ) );
  ASSERT_TRUE( utils::url::schemeIs( "attachment://", "attachment://picture0.jpg" ) );
  ASSERT_FALSE( utils::url::schemeIs( "boboop://", "/path/to/spaces%20here" ) );
}

TEST( FsUtils, toLocalPath )
{
#ifndef _WIN32
    ASSERT_EQ( "/a/b/c/movie.avi", utils::url::toLocalPath( "file:///a/b/c/movie.avi" ) );
    ASSERT_EQ( "/yea /sp ace", utils::url::toLocalPath( "file:///yea%20/sp%20ace" ) );
    ASSERT_EQ( "/tést/ßóíú/file", utils::url::toLocalPath( "file:///t%C3%A9st/%C3%9F%C3%B3%C3%AD%C3%BA/file" ) );
    ASSERT_EQ( "/&/#/~", utils::url::toLocalPath( "file:///%26/%23/%7E" ) );
    ASSERT_EQ( "/yea /sp ace", utils::url::toLocalPath( "file:///yea%20/sp%20ace" ) );
    ASSERT_EQ( "/c/foo/bar.mkv", utils::url::toLocalPath( "file:///c/foo/bar.mkv" ) );
#else
    ASSERT_EQ( "a\\b\\c\\movie.avi", utils::url::toLocalPath( "file:///a/b/c/movie.avi" ) );
    ASSERT_EQ( "x\\yea \\sp ace", utils::url::toLocalPath( "file:///x/yea%20/sp%20ace" ) );
    ASSERT_EQ( "d\\tést\\ßóíú\\file", utils::url::toLocalPath( "file:///d/t%C3%A9st/%C3%9F%C3%B3%C3%AD%C3%BA/file" ) );
    ASSERT_EQ( "c\\&\\#\\~", utils::url::toLocalPath( "file:///c/%26/%23/%7E" ) );
    ASSERT_EQ( "c\\foo\\bar.mkv", utils::url::toLocalPath( "file:///c/foo/bar.mkv" ) );
    ASSERT_EQ( "x\\yea \\sp ace", utils::url::toLocalPath( "file:///x/yea%20/sp%20ace" ) );
#endif
}

TEST( FsUtils, Path )
{
    ASSERT_EQ( "path/to/file.mkv", utils::url::path( "http://host/path/to/file.mkv" ) );
    ASSERT_EQ( "path/to/file.mkv", utils::url::path( "http://///host/path/to/file.mkv" ) );
    ASSERT_THROW( utils::url::path( "/no/scheme/url" ), fs::errors::UnhandledScheme );
    ASSERT_THROW( utils::url::path( "" ), fs::errors::UnhandledScheme );
}
