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

static void Parse( Tests* T )
{
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( T->lock ) );

    T->runChecks();
}

static void ParseTwice( Tests* T )
{
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( T->lock ) );

    T->runChecks();

    T->m_cb->prepareForRemoval( T->input.Size() );
    for ( auto i = 0u; i < T->input.Size(); ++i )
    {
        auto samplesDir = Tests::Directory + "samples/" + T->input[i].GetString();
        samplesDir = utils::fs::toAbsolute( samplesDir );
        T->m_ml->removeEntryPoint( utils::file::toMrl( samplesDir ) );
    }

    ASSERT_TRUE( T->m_cb->waitForRemovalComplete( T->lock ) );

    for ( auto i = 0u; i < T->input.Size(); ++i )
    {
        auto samplesDir = Tests::Directory + "samples/" + T->input[i].GetString();
        samplesDir = utils::fs::toAbsolute( samplesDir );
        T->m_ml->discover( utils::file::toMrl( samplesDir ) );
    }

    ASSERT_TRUE( T->m_cb->waitForParsingComplete( T->lock ) );

    T->runChecks();
}

static void RunResumeTests( ResumeTests* T )
{
    ASSERT_TRUE( T->m_cb->waitForDiscoveryComplete( T->lock ) );
    auto testMl = static_cast<MediaLibraryResumeTest*>( T->m_ml.get() );
    testMl->forceParserStart();
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( T->lock ) );

    T->runChecks();
}

static void Rescan( ResumeTests* T )
{
    ASSERT_TRUE( T->m_cb->waitForDiscoveryComplete( T->lock ) );
    auto testMl = static_cast<MediaLibraryResumeTest*>( T->m_ml.get() );
    testMl->forceParserStart();
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( T->lock ) );

    T->m_cb->reinit();
    T->m_ml->forceRescan();
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( T->lock ) );

    T->runChecks();
}

static void RunRefreshTests( RefreshTests* T )
{
    ASSERT_TRUE( T->m_cb->waitForDiscoveryComplete( T->lock ) );
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( T->lock ) );

    T->runChecks();

    T->m_cb->reinit();
    T->forceRefresh();

    ASSERT_TRUE( T->m_cb->waitForParsingComplete( T->lock ) );

    T->runChecks();
}

static void ReplaceVlcInstance( Tests* T )
{
    VLC::Instance inst{ 0, nullptr };
    T->m_ml->setExternalLibvlcInstance( inst.get() );
    /* Replacing the instance will stop the discoverer so let's resume it */
    T->m_ml->reload();
    ASSERT_TRUE( T->m_cb->waitForParsingComplete( T->lock ) );

    T->runChecks();
}

static void RunBackupRestorePlaylist( BackupRestorePlaylistTests* T )
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
    try \
    { \
        auto T = std::make_unique<Type>(); \
        T->SetUp( testType, testName ); \
        Func( T.get() ); \
        T->TearDown(); \
        return 0; \
    } catch ( const TestFailed& tf ) { \
        fprintf( stderr, "Test %s.%s failed: %s\n", testType.c_str(), \
                 testName.c_str(), tf.what() ); \
        return 2; \
    }

int main(int ac, char** av)
{
    std::string testType;
    std::string testName;
    if ( ac != 3 )
    {
        fprintf( stderr, "usage: %s <test type> <test name>\n", av[0] );
        return 1;
    }
    testType = av[1];
    testName = av[2];
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
    else if ( testType == "ReplaceVlcInstance" )
    {
        RUN_TEST( Tests, ReplaceVlcInstance );
    }
    else if ( testType == "BackupRestorePlaylist" )
    {
        RUN_TEST( BackupRestorePlaylistTests, RunBackupRestorePlaylist );
    }
    else
    {
        assert( !"Invalid test type" );
        return 1;
    }
    return 1;
}
