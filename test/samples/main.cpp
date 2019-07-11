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
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "utils/Filename.h"
#include "utils/Directory.h"

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
    "playlist_external_media",
    "playlist_external_folder",
    "playlist_same_folder",
    "same_album_with_subfolder",
    "compilation_different_years"
};

static const char* reducedTestCases[] = {
    "featuring",
    "parse_video",
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
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    runChecks( doc );
}

TEST_P( ResumeTests, Parse )
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
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( m_cb->waitForDiscoveryComplete() );
    auto testMl = static_cast<MediaLibraryResumeTest*>( m_ml.get() );
    testMl->forceParserStart();
    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    runChecks( doc );
}

TEST_P( ResumeTests, Rescan )
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
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( m_cb->waitForDiscoveryComplete() );
    auto testMl = static_cast<MediaLibraryResumeTest*>( m_ml.get() );
    testMl->forceParserStart();
    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    m_cb->reinit();
    m_ml->forceRescan();
    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    runChecks( doc );
}

TEST_P( RefreshTests, Refresh )
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
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( m_cb->waitForDiscoveryComplete() );
    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    runChecks( doc );

    m_cb->reinit();
    forceRefresh();

    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    runChecks( doc );
}

TEST_P( ReducedTests, OverrideExternalMedia )
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
    auto nbMedia = 0u;
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        // Fetch all files in the sample directory and insert those as external
        // media before the discovery starts
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );
        auto samplesMrl = utils::file::toMrl( samplesDir );
        auto fsFactory = m_ml->fsFactoryForMrl( samplesMrl );
        auto dir = fsFactory->createDirectory( samplesMrl );
        auto files = dir->files();
        for ( const auto& f : files )
        {
            auto media = m_ml->addExternalMedia( f->mrl() );
            ASSERT_NE( nullptr, media );
            ASSERT_EQ( IMedia::Type::External, media->type() );
            ASSERT_EQ( -1, media->duration() );
            ++nbMedia;
        }

        m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_NE( 0u, nbMedia );
    auto folders = m_ml->folders( IMedia::Type::Unknown )->all();
    ASSERT_EQ( 0u, folders.size() );

    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    for ( auto i = 1u; i <= nbMedia; ++i )
    {
        auto media = m_ml->media( i );
        ASSERT_NE( nullptr, media );
        ASSERT_NE( IMedia::Type::External, media->type() );
        ASSERT_TRUE( media->type() == IMedia::Type::Audio ||
                     media->type() == IMedia::Type::Video );
        ASSERT_NE( -1, media->duration() );
    }
    // Check that the media are correctly linked with their folders
    folders = m_ml->folders( IMedia::Type::Unknown )->all();
    ASSERT_NE( 0u, folders.size() );
    auto nbMediaByFolders = 0u;
    for ( auto i = 0u; i < folders.size(); ++i )
    {
        nbMediaByFolders += folders[i]->media( IMedia::Type::Unknown, nullptr )->count();
    }
    ASSERT_EQ( nbMedia, nbMediaByFolders );
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

INSTANTIATE_TEST_CASE_P(SamplesTests, ReducedTests,
                        ::testing::ValuesIn(reducedTestCases) );

INSTANTIATE_TEST_CASE_P(SamplesTests, ResumeTests,
                        ::testing::ValuesIn(testCases) );

INSTANTIATE_TEST_CASE_P(SamplesTests, RefreshTests,
                        ::testing::ValuesIn(testCases) );

