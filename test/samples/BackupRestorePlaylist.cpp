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
#include "Playlist.h"
#include "Media.h"
#include "utils/Filename.h"
#include "utils/Directory.h"

extern bool Verbose;
extern bool ExtraVerbose;
extern bool DebugVerbose;

class PlRestoreCb : public MockCallback
{
public:
    // We need to force the discover to appear as complete, as we won't do any
    // discovery for this test. Otherwise, we'd receive the parsing completed
    // event and just ignore it.
    void prepareForPlaylistReload()
    {
        m_discoveryCompleted = true;
    }

    bool waitForPlaylistReload( std::unique_lock<compat::Mutex>& lock )
    {
        m_done = false;
        // Wait for a while, generating snapshots can be heavy...
        return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds{ 20 }, [this]() {
            return m_done;
        });
    }
};

class MiscTests : public testing::Test
{
protected:
    std::unique_ptr<PlRestoreCb> m_cb;
    std::unique_ptr<MediaLibrary> m_ml;

    void SetUp()
    {
        unlink("test.db");
        m_cb.reset( new PlRestoreCb );
        m_ml.reset( new MediaLibrary );
        if ( ExtraVerbose == true )
            m_ml->setVerbosity( LogLevel::Debug );
        else if ( Verbose == true )
            m_ml->setVerbosity( LogLevel::Info );
        else if ( DebugVerbose == true )
            m_ml->setVerbosity( LogLevel::Verbose );

    #ifndef _WIN32
        auto mlDir = "/tmp/ml_folder/";
    #else
        // This assumes wine for now
        auto mlDir = "Z:\\tmp\\ml_folder\\";
    #endif
        auto devLister = std::make_shared<ForceRemovableStorareDeviceLister>();
        m_ml->registerDeviceLister( std::move( devLister ), "file://" );
        auto res = m_ml->initialize( "test.db", mlDir, m_cb.get() );
        ASSERT_EQ( InitializeResult::Success, res );
    }
};

TEST_F( MiscTests, ExportRestorePlaylist )
{
    auto lock = m_cb->lock();
    auto samplesFolder = std::string{ SRC_DIR "/test/samples/samples/playlist/tracks" };
    ASSERT_TRUE( utils::fs::isDirectory( samplesFolder ) );
    samplesFolder = utils::fs::toAbsolute( samplesFolder );
    m_ml->discover( utils::file::toMrl( samplesFolder ) );
    auto res = m_cb->waitForParsingComplete( lock );
    ASSERT_TRUE( res );
    // Now we should have discovered some media

    auto media = m_ml->audioFiles( nullptr )->all();
    ASSERT_EQ( 3u, media.size() );

    auto pl1 = std::static_pointer_cast<Playlist>( m_ml->createPlaylist( "Exported Playlist 1" ) );
    auto m1 = media[0];
    auto m2 = media[1];
    auto m3 = m_ml->addExternalMedia( "http://example.org/sea&otter.avi" );
    pl1->append( *m1 );
    pl1->append( *m2 );
    pl1->append( *m3 );

    auto pl2 = std::static_pointer_cast<Playlist>( m_ml->createPlaylist( "Exported Playlist 2" ) );
    pl2->append( *m3 );
    pl2->append( *m2 );
    pl2->append( *m1 );

    auto backup = Playlist::backupPlaylists( m_ml.get(), Settings::DbModelVersion );
    ASSERT_TRUE( std::get<0>( backup ) );

    m_cb->prepareForPlaylistReload();
    m_ml->clearDatabase( true );

    res = m_cb->waitForPlaylistReload( lock );
    ASSERT_TRUE( res );

    auto playlists = m_ml->playlists( nullptr )->all();
    ASSERT_EQ( 2u, playlists.size() );
    auto playlist1 = playlists[0];
    media = playlist1->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->title(), media[0]->title() );
    ASSERT_EQ( m2->title(), media[1]->title() );
    ASSERT_EQ( m3->title(), media[2]->title() );
    ASSERT_EQ( "Exported Playlist 1", playlist1->name() );

    auto playlist2 = playlists[1];
    media = playlist2->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m3->title(), media[0]->title() );
    ASSERT_EQ( m2->title(), media[1]->title() );
    ASSERT_EQ( m1->title(), media[2]->title() );
    ASSERT_EQ( "Exported Playlist 2", playlist2->name() );
}
