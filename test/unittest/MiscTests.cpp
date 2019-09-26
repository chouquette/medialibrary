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

#include <fstream>

#include "Tests.h"
#include "database/SqliteTools.h"
#include "database/SqliteConnection.h"
#include "utils/Strings.h"

#include "Artist.h"
#include "Media.h"
#include "Metadata.h"
#include "Playlist.h"
#include "Device.h"

namespace
{
    const std::vector<const char*> expectedTriggers{
        "add_album_track", "auto_delete_album_thumbnail",
        "auto_delete_artist_thumbnail", "auto_delete_media_thumbnail",
        "cascade_file_deletion", "decr_thumbnail_refcount", "decrement_media_nb_playlist",
        "delete_album_fts", "delete_album_track", "delete_artist_fts",
        "delete_folder_fts", "delete_genre_fts", "delete_label_fts",
        "delete_media_fts", "delete_playlist_fts", "delete_playlist_linking_tasks",
        "delete_show_fts", "delete_unused_thumbnail",
        "has_album_remaining", "has_track_remaining", "has_tracks_present",
        "incr_thumbnail_refcount",
        "increment_media_nb_playlist", "insert_album_fts", "insert_artist_fts",
        "insert_folder_fts", "insert_genre_fts", "insert_media_fts",
        "insert_playlist_fts", "insert_show_fts", "is_album_present",
        "is_media_device_present", "update_folder_nb_media_on_delete",
        "update_folder_nb_media_on_insert", "update_folder_nb_media_on_update",
        "update_genre_on_new_track", "update_genre_on_track_deleted",
        "update_media_title_fts", "update_playlist_fts",
        "update_playlist_order_on_delete", "update_playlist_order_on_insert",
        "update_thumbnail_refcount",
    };

    const std::vector<const char*> expectedIndexes{
        "album_artist_id_idx",
        "album_media_artist_genre_album_idx",
        "album_track_album_genre_artist_ids",
        "audio_track_media_idx",
        "file_folder_id_index",
        "file_media_id_index",
        "folder_device_id_idx",
        "index_last_played_date",
        "index_media_presence",
        "media_last_usage_dates_idx",
        "media_types_idx",
        "movie_media_idx",
        "parent_folder_id_idx",
        "playlist_file_id",
        "playlist_position_pl_id_index",
        "show_episode_media_show_idx",
        "subtitle_track_media_idx",
        "thumbnail_link_index",
        "video_track_media_idx",
    };

    const std::vector<const char*> expectedTables{
        "Album",
        "AlbumFts",
        "AlbumTrack",
        "Artist",
        "ArtistFts",
        "AudioTrack",
        "Bookmark",
        "Chapter",
        "Device",
        "ExcludedEntryFolder",
        "File",
        "Folder",
        "FolderFts",
        "Genre",
        "GenreFts",
        "Label",
        "LabelFileRelation",
        "Media",
        "MediaArtistRelation",
        "MediaFts",
        "Metadata",
        "Movie",
        "Playlist",
        "PlaylistFts",
        "PlaylistMediaRelation",
        "Settings",
        "Show",
        "ShowEpisode",
        "ShowFts",
        "SubtitleTrack",
        "Task",
        "Thumbnail",
        "ThumbnailLinking",
        "VideoTrack",
    };

    const std::vector<const char*> expectedViews{
        "VideoGroup",
    };

    bool checkAlphaOrderedVector( const std::vector<const char*> in )
    {
        for ( auto i = 0u; i < in.size() - 1; i++ )
        {
            if ( strcmp( in[i], in[i + 1] ) >= 0 )
                return false;
        }
        return true;
    }
}

class Misc : public Tests
{
    virtual void SetUp() override
    {
        // No need to setup anything, those tests are not using the DB
    }
};

TEST_F( Misc, FileExtensions )
{
    const auto supportedExtensions = ml->getSupportedExtensions();
    auto res = checkAlphaOrderedVector( supportedExtensions );
    ASSERT_TRUE( res );
}

TEST_F( Misc, TrimString )
{
    ASSERT_EQ( utils::str::trim( "hello world" ), "hello world" );
    ASSERT_EQ( utils::str::trim( "  spaaaaaace   " ), "spaaaaaace" );
    ASSERT_EQ( utils::str::trim( "\tfluffy\notters  \t\n" ), "fluffy\notters" );
    ASSERT_EQ( utils::str::trim( "    " ), "" );
    ASSERT_EQ( utils::str::trim( "" ), "" );
}

