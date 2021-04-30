/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2017-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "common/util.h"
#include "database/SqliteTools.h"
#include "database/SqliteConnection.h"
#include "utils/Strings.h"
#include "utils/Defer.h"
#include "utils/Md5.h"
#include "utils/Xml.h"

#include "parser/Task.h"

namespace
{
    bool checkAlphaOrderedVector( const std::vector<const char*>& in )
    {
        for ( auto i = 0u; i < in.size() - 1; i++ )
        {
            if ( strcmp( in[i], in[i + 1] ) >= 0 )
                return false;
        }
        return true;
    }
}

struct MiscTests : public Tests
{
    virtual void SetUp() override
    {
        // No need to setup anything more than the media library instance,
        // those tests are not using the DB, we only need an instance for
        // accessing the supported extensions
        ml.reset( new MediaLibraryTester( "no_such_file", "or_directory" ) );
    }
};

static void MediaExtensions( MiscTests* T )
{
    const auto supportedExtensions = T->ml->supportedMediaExtensions();
    auto res = checkAlphaOrderedVector( supportedExtensions );
    ASSERT_TRUE( res );
}

static void PlaylistExtensions( MiscTests* T )
{
    const auto supportedExtensions = T->ml->supportedPlaylistExtensions();
    auto res = checkAlphaOrderedVector( supportedExtensions );
    ASSERT_TRUE( res );
}

static void SubtitleExtensions( MiscTests* T )
{
    const auto supportedExtensions = T->ml->supportedSubtitleExtensions();
    auto res = checkAlphaOrderedVector( supportedExtensions );
    ASSERT_TRUE( res );
}

static void TrimString( MiscTests* )
{
    ASSERT_EQ( utils::str::trim( "hello world" ), "hello world" );
    ASSERT_EQ( utils::str::trim( "  spaaaaaace   " ), "spaaaaaace" );
    ASSERT_EQ( utils::str::trim( "\tfluffy\notters  \t\n" ), "fluffy\notters" );
    ASSERT_EQ( utils::str::trim( "    " ), "" );
    ASSERT_EQ( utils::str::trim( "" ), "" );
}

static void SanitizePattern( MiscTests* )
{
    // "" will become " "" "" *", (without spaces) as all double quotes are
    // escaped, and the pattern itself is enclosed between " *"
    ASSERT_EQ( "\"\"\"\"\"*\"", sqlite::Tools::sanitizePattern( "\"\"" ) );
    ASSERT_EQ( "\"Little Bobby Table*\"", sqlite::Tools::sanitizePattern( "Little Bobby Table" ) );
    ASSERT_EQ( "\"Test \"\" Pattern*\"", sqlite::Tools::sanitizePattern( "Test \" Pattern" ) );
    ASSERT_EQ( "\"It''s a test*\"", sqlite::Tools::sanitizePattern( "It's a test" ) );
    ASSERT_EQ( "\"''\"\"*\"", sqlite::Tools::sanitizePattern( "\'\"" ) );
}

static void Utf8NbChars( MiscTests* )
{
    ASSERT_EQ( 0u, utils::str::utf8::nbChars( "" ) );
    ASSERT_EQ( 5u, utils::str::utf8::nbChars( "ABCDE" ) );
    ASSERT_EQ( 7u, utils::str::utf8::nbChars( "NEO지식창고" ) );
    ASSERT_EQ( 0u, utils::str::utf8::nbChars( "INVALID\xC3" ) );
    ASSERT_EQ( 0u, utils::str::utf8::nbChars( "\xEC\xEC" ) );
}

static void Utf8NbBytes( MiscTests* )
{
    ASSERT_EQ( 5u, utils::str::utf8::nbBytes( "ABCDE", 0, 5 ) );
    ASSERT_EQ( 0u, utils::str::utf8::nbBytes( "ABCDE", 0, 0 ) );
    ASSERT_EQ( 5u, utils::str::utf8::nbBytes( "ABCDE", 0, 999 ) );
    ASSERT_EQ( 4u, utils::str::utf8::nbBytes( "ABCDéFG", 4, 3 ) );

    ASSERT_EQ( 15u, utils::str::utf8::nbBytes( "NEO지식창고", 0, 7 ) );
    ASSERT_EQ( 12u, utils::str::utf8::nbBytes( "NEO지식창고", 0, 6 ) );

    ASSERT_EQ( 0u, utils::str::utf8::nbBytes( "INVALID\xC3", 5, 3 ) );
    ASSERT_EQ( 0u, utils::str::utf8::nbBytes( "\xEC\xEC", 5, 3 ) );
}

static void XmlEncode( MiscTests* )
{
    ASSERT_EQ( "1 &lt; 2", utils::xml::encode( "1 < 2" ) );
    ASSERT_EQ( "2 &gt; 1", utils::xml::encode( "2 > 1" ) );
    ASSERT_EQ( "&apos;test&apos; &amp; &quot;double test&quot;",
               utils::xml::encode( "'test' & \"double test\"" ) );
}

static void Defer( MiscTests* )
{
    auto i = 0u;
    bool set = false;
    {
        auto d = utils::make_defer( [&i, &set]() {
            ++i;
            set = true;
        });
        ASSERT_FALSE( set );
        ASSERT_EQ( 0u, i );
    }
    ASSERT_TRUE( set );
    ASSERT_EQ( 1u, i );
}

static void Md5Buffer( MiscTests* )
{
    ASSERT_EQ( "F96B697D7CB7938D525A2F31AAF161D0",
               utils::Md5Hasher::fromBuff( (const uint8_t*)"message digest",
                                                strlen( "message digest" ) ) );
}

static void Md5File( MiscTests* )
{
    ASSERT_EQ( "5BF2922FB1FF5800D533AE34785953F1",
               utils::Md5Hasher::fromFile( SRC_DIR "/test/unittest/md5_input.bin" ) );
}

static void CheckTaskDbModel( Tests* T )
{
    auto res = parser::Task::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void ClearDatabaseKeepPlaylist( Tests* T )
{
    T->ml->clearDatabase( true );
}

static void ClearDatabase( Tests* T )
{
    T->ml->clearDatabase( false );
}

int test_without_ml_init( int ac, char** av )
{
    INIT_TESTS_C( MiscTests );

    ADD_TEST( MediaExtensions );
    ADD_TEST( PlaylistExtensions );
    ADD_TEST( SubtitleExtensions );
    ADD_TEST( TrimString );
    ADD_TEST( SanitizePattern );
    ADD_TEST( Utf8NbChars );
    ADD_TEST( Utf8NbBytes );
    ADD_TEST( XmlEncode );
    ADD_TEST( Defer );
    ADD_TEST( Md5Buffer );
    ADD_TEST( Md5File );

    END_TESTS
}

int test_with_ml_init( int ac, char** av )
{
    INIT_TESTS(MiscTests)

    ADD_TEST( CheckTaskDbModel );
    ADD_TEST( ClearDatabaseKeepPlaylist );
    ADD_TEST( ClearDatabase );

    END_TESTS
}

int main( int ac, char** av )
{
    if ( test_without_ml_init( ac, av ) == 0 ||
           test_with_ml_init( ac, av ) == 0 )
        return 0;
    return 1;
}
