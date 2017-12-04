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

#include "Tester.h"

static std::string TestDirectory = SRC_DIR "/test/samples/";
static std::string ForcedTestDirectory;
bool Verbose = false;
bool ExtraVerbose = false;

static const char* testCases[] = {
    "featuring",
    "parse_video",
    "parse_audio",
    "same_album_name_different_artist",
    "same_album_name_same_artist",
    "compilation",
    "compilation_no_albumartist",
    "release_year_same",
    "notags",
    "multi_cd",
    "no_album_artist",
    "utf8",
    "deduce_artwork_from_album",
    "deduce_artwork_from_track",
    "xiph_embedded_artwork",
    "same_album_with_subfolder",
};

class TestEnv : public ::testing::Environment
{
    public:
        virtual void SetUp()
        {
            // Always clean the DB in case a previous test crashed
            unlink("test.db");
        }
};

TEST_P( Tests, Parse )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + GetParam() + ".json";
    std::unique_ptr<FILE, int(*)(FILE*)> f( fopen( casePath.c_str(), "rb" ), &fclose );
    ASSERT_NE( nullptr, f );
    char buff[65536]; // That's how ugly I am!
    auto ret = fread( buff, sizeof(buff[0]), sizeof(buff), f.get() );
    ASSERT_NE( 0u, ret );
    buff[ret] = 0;
    rapidjson::Document doc;
    doc.Parse( buff );

    ASSERT_TRUE( doc.HasMember( "input" ) );
    const auto& input = doc["input"];
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        struct stat s;
        auto res = stat( samplesDir.c_str(), &s );
        ASSERT_EQ( 0, res );

        m_ml->discover( "file://" + samplesDir );
    }
    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    if ( doc.HasMember( "expected" ) == false )
    {
        // That's a lousy test case with no assumptions, but ok.
        return;
    }
    const auto& expected = doc["expected"];

    if ( expected.HasMember( "albums" ) == true )
    {
        checkAlbums( expected["albums" ], m_ml->albums( SortingCriteria::Default, false ) );
    }
    if ( expected.HasMember( "media" ) == true )
        checkMedias( expected["media"] );
    if ( expected.HasMember( "nbVideos" ) == true )
    {
        const auto videos = m_ml->videoFiles( SortingCriteria::Default, false );
        ASSERT_EQ( expected["nbVideos"].GetUint(), videos.size() );
    }
    if ( expected.HasMember( "nbAudios" ) == true )
    {
        const auto audios = m_ml->audioFiles( SortingCriteria::Default, false );
        ASSERT_EQ( expected["nbAudios"].GetUint(), audios.size() );
    }
    if ( expected.HasMember( "artists" ) )
    {
        checkArtists( expected["artists"], m_ml->artists( SortingCriteria::Default, false ) );
    }
}

int main(int ac, char** av)
{
    ::testing::InitGoogleTest(&ac, av);
    const std::string verboseArg = "-v";
    const std::string extraVerboseArg = "-vv";
    const std::string forcedTestDir = "--testdir";
    for ( auto i = 1; i < ac; ++i )
    {
        if ( av[i] == verboseArg )
            Verbose = true;
        else if ( av[i] == extraVerboseArg )
            ExtraVerbose = true;
        else if ( av[i] == forcedTestDir )
        {
            assert(i + 1 < ac);
            ForcedTestDirectory = av[i + 1];
            ++i;
        }
    }
    return RUN_ALL_TESTS();
}

INSTANTIATE_TEST_CASE_P(SamplesTests, Tests,
                        ::testing::ValuesIn(testCases) );

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);