TEST_F( Misc, SanitizePattern )
{
    // "" will become " "" "" *", (without spaces) as all double quotes are
    // escaped, and the pattern itself is enclosed between " *"
    ASSERT_EQ( "\"\"\"\"\"*\"", sqlite::Tools::sanitizePattern( "\"\"" ) );
    ASSERT_EQ( "\"Little Bobby Table*\"", sqlite::Tools::sanitizePattern( "Little Bobby Table" ) );
    ASSERT_EQ( "\"Test \"\" Pattern*\"", sqlite::Tools::sanitizePattern( "Test \" Pattern" ) );
    ASSERT_EQ( "\"It''s a test*\"", sqlite::Tools::sanitizePattern( "It's a test" ) );
    ASSERT_EQ( "\"''\"\"*\"", sqlite::Tools::sanitizePattern( "\'\"" ) );
}

class MiscDb : public Tests
{
};

TEST_F( MiscDb, TaskCheckDbModel )
{
    auto res = parser::Task::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

class DbModel : public testing::Test
{
protected:
    std::unique_ptr<MediaLibraryTester> ml;
    std::unique_ptr<mock::NoopCallback> cbMock;

public:
    virtual void SetUp() override
    {
        unlink("test.db");
        ml.reset( new MediaLibraryTester );
        cbMock.reset( new mock::NoopCallback );
    }

    void LoadFakeDB( const char* dbPath )
    {
        std::ifstream file{ dbPath };
        {
            // Don't use our Sqlite wrapper to open a connection. We don't want
            // to mess with per-thread connections.
            medialibrary::sqlite::Connection::Handle conn;
            sqlite3_open( "test.db", &conn );
            std::unique_ptr<sqlite3, int(*)(sqlite3*)> dbPtr{ conn, &sqlite3_close };
            // The backup file already contains a transaction
            char buff[2048];
            while( file.getline( buff, sizeof( buff ) ) )
            {
                medialibrary::sqlite::Statement stmt( conn, buff );
                stmt.execute();
                while ( stmt.row() != nullptr )
                    ;
            }
            // Ensure we are doing a migration
            {
                medialibrary::sqlite::Statement stmt{ conn,
                        "SELECT * FROM Settings" };
                stmt.execute();
                auto row = stmt.row();
                uint32_t dbVersion;
                row >> dbVersion;
                ASSERT_NE( dbVersion, Settings::DbModelVersion );
            }
            // Keep address sanitizer/memleak detection happy
            medialibrary::sqlite::Statement::FlushStatementCache();
        }
    }

    void CheckTriggers( std::vector<const char*> expected )
    {
        medialibrary::sqlite::Statement stmt{ ml->getConn()->handle(),
                "SELECT name FROM sqlite_master WHERE type='trigger' "
                    "ORDER BY name;"
        };
        stmt.execute();
        auto res = checkAlphaOrderedVector( expected );
        ASSERT_TRUE( res );
        for ( const auto& expectedName : expected )
        {
            auto row = stmt.row();
            ASSERT_TRUE( row != nullptr );
            ASSERT_EQ( 1u, row.nbColumns() );
            auto name = row.extract<std::string>();
            ASSERT_EQ( expectedName, name );
        }
        ASSERT_EQ( stmt.row(), nullptr );
    }

    void CheckIndexes( std::vector<const char*> expected )
    {
        auto res = checkAlphaOrderedVector( expected );
        ASSERT_TRUE( res );

        medialibrary::sqlite::Statement stmt{ ml->getConn()->handle(),
                "SELECT name FROM sqlite_master WHERE type='index' AND "
                "name NOT LIKE 'sqlite_autoindex%' ORDER BY name" };
        stmt.execute();
        for ( const auto& expectedName : expected )
        {
            auto row = stmt.row();
            ASSERT_TRUE( row != nullptr );
            ASSERT_EQ( 1u, row.nbColumns() );
            auto name = row.extract<std::string>();
            ASSERT_EQ( expectedName, name );
        }
        ASSERT_EQ( stmt.row(), nullptr );
    }

    void CheckTables( std::vector<const char*> expected )
    {
        auto res = checkAlphaOrderedVector( expected );
        ASSERT_TRUE( res );

        medialibrary::sqlite::Statement stmt{ ml->getConn()->handle(),
                "SELECT name FROM sqlite_master WHERE type='table' "
                "AND name NOT LIKE '%#_%' ESCAPE '#' ORDER BY name"
        };
        stmt.execute();
        for ( const auto& expectedName : expected )
        {
            auto row = stmt.row();
            ASSERT_EQ( 1u, row.nbColumns() );
            auto name = row.extract<std::string>();
            ASSERT_EQ( expectedName, name );
        }
        ASSERT_EQ( stmt.row(), nullptr );
    }

    void CheckViews( std::vector<const char*> expected )
    {
        auto res = checkAlphaOrderedVector( expected );
        ASSERT_TRUE( res );

        medialibrary::sqlite::Statement stmt{ ml->getConn()->handle(),
                "SELECT name FROM sqlite_master WHERE type='view'"
        };
        stmt.execute();
        for ( const auto& expectedName : expected )
        {
            auto row = stmt.row();
            ASSERT_EQ( 1u, row.nbColumns() );
            auto name = row.extract<std::string>();
            ASSERT_EQ( expectedName, name );
        }
        ASSERT_EQ( stmt.row(), nullptr );
    }

    virtual void TearDown() override
    {
        medialibrary::sqlite::Connection::Handle conn;
        sqlite3_open( "test.db", &conn );
        std::unique_ptr<sqlite3, int(*)(sqlite3*)> dbPtr{ conn, &sqlite3_close };
        {
            medialibrary::sqlite::Statement stmt{ conn,
                    "SELECT * FROM Settings" };
            stmt.execute();
            auto row = stmt.row();
            uint32_t dbVersion;
            row >> dbVersion;
            ASSERT_EQ( Settings::DbModelVersion, dbVersion );
        }
        medialibrary::sqlite::Statement::FlushStatementCache();
    }
};

TEST_F( DbModel, NbTriggers )
{
    // Test the expected number of triggers on a freshly created database
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckTriggers( expectedTriggers );
    CheckIndexes( expectedIndexes );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade3to5 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v3.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    // All is done during the database initialization, we only care about no
    // exception being thrown, and MediaLibrary::initialize() returning true
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade4to5 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v4.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::DbReset, res );

    // The culprit  with V4 was an invalid migration, leading to missing fields
    // in File and most likely Playlist tables. Simply try to create/fetch a file
    auto m = ml->addMedia( "test.mkv" );
    ASSERT_NE( m, nullptr );
    auto files = ml->files();
    ASSERT_NE( files.size(), 0u );

    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade7to8 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v7.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    // Removed post migration tests starting with V9, since we force a re-scan,
    // there is no content left to test

    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade8to9 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v8.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    // We expect the file-orphaned media to have been deleted
    auto media = ml->files();
    ASSERT_EQ( 1u, media.size() );

    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade12to13 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v12.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    // We can't check for the number of albums anymore since they are deleted
    // as part of 13 -> 14 migration

    CheckTriggers( expectedTriggers );
    CheckIndexes( expectedIndexes );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade13to14 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v13.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    auto media = ml->files();
    ASSERT_EQ( 4u, media.size() );
    auto m = media[0];
#if 0
    // Since 16 to 17 migration, we flush the thumbnails, so this will fail
    ASSERT_EQ( m->thumbnailMrl(), ml->thumbnailPath() + "/path/to/thumbnail" );
    ASSERT_TRUE( m->isThumbnailGenerated() );
#endif
    ASSERT_EQ( m->fileName(), "file with space.avi" );

    m = media[1];

    // Ensure we're probing the correct fake media
    ASSERT_EQ( m->id(), 2 );
    auto& meta = m->metadata( IMedia::MetadataType::Progress );
    ASSERT_EQ( "fake progress", meta.asStr() );

    auto playlists = ml->playlists( nullptr )->all();
    ASSERT_EQ( 1u, playlists.size() );
    auto playlistMedia = playlists[0]->media()->all();
    ASSERT_EQ( 3u, playlistMedia.size() );
    ASSERT_EQ( media[0]->id(), playlistMedia[0]->id() );
    ASSERT_EQ( 1u, std::static_pointer_cast<Media>( playlistMedia[0] )->nbPlaylists() );
    ASSERT_EQ( media[1]->id(), playlistMedia[1]->id() );
    ASSERT_EQ( 1u, std::static_pointer_cast<Media>( playlistMedia[1] )->nbPlaylists() );
    ASSERT_EQ( media[2]->id(), playlistMedia[2]->id() );
    ASSERT_EQ( 1u, std::static_pointer_cast<Media>( playlistMedia[2] )->nbPlaylists() );

    ASSERT_EQ( IMedia::Type::External, media[2]->type() );

    auto externalMedia = ml->media( 99 );
    ASSERT_NE( nullptr, externalMedia );
    ASSERT_EQ( IMedia::Type::Unknown, externalMedia->type() );
    ASSERT_EQ( 0u, std::static_pointer_cast<Media>( externalMedia )->nbPlaylists() );

    auto folder = ml->folder( 1 );
    ASSERT_NE( nullptr, folder );
    ASSERT_EQ( 2u, folder->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( "folder", folder->name() );

    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade14to15 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v14.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckIndexes( expectedIndexes );
    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade15to16 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v15.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckIndexes( expectedIndexes );
    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );

    // Check that playlists were properly migrated
    medialibrary::sqlite::Statement stmt{
        ml->getConn()->handle(),
        "SELECT playlist_id, position FROM PlaylistMediaRelation "
            "ORDER BY playlist_id, position"
    };
    stmt.execute();
    auto expected = 0u;
    auto playlistId = 0uLL;
    sqlite::Row row;
    while ( ( row = stmt.row() ) != nullptr )
    {
        uint32_t pos;
        uint64_t pId;
        row >> pId >> pos;
        if ( pId != playlistId )
        {
            expected = 0;
            playlistId = pId;
        }
        ASSERT_EQ( pos, expected );
        ++expected;
    }
}

