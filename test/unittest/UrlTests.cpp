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

#include "utils/Url.h"
#include "medialibrary/filesystem/Errors.h"

using namespace medialibrary;

static void encode( Tests* )
{
    ASSERT_EQ( "meow", utils::url::encode( "meow" ) );
    ASSERT_EQ( "plain%20space", utils::url::encode(  "plain space" ) );
    ASSERT_EQ( "/%C3%A1%C3%A9%C3%BA%C3%AD%C3%B3/f00/%C3%9Far", utils::url::encode( "/áéúíó/f00/ßar" ) );
    ASSERT_EQ( "/file/with%23sharp", utils::url::encode( "/file/with#sharp" ) );
    ASSERT_EQ( "file:///file%20with%20spaces/test.mp4",
               utils::url::encode( "file:///file with spaces/test.mp4" ) );
    ASSERT_EQ( "http://foo:bar@examples.com:1234/h@ck3rz:%20episode2.avi",
               utils::url::encode( "http://foo:bar@examples.com:1234/h@ck3rz: episode2.avi" ) );
    ASSERT_EQ( "http://justhost", utils::url::encode( "http://justhost" ) );
    ASSERT_EQ( "http://@1.2.3.4", utils::url::encode( "http://@1.2.3.4" ) );
    ASSERT_EQ( "http:///invalid.url", utils::url::encode( "http:///invalid.url" ) );
    ASSERT_EQ( "file://%40encodeme%3A/file.mkv", utils::url::encode( "file://@encodeme:/file.mkv" ) );
    ASSERT_EQ( "http://host/path/to?/file.mkv?param=value", utils::url::encode( "http://host/path/to?/file.mkv?param=value" ) );
    ASSERT_EQ( "file:///path/to%3F/file.mkv%3Fparam%3Dvalue", utils::url::encode( "file:///path/to?/file.mkv?param=value" ) );
#ifdef _WIN32
    ASSERT_EQ( "file:///C:/file%3Atest.mkv", utils::url::encode( "file:///C:/file:test.mkv" ) );
    ASSERT_EQ( "file://", utils::url::encode( "file://" ) );
    ASSERT_EQ( "file:///C", utils::url::encode( "file:///C" ) );
#endif
}

static void stripScheme( Tests* )
{
  ASSERT_EQ( "space%20marine", utils::url::stripScheme( "sc2://space%20marine" ) );
  ASSERT_THROW( utils::url::stripScheme( "bl%40bla" ), fs::errors::UnhandledScheme );
  ASSERT_EQ( "", utils::url::stripScheme( "vlc://" ) );
  ASSERT_EQ( "leaf/ern/%C3%A7a/pak.one", utils::url::stripScheme( "bteam://leaf/ern/%C3%A7a/pak.one" ) );
  ASSERT_EQ( "/I", utils::url::stripScheme( "file:///I" ) );
}

static void scheme( Tests* )
{
  ASSERT_EQ( "scheme://", utils::url::scheme( "scheme://on/them/33.spy" ) );
  ASSERT_EQ( "file://", utils::url::scheme( "file:///l/z/4/" ) );
  ASSERT_EQ( "miel://", utils::url::scheme( "miel://nuage.mkv" ) );
  ASSERT_EQ( "://", utils::url::scheme( ":////\\//" ) );
}

static void schemeIs( Tests* )
{
  ASSERT_TRUE( utils::url::schemeIs( "attachment://", "attachment://" ) );
  ASSERT_TRUE( utils::url::schemeIs( "attachment://", "attachment://picture0.jpg" ) );
  ASSERT_FALSE( utils::url::schemeIs( "boboop://", "/path/to/spaces%20here" ) );
}

static void Split( Tests* )
{
    auto test = [](const std::string& url, const std::string& scheme,
                   const std::string& userInfo, const std::string& host,
                   const std::string& port, const std::string& path,
                   const std::string& query, const std::string& fragments) {
        auto p = utils::url::split( url );
        ASSERT_EQ( scheme, p.scheme );
        ASSERT_EQ( userInfo, p.userInfo );
        ASSERT_EQ( host, p.host );
        ASSERT_EQ( port, p.port );
        ASSERT_EQ( path, p.path );
        ASSERT_EQ( query, p.query );
        ASSERT_EQ( fragments, p.fragments );
    };
    test( "file:///path/to/file", "file", "", "", "",  "/path/to/file", "", "" );
    test( "/path/to/file", "", "", "", "", "/path/to/file", "", "" );
    test( "foo://example.com:8042/over/there?name=ferret#nose", "foo",
          "", "example.com", "8042", "/over/there", "name=ferret", "nose" );
    test( "otter:///?#foo", "otter", "", "", "", "/", "", "foo" );
    test( "otter:///?#", "otter", "", "", "", "/", "", "" );
    test( "otter:///path/to/file#anchor", "otter", "", "", "", "/path/to/file", "", "anchor" );
    test( "", "", "", "", "", "", "", "" );
    test( "scheme://", "scheme", "", "", "", "", "", "" );
    test( "p://u:p@host:123/a/b/c?o=v", "p", "u:p", "host", "123", "/a/b/c", "o=v", "" );
    test( "protocol://john:doe@1.2.3.4:567", "protocol", "john:doe",
          "1.2.3.4", "567", "", "", "" );
    test( "scheme://host:80#foo", "scheme", "", "host", "80", "", "", "foo" );
    test( "scheme://@host:80#foo", "scheme", "", "host", "80", "", "", "foo" );
    test( "scheme://@host:#foo", "scheme", "", "host", "", "", "", "foo" );
    test( "smb://útf8_hò§t/#fôõ", "smb", "", "útf8_hò§t", "", "/", "", "fôõ" );
    test( "scheme://foo:bar@baz?query", "scheme", "foo:bar", "baz", "", "", "query", "" );
    test( "scheme://foo?bar/", "scheme", "", "foo", "", "", "bar/", "" );
    test( "scheme://foo#bar/", "scheme", "", "foo", "", "", "", "bar/" );
    test( "scheme://foo#bar?/", "scheme", "", "foo", "", "", "", "bar?/" );
}

static void toLocalPath( Tests* )
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

static void Path( Tests* )
{
    ASSERT_EQ( "path/to/file.mkv", utils::url::path( "http://host/path/to/file.mkv" ) );
    ASSERT_EQ( "path/to/file.mkv", utils::url::path( "http://///host/path/to/file.mkv" ) );
    ASSERT_THROW( utils::url::path( "/no/scheme/url" ), fs::errors::UnhandledScheme );
    ASSERT_THROW( utils::url::path( "" ), fs::errors::UnhandledScheme );
}

static void Decode( Tests* )
{
    ASSERT_EQ( "\"url\" !benchmark# with sp€ci@l c!!$#%aracters",
               utils::url::decode( "%22url%22%20%21benchmark%23%20with%20sp%E2%82%ACci%40l%20c%21%21%24%23%25aracters" ) );
    ASSERT_THROW( utils::url::decode( "%%%%" ), std::runtime_error );
    ASSERT_THROW( utils::url::decode( "%" ), std::runtime_error );
    ASSERT_EQ( "", utils::url::decode( "" ) );
}

int main( int ac, char** av )
{
    INIT_TESTS(Url);

    ADD_TEST( encode );
    ADD_TEST( stripScheme );
    ADD_TEST( scheme );
    ADD_TEST( schemeIs );
    ADD_TEST( Split );
    ADD_TEST( toLocalPath );
    ADD_TEST( Path );
    ADD_TEST( Decode );

    END_TESTS;
}
