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

#include "Tester.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "utils/Filename.h"
#include "utils/Directory.h"

static std::string TestDirectory = SRC_DIR "/test/samples/";
static std::string ForcedTestDirectory;
bool Verbose = false;
bool ExtraVerbose = false;

#ifndef _WIN32
#define TEST_CASE_LIST \
    X("featuring") \
    X("parse_video") \
    X("parse_audio") \
    X("same_album_name_different_artist") \
    X("same_album_name_same_artist") \
    X("compilation") \
    X("compilation_no_albumartist") \
    X("release_year_same") \
    X("notags") \
    X("multi_cd") \
    X("no_album_artist") \
    X("utf8") \
    X("deduce_artwork_from_album") \
    X("deduce_artwork_from_track") \
    X("xiph_embedded_artwork") \
    X("playlist_external_media") \
    X("playlist_external_folder") \
    X("playlist_same_folder") \
    X("playlist_mixed_content") \
    X("same_album_with_subfolder") \
    X("compilation_different_years")


#define REDUCED_TEST_CASE_LIST \
    X("featuring") \
    X("parse_video")

#else
// Disable the parse_video tests on windows for now, since we can't run the
// title analyzer on wine yet.
#define TEST_CASE_LIST \
    X("featuring") \
    X("parse_audio") \
    X("same_album_name_different_artist") \
    X("same_album_name_same_artist") \
    X("compilation") \
    X("compilation_no_albumartist") \
    X("release_year_same") \
    X("notags") \
    X("multi_cd") \
    X("no_album_artist") \
    X("utf8") \
    X("deduce_artwork_from_album") \
    X("deduce_artwork_from_track") \
    X("xiph_embedded_artwork") \
    X("playlist_external_media") \
    X("playlist_external_folder") \
    X("playlist_same_folder") \
    X("playlist_mixed_content") \
    X("same_album_with_subfolder") \
    X("compilation_different_years")

#define REDUCED_TEST_CASE_LIST \
    X("featuring")

#endif

static std::tuple<std::string, bool> testCases[] = {
    #define X(TESTCASE) std::make_tuple( TESTCASE, false ),
    TEST_CASE_LIST
    #undef X
    #define X(TESTCASE) std::make_tuple( TESTCASE, true ),
    TEST_CASE_LIST
    #undef X
};

static std::tuple<std::string, bool> reducedTestCases[] = {
    #define X(TESTCASE) std::make_tuple( TESTCASE, false ),
    REDUCED_TEST_CASE_LIST
    #undef X
    #define X(TESTCASE) std::make_tuple( TESTCASE, true ),
    REDUCED_TEST_CASE_LIST
    #undef X
};

TEST_P( Tests, Parse )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + std::get<0>( GetParam() ) + ".json";
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
    auto lock = m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( m_cb->waitForParsingComplete( lock ) );

    runChecks( doc );
}

TEST_P( ResumeTests, Parse )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + std::get<0>( GetParam() ) + ".json";
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
    auto lock = m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( m_cb->waitForDiscoveryComplete( lock ) );
    auto testMl = static_cast<MediaLibraryResumeTest*>( m_ml.get() );
    testMl->forceParserStart();
    ASSERT_TRUE( m_cb->waitForParsingComplete( lock ) );

    runChecks( doc );
}

TEST_P( ResumeTests, Rescan )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + std::get<0>( GetParam() ) + ".json";
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
    auto lock = m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( m_cb->waitForDiscoveryComplete( lock ) );
    auto testMl = static_cast<MediaLibraryResumeTest*>( m_ml.get() );
    testMl->forceParserStart();
    ASSERT_TRUE( m_cb->waitForParsingComplete( lock ) );

    m_cb->reinit();
    m_ml->forceRescan();
    ASSERT_TRUE( m_cb->waitForParsingComplete( lock ) );

    runChecks( doc );
}

TEST_P( RefreshTests, Refresh )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + std::get<0>( GetParam() ) + ".json";
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
    auto lock = m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( m_cb->waitForDiscoveryComplete( lock ) );
    ASSERT_TRUE( m_cb->waitForParsingComplete( lock ) );

    runChecks( doc );

    m_cb->reinit();
    forceRefresh();

    ASSERT_TRUE( m_cb->waitForParsingComplete( lock ) );

    runChecks( doc );
}

TEST_P( ReducedTests, OverrideExternalMedia )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + std::get<0>( GetParam() ) + ".json";
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
    auto lock = m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        // Fetch all files in the sample directory and insert those as external
        // media before the discovery starts
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );
        auto samplesMrl = utils::file::toMrl( samplesDir );
        auto ml = static_cast<MediaLibrary*>( m_ml.get() );
        auto fsFactory = ml->fsFactoryForMrl( samplesMrl );
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

    ASSERT_TRUE( m_cb->waitForParsingComplete( lock ) );

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

INSTANTIATE_TEST_SUITE_P(SamplesTests, Tests,
                        ::testing::ValuesIn(testCases) );

INSTANTIATE_TEST_SUITE_P(SamplesTests, ReducedTests,
                        ::testing::ValuesIn(reducedTestCases) );

INSTANTIATE_TEST_SUITE_P(SamplesTests, ResumeTests,
                        ::testing::ValuesIn(testCases) );

INSTANTIATE_TEST_SUITE_P(SamplesTests, RefreshTests,
                        ::testing::ValuesIn(testCases) );