TEST_F( DbModel, Upgrade16to17 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v16.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckIndexes( expectedIndexes );
    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade17to18 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v17.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckIndexes( expectedIndexes );
    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade18to19Broken )
{
    // Test the repair migration after a broken 17/18 migration
    LoadFakeDB( SRC_DIR "/test/unittest/db_v18_broken.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckIndexes( expectedIndexes );
    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}


TEST_F( DbModel, Upgrade18to19Noop )
{
    // Check that the repair migration doesn't do anything for a successful
    // 17->18 migration
    LoadFakeDB( SRC_DIR "/test/unittest/db_v18_ok.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckIndexes( expectedIndexes );
    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}


TEST_F( DbModel, Upgrade19to20 )
{
    // Check that the repair migration doesn't do anything for a successful
    // 17->18 migration
    LoadFakeDB( SRC_DIR "/test/unittest/db_v19.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckIndexes( expectedIndexes );
    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrade20to21 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v20.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckIndexes( expectedIndexes );
    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );
}

TEST_F( DbModel, Upgrate21to22 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v21.sql" );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    CheckIndexes( expectedIndexes );
    CheckTriggers( expectedTriggers );
    CheckTables( expectedTables );
    CheckViews( expectedViews );

    // The medialibrary may not find the device in the dummy database, so it
    // will be marked as missing, causing no folders to be returned.
    // However, if the device matches the one in the dummy database (ie. on my
    // machine...) the setPresent method will assert, causing the test to fail
    // in a different way.
    auto devices = Device::fetchAll( ml.get() );
    ASSERT_EQ( 1u, devices.size() );
    if ( devices[0]->isPresent() == false )
        devices[0]->setPresent( true );

    auto folders = ml->folders( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 3u, folders.size() );
    for ( const auto f : folders )
    {
        auto audioQuery = f->media( IMedia::Type::Audio, nullptr );
        ASSERT_EQ( 1u, audioQuery->count() );
        ASSERT_EQ( 1u, audioQuery->all().size() );

        auto videoQuery = f->media( IMedia::Type::Video, nullptr );
        ASSERT_EQ( 0u, videoQuery->count() );
        ASSERT_EQ( 0u, videoQuery->all().size() );
    }
}
