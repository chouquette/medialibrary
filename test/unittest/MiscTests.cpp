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
#include "utils/Xml.h"
#include "utils/XxHasher.h"
#include "utils/Date.h"

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
    class TestSqliteConnection : public sqlite::Connection
    {
    public:
        static int testCollate( const char* str1, const char* str2 )
        {
            return sqlite::Connection::collateFilename( nullptr,
                        strlen( str1 ), str1, strlen( str2 ), str2 );
        }
    };
}

struct MiscTests : public Tests
{
    virtual void SetUp( const std::string&, const std::string& ) override
    {
        // No need to setup anything more than the media library instance,
        // those tests are not using the DB, we only need an instance for
        // accessing the supported extensions
        ml.reset( new MediaLibraryTester( "no_such_file", "or_directory", nullptr ) );
    }
    virtual void TearDown() override
    {
        /* Override the test folder directory removal out */
        ml.reset();
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

static void XxHashBuffer( MiscTests* )
{
    auto hash = utils::hash::xxFromBuff( (const uint8_t*)"message digest",
                                         strlen( "message digest" ) );
    ASSERT_EQ( hash, 0x160D8E9329BE94F9u );
    ASSERT_EQ( "160D8E9329BE94F9", utils::hash::toString( hash ) );
}

static void XxHashFile( MiscTests* )
{
    auto hash = utils::hash::xxFromFile( SRC_DIR "/test/unittest/md5_input.bin" );
    ASSERT_EQ( hash, 0x5AF0124E1F8A891u );
    ASSERT_EQ( "5AF0124E1F8A891", utils::hash::toString( hash ) );
}

static void FilenameCollate( MiscTests* )
{
    ASSERT_TRUE( TestSqliteConnection::testCollate( "000001 A", "1 B" ) < 0 );
    ASSERT_TRUE( TestSqliteConnection::testCollate( "", "" ) == 0 );
    ASSERT_TRUE( TestSqliteConnection::testCollate( "A", "" ) > 0 );
    ASSERT_TRUE( TestSqliteConnection::testCollate( "normal string", "another string" ) > 0 );
    ASSERT_TRUE( TestSqliteConnection::testCollate( "123", "321" ) < 0 );
    ASSERT_TRUE( TestSqliteConnection::testCollate( "000123 test", "123 test" ) == 0 );
    ASSERT_TRUE( TestSqliteConnection::testCollate( "02 something.mp3", "3 something else.mp3" ) < 0 );
}

static void CheckTaskDbModel( Tests* T )
{
    auto res = parser::Task::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void ClearDatabaseKeepPlaylist( Tests* T )
{
    auto res = T->ml->clearDatabase( true );
    ASSERT_TRUE( res );
}

static void ClearDatabase( Tests* T )
{
    auto res = T->ml->clearDatabase( false );
    ASSERT_TRUE( res );
}

static void DateFromStr( Tests* )
{
    struct
    {
        const char* str;
        time_t exp;
        bool res;
    } cases[] = {
        { "Wed, 06 Jul 2022 13:58:", 0, false },
        { "Wed, 06 Jul 2022 13:58:45 +", 0, false },
        { "Wed, 06 Jul 2022 13:58:45 +-+", 0, false },
        { "Wed, 06 Jul 2022 13:58:45 +123", 0, false },
        { "Wed, 06 Jul 2022 13:58:45 MEOW", 0, false },
        { "Fri, 08 Jul 2022 13:12:00 GMT",   1657285920, true },
        { "Fri, 08 Jul 2022 13:12:00 +0200", 1657278720, true },
        { "Wed, 02 Oct 2002 08:00:00 EST", 1033563600, true },
    };
    for ( const auto& c : cases )
    {
        struct tm t;
        auto res = utils::date::fromStr( c.str, &t );
        if ( c.res == true )
        {
            ASSERT_TRUE( res );
            auto ts = utils::date::mktime( &t );
            ASSERT_EQ( ts, c.exp );
        }
        else
        {
            ASSERT_FALSE( res );
        }
    }
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
    ADD_TEST( XxHashBuffer );
    ADD_TEST( XxHashFile );
    ADD_TEST( DateFromStr );
    ADD_TEST( FilenameCollate );

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
