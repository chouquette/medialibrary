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
#include "Playlist.h"
#include "Media.h"

#include <vlcpp/vlc.hpp>

static std::string TestDirectory = SRC_DIR "/test/samples/";
static std::string ForcedTestDirectory;

static void Parse( Tests* T, const std::string& testFile )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + testFile + ".json";
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
    auto lock = T->m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        T->m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( lock ) );

    T->runChecks( doc );
}

static void ParseTwice( Tests* T, const std::string& testFile )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + testFile + ".json";
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
    auto lock = T->m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        T->m_ml->discover( utils::file::toMrl( samplesDir ) );
    }

    ASSERT_TRUE( T->m_cb->waitForParsingComplete( lock ) );

    T->runChecks( doc );

    for ( auto i = 0u; i < input.Size(); ++i )
    {
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        samplesDir = utils::fs::toAbsolute( samplesDir );
        T->m_ml->removeEntryPoint( utils::file::toMrl( samplesDir ) );
    }

    ASSERT_TRUE( T->m_cb->waitForRemovalComplete( lock ) );

    for ( auto i = 0u; i < input.Size(); ++i )
    {
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        samplesDir = utils::fs::toAbsolute( samplesDir );
        T->m_ml->discover( utils::file::toMrl( samplesDir ) );
    }

    ASSERT_TRUE( T->m_cb->waitForParsingComplete( lock ) );

    T->runChecks( doc );
}

static void RunResumeTests( ResumeTests* T, const std::string& testFile )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + testFile + ".json";
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
    auto lock = T->m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        T->m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( T->m_cb->waitForDiscoveryComplete( lock ) );
    auto testMl = static_cast<MediaLibraryResumeTest*>( T->m_ml.get() );
    testMl->forceParserStart();
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( lock ) );

    T->runChecks( doc );
}

static void Rescan( ResumeTests* T, const std::string& testFile )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + testFile + ".json";
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
    auto lock = T->m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        T->m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( T->m_cb->waitForDiscoveryComplete( lock ) );
    auto testMl = static_cast<MediaLibraryResumeTest*>( T->m_ml.get() );
    testMl->forceParserStart();
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( lock ) );

    T->m_cb->reinit();
    T->m_ml->forceRescan();
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( lock ) );

    T->runChecks( doc );
}

static void RunRefreshTests( RefreshTests* T, const std::string& testFile )
{
    auto testDir = ForcedTestDirectory.empty() == false ? ForcedTestDirectory : TestDirectory;
    auto casePath = testDir + "testcases/" + testFile + ".json";
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
    auto lock = T->m_cb->lock();
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = testDir + "samples/" + input[i].GetString();
        ASSERT_TRUE( utils::fs::isDirectory( samplesDir ) );
        samplesDir = utils::fs::toAbsolute( samplesDir );

        T->m_ml->discover( utils::file::toMrl( samplesDir ) );
    }
    ASSERT_TRUE( T->m_cb->waitForDiscoveryComplete( lock ) );
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( lock ) );

    T->runChecks( doc );

    T->m_cb->reinit();
    T->forceRefresh();

    ASSERT_TRUE( T->m_cb->waitForParsingComplete( lock ) );

    T->runChecks( doc );
}

static void RunBackupRestorePlaylist( Tests* T, const std::string& )
{
    auto lock = T->m_cb->lock();
    auto samplesFolder = std::string{ SRC_DIR "/test/samples/samples/playlist/tracks" };
    ASSERT_TRUE( utils::fs::isDirectory( samplesFolder ) );
    samplesFolder = utils::fs::toAbsolute( samplesFolder );
    T->m_ml->discover( utils::file::toMrl( samplesFolder ) );
    auto res = T->m_cb->waitForParsingComplete( lock );
    ASSERT_TRUE( res );
    // Now we should have discovered some media

    auto media = T->m_ml->audioFiles( nullptr )->all();
    ASSERT_EQ( 3u, media.size() );

    auto pl1 = std::static_pointer_cast<Playlist>( T->m_ml->createPlaylist( "Exported Playlist 1" ) );
    auto m1 = media[0];
    auto m2 = media[1];
    auto m3 = T->m_ml->addExternalMedia( "http://example.org/sea&ottér.avi", -1 );
    pl1->append( *m1 );
    pl1->append( *m2 );
    pl1->append( *m3 );

    auto pl2 = std::static_pointer_cast<Playlist>( T->m_ml->createPlaylist( "Exported Playlist <2>" ) );
    pl2->append( *m3 );
    pl2->append( *m2 );
    pl2->append( *m1 );

    auto backup = Playlist::backupPlaylists( static_cast<MediaLibrary*>( T->m_ml.get() ),
                                             Settings::DbModelVersion );
    ASSERT_TRUE( std::get<0>( backup ) );

    T->m_cb->prepareForPlaylistReload();
    T->m_ml->clearDatabase( true );

    res = T->m_cb->waitForPlaylistReload( lock );
    ASSERT_TRUE( res );

    auto playlists = T->m_ml->playlists( nullptr )->all();
    ASSERT_EQ( 2u, playlists.size() );
    auto playlist1 = playlists[0];
    media = playlist1->media( nullptr )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( "Exported Playlist 1", playlist1->name() );

    auto playlist2 = playlists[1];
    media = playlist2->media( nullptr )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( "Exported Playlist <2>", playlist2->name() );

    /*
     * Since the folder isn't discovered yet, the media won't be preparsed and
     * won't have their duration or title.
     * However if we discover those media again, the media should be analyzed and
     * converted back to internal media, meaning they'll recover their titles
     * and duration among other information.
     */
    T->m_ml->discover( utils::file::toMrl( samplesFolder ) );
    res = T->m_cb->waitForParsingComplete( lock );
    ASSERT_TRUE( res );
    media = playlist1->media( nullptr )->all();
    ASSERT_EQ( m1->title(), media[0]->title() );
    ASSERT_EQ( m2->title(), media[1]->title() );
    ASSERT_EQ( m3->title(), media[2]->title() );

    media = playlist2->media( nullptr )->all();
    ASSERT_EQ( m3->title(), media[0]->title() );
    ASSERT_EQ( m2->title(), media[1]->title() );
    ASSERT_EQ( m1->title(), media[2]->title() );
}

#define RUN_TEST( Type, Func ) \
    auto T = std::make_unique<Type>(); \
    T->SetUp(); \
    Func( T.get(), testName ); \

int main(int ac, char** av)
{
    const std::string forcedTestDir = "--testdir";
    std::string testType;
    std::string testName;
    for ( auto i = 1; i < ac; ++i )
    {
        if ( av[i] == forcedTestDir )
        {
            assert(i + 1 < ac);
            ForcedTestDirectory = av[i + 1];
            ++i;
        }
        else
        {
            testType = av[i];
            testName = av[i + 1];
            assert( i + 2 == ac && "Invalid number of arguments" );
            break;
        }
    }
    if ( testType == "Parse" )
    {
        RUN_TEST( Tests, Parse );
    }
    else if ( testType == "ParseTwice" )
    {
        RUN_TEST( Tests, ParseTwice );
    }
    else if ( testType == "Resume" )
    {
        RUN_TEST( ResumeTests, RunResumeTests );
    }
    else if ( testType == "Rescan" )
    {
        RUN_TEST( ResumeTests, Rescan );
    }
    else if ( testType == "Refresh" )
    {
        RUN_TEST( RefreshTests, RunRefreshTests );
    }
    else if ( testType == "BackupRestorePlaylist" )
    {
        RUN_TEST( Tests, RunBackupRestorePlaylist );
    }
    else
        assert( !"Invalid test type" );

}
