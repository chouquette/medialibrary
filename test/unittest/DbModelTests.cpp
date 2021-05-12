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

#include "Artist.h"
#include "Media.h"
#include "Metadata.h"
#include "Playlist.h"
#include "Device.h"
#include "parser/Task.h"
#include "Show.h"
#include "ShowEpisode.h"
#include "MediaGroup.h"
#include "File.h"

#include <fstream>

namespace
{
    const std::vector<const char*> expectedTriggers{
        "add_album_track",
        "album_is_present",
        "artist_decrement_nb_albums",
        "artist_decrement_nb_tracks",
        "artist_has_tracks_present",
        "artist_increment_nb_albums_unknown_album",
        "artist_increment_nb_tracks",
        "artist_update_nb_albums",
        "auto_delete_album_thumbnail",
        "auto_delete_artist_thumbnail", "auto_delete_media_thumbnail",
        "decr_thumbnail_refcount",
        "decrement_media_nb_playlist",
        "delete_album_fts", "delete_album_track", "delete_artist_fts",
        "delete_artist_without_tracks",
        "delete_folder_fts", "delete_genre_fts", "delete_label_fts",
        "delete_media_fts", "delete_playlist_fts", "delete_playlist_linking_tasks",
        "delete_show_fts", "delete_unused_thumbnail",
        "genre_update_is_present",
        "incr_thumbnail_refcount",
        "increment_media_nb_playlist", "insert_album_fts", "insert_artist_fts",
        "insert_folder_fts", "insert_genre_fts", "insert_media_fts",
        "insert_playlist_fts", "insert_show_fts",
        "media_cascade_file_deletion",
        "media_cascade_file_update",
        "media_group_decrement_nb_media_on_deletion",
        "media_group_delete_empty_group",
        "media_group_delete_fts",
        "media_group_insert_fts",
        "media_group_rename_forced_singleton",
        "media_group_update_duration_on_media_change",
        "media_group_update_duration_on_media_deletion",
        "media_group_update_nb_media_types",
        "media_group_update_nb_media_types_presence",
        "media_update_device_presence",
        "playlist_update_nb_media_on_media_deletion",
        "playlist_update_nb_present_media",
        "show_decrement_nb_episode",
        "show_increment_nb_episode",
        "show_update_is_present",
        "update_folder_nb_media_on_delete",
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
        "media_folder_id_idx",
        "media_group_creation_date",
        "media_group_duration",
        "media_group_forced_singleton",
        "media_group_id_idx",
        "media_group_last_modification_date",
        "media_last_usage_dates_idx",
        "media_progress_idx",
        "media_types_idx",
        "movie_media_idx",
        "parent_folder_id_idx",
        "playlist_file_id",
        "playlist_position_pl_id_index",
        "show_episode_media_show_idx",
        "subtitle_track_media_idx",
        "task_parent_folder_id_idx",
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
        "DeviceMountpoint",
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
        "MediaGroup",
        "MediaGroupFts",
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

class MediaLibraryTesterNoForceRescan : public MediaLibraryTester
{
public:
    using MediaLibraryTester::MediaLibraryTester;

    /*
     * Override forceRescanLocked to avoid removing all entities after the migration.
     * This allows more testing
     */
    virtual bool forceRescanLocked() override { return true; }

    virtual void onDbConnectionReady( sqlite::Connection* ) override
    {
    }
};

struct DbModel : public Tests
{
    virtual void InstantiateMediaLibrary( const std::string& dbPath,
                                          const std::string& mlDir ) override
    {
        ml.reset( new MediaLibraryTesterNoForceRescan( dbPath, mlDir ) );
    }

    virtual void Initialize() override
    {
        /*
         * Don't initialize the media lib now, wait until we load the fake
         * database for the migration tests
         */
    }

    void LoadFakeDB( const char* dbPath )
    {
        utils::fs::mkdir( utils::file::directory( getDbPath() ) );

        std::ifstream file{ dbPath };
        {
            auto dbConn = sqlite::Connection::connect( getDbPath() );
            ml->deleteAllTables( dbConn.get() );
            // The backup file already contains a transaction
            char buff[2048];
            {
                sqlite::Connection::WeakDbContext ctx{ dbConn.get() };
                while( file.getline( buff, sizeof( buff ) ) )
                {
                    medialibrary::sqlite::Statement stmt( dbConn->handle(), buff );
                    stmt.execute();
                    while ( stmt.row() != nullptr )
                        ;
                }
            }
            // Ensure we are doing a migration
            {
                medialibrary::sqlite::Statement stmt{ dbConn->handle(),
                        "SELECT * FROM Settings" };
                stmt.execute();
                auto row = stmt.row();
                uint32_t dbVersion;
                row >> dbVersion;
                ASSERT_NE( dbVersion, Settings::DbModelVersion );
            }
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
        ASSERT_TRUE( stmt.row() == nullptr );
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
        ASSERT_TRUE( stmt.row() == nullptr );
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
        ASSERT_TRUE( stmt.row() == nullptr );
    }

    virtual void TearDown() override
    {
        {
            auto dbConn = sqlite::Connection::connect( getDbPath() );
            medialibrary::sqlite::Statement stmt{ dbConn->handle(),
                    "SELECT * FROM Settings" };
            stmt.execute();
            auto row = stmt.row();
            uint32_t dbVersion;
            row >> dbVersion;
            ASSERT_EQ( Settings::DbModelVersion, dbVersion );
            /*
             * Let the local connection be closed before starting tearing down
             * all others and removing the database from disk
             */
        }
        Tests::TearDown();
    }

    void CommonMigrationTest( const char* mockDb )
    {
        LoadFakeDB( mockDb );
        auto res = ml->initialize( cbMock.get() );
        ASSERT_EQ( InitializeResult::Success, res );

        CheckTriggers( expectedTriggers );
        CheckIndexes( expectedIndexes );
        CheckTables( expectedTables );
    }
};

static void NbTriggers( DbModel* T )
{
    // Test the expected number of triggers on a freshly created database
    auto res = T->ml->initialize( T->cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    T->CheckTriggers( expectedTriggers );
    T->CheckIndexes( expectedIndexes );
    T->CheckTables( expectedTables );
}

static void Upgrade3to5( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v3.sql" );
}

static void Upgrade4to5( DbModel* T )
{
    T->LoadFakeDB( SRC_DIR "/test/unittest/db_v4.sql" );
    auto res = T->ml->initialize( T->cbMock.get() );
    ASSERT_EQ( InitializeResult::DbReset, res );

    // The culprit  with V4 was an invalid migration, leading to missing fields
    // in File and most likely Playlist tables. Simply try to create/fetch a file
    auto m = T->ml->addExternalMedia( "test.mkv", -1 );
    ASSERT_NE( m, nullptr );
    auto files = T->ml->files();
    ASSERT_NE( files.size(), 0u );

    T->CheckTables( expectedTables );
}

static void Upgrade7to8( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v7.sql" );
}

static void Upgrade8to9( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v8.sql" );

    // We expect the file-orphaned media to have been deleted
    auto media = T->ml->files();
    ASSERT_EQ( 1u, media.size() );
}

static void Upgrade12to13( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v12.sql" );
}

static void Upgrade13to14( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v13.sql" );
    auto media = T->ml->files();
    ASSERT_EQ( 4u, media.size() );
    auto m = media[0];
#if 0
    // Since 16 to 17 migration, we flush the thumbnails, so this will fail
    ASSERT_EQ( m->thumbnailMrl(), T->ml->thumbnailPath() + "/path/to/thumbnail" );
    ASSERT_TRUE( m->isThumbnailGenerated() );
#endif
    ASSERT_EQ( m->fileName(), "file with space.avi" );

    m = media[1];

    // Ensure we're probing the correct fake media
    ASSERT_EQ( m->id(), 2 );
    // Was IMedia::MetadataType::Progress
    auto& meta = m->metadata( static_cast<IMedia::MetadataType>( 50 ) );
    ASSERT_EQ( "fake progress", meta.asStr() );

    auto playlists = T->ml->playlists( nullptr )->all();
    ASSERT_EQ( 1u, playlists.size() );
    auto playlistMedia = playlists[0]->media( nullptr )->all();
    ASSERT_EQ( 3u, playlistMedia.size() );
    ASSERT_EQ( media[0]->id(), playlistMedia[0]->id() );
    ASSERT_EQ( 1u, std::static_pointer_cast<Media>( playlistMedia[0] )->nbPlaylists() );
    ASSERT_EQ( media[1]->id(), playlistMedia[1]->id() );
    ASSERT_EQ( 1u, std::static_pointer_cast<Media>( playlistMedia[1] )->nbPlaylists() );
    ASSERT_EQ( media[2]->id(), playlistMedia[2]->id() );
    ASSERT_EQ( 1u, std::static_pointer_cast<Media>( playlistMedia[2] )->nbPlaylists() );

    ASSERT_TRUE( media[2]->isExternalMedia() );

    auto externalMedia = T->ml->media( 99 );
    ASSERT_NE( nullptr, externalMedia );
    ASSERT_EQ( IMedia::Type::Unknown, externalMedia->type() );
    ASSERT_EQ( 0u, std::static_pointer_cast<Media>( externalMedia )->nbPlaylists() );

    auto folder = T->ml->folder( 1 );
    ASSERT_NE( nullptr, folder );
    ASSERT_EQ( 2u, folder->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( "folder", folder->name() );
}

static void Upgrade14to15( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v14.sql" );
}

static void Upgrade15to16( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v15.sql" );

    // Check that playlists were properly migrated
    medialibrary::sqlite::Statement stmt{
        T->ml->getConn()->handle(),
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

static void Upgrade16to17( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v16.sql" );
}

static void Upgrade17to18( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v17.sql" );
}

static void Upgrade18to19Broken( DbModel* T )
{
    // Test the repair migration after a broken 17/18 migration
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v18_broken.sql" );
}

static void Upgrade18to19Noop( DbModel* T )
{
    // Check that the repair migration doesn't do anything for a successful
    // 17->18 migration
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v18_ok.sql" );
}

static void Upgrade19to20( DbModel* T )
{
    // Check that the repair migration doesn't do anything for a successful
    // 17->18 migration
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v19.sql" );
}

static void Upgrade20to21( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v20.sql" );
}

static void Upgrade21to22( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v21.sql" );

    // The medialibrary may not find the device in the dummy database, so it
    // will be marked as missing, causing no folders to be returned.
    // However, if the device matches the one in the dummy database (ie. on my
    // machine...) the setPresent method will assert, causing the test to fail
    // in a different way.
    auto devices = Device::fetchAll( T->ml.get() );
    ASSERT_EQ( 1u, devices.size() );
    if ( devices[0]->isPresent() == false )
        devices[0]->setPresent( true );

    auto folders = T->ml->folders( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 3u, folders.size() );
    for ( const auto& f : folders )
    {
        auto audioQuery = f->media( IMedia::Type::Audio, nullptr );
        ASSERT_EQ( 1u, audioQuery->count() );
        ASSERT_EQ( 1u, audioQuery->all().size() );

        auto videoQuery = f->media( IMedia::Type::Video, nullptr );
        ASSERT_EQ( 0u, videoQuery->count() );
        ASSERT_EQ( 0u, videoQuery->all().size() );
    }
}

static void Upgrade22to23( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v22.sql" );

    // Check that we correctly migrated an internal media:
    auto m1 = T->ml->media( 1 );
    ASSERT_EQ( IMedia::Type::Audio, m1->type() );
    ASSERT_TRUE( m1->isDiscoveredMedia() );

    // Check that the stream media was correctly migrated as well
    auto m2 = T->ml->media( 4 );
    ASSERT_EQ( IMedia::Type::Unknown, m2->type() );
    ASSERT_FALSE( m2->isDiscoveredMedia() );
    ASSERT_TRUE( m2->isExternalMedia() );
    ASSERT_TRUE( m2->isStream() );

    // Ensure we now have one playlist task, which was tagged as a media task before
    uint32_t nbPlaylistTask;
    {
        sqlite::Statement stmt{
            T->ml->getConn()->handle(),
            "SELECT COUNT(*) FROM " + parser::Task::Table::Name +
                " WHERE file_type = " + std::to_string(
                        static_cast<std::underlying_type_t<IFile::Type>>( IFile::Type::Playlist ) )
        };
        stmt.execute();
        auto row = stmt.row();
        row >> nbPlaylistTask;
    }
    ASSERT_EQ( 1u, nbPlaylistTask );
}

static void Upgrade23to24( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v23.sql" );

    // Ensure user provided title was correctly deduced:
    auto m5 = T->ml->media( 5 );
    auto m6 = T->ml->media( 6 );
    ASSERT_FALSE( m5->isForcedTitle() );
    ASSERT_TRUE( m6->isForcedTitle() );
    ASSERT_EQ( "Custom title", m6->title() );

    auto devices = Device::fetchAll( T->ml.get() );
    ASSERT_EQ( 1u, devices.size() );

    auto shows = Show::fetchAll( T->ml.get() );
    ASSERT_EQ( 1u, shows.size() );
    auto episodes = shows[0]->episodes( nullptr )->all();
    ASSERT_EQ( 1u, episodes.size() );
    auto showEpisode = episodes[0]->showEpisode();
    ASSERT_NE( nullptr, showEpisode );
    ASSERT_EQ( showEpisode->title(), episodes[0]->title() );
}

static void Upgrade24to25( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v24.sql" );
    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( 2 * 10057u, groups[0]->duration() );
    ASSERT_EQ( "test group", groups[0]->name() );

    auto networkDevice = T->ml->device( "DOOP", "smb://" );
    ASSERT_NE( networkDevice, nullptr );
    ASSERT_TRUE( networkDevice->isNetwork() );
}

static void Upgrade25to26( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v25.sql" );

    auto show = T->ml->createShow( "new test show" );
    ASSERT_NE( nullptr, show );

    /* Ensure we don't have any restore task with unknown file_type field anymode */
    uint32_t nbUnknownFileTypeRestoreTask;
    {
        sqlite::Statement stmt{
            T->ml->getConn()->handle(),
            "SELECT COUNT(*) FROM " + parser::Task::Table::Name +
                " WHERE file_type = ? AND type = ? "
        };
        stmt.execute( IFile::Type::Unknown, parser::Task::Type::Restore );
        auto row = stmt.row();
        row >> nbUnknownFileTypeRestoreTask;
    }
    ASSERT_EQ( 0u, nbUnknownFileTypeRestoreTask );

    auto mg = T->ml->mediaGroup( 1 );
    ASSERT_EQ( "test-group", mg->name() );
    ASSERT_EQ( 1u, mg->nbPresentAudio() );
    ASSERT_EQ( 0u, mg->nbPresentVideo() );
    ASSERT_EQ( 0u, mg->nbPresentUnknown() );
    ASSERT_EQ( 1u, mg->nbPresentMedia() );
    ASSERT_EQ( 2u, mg->nbTotalMedia() );

    auto encodedFile = File::fetch( T->ml.get(), 6 );
    ASSERT_NE( nullptr, encodedFile );
    ASSERT_EQ( "udp://@224.10.50.36:5004", encodedFile->mrl() );
    ASSERT_TRUE( encodedFile->isNetwork() );

    const std::string req = "SELECT * FROM " + File::Table::Name +
            " WHERE is_network = 1";
    auto networkFiles = File::fetchAll<File>( T->ml.get(), req );
    ASSERT_EQ( 1u, networkFiles.size() );
    ASSERT_EQ( networkFiles[0]->id(), encodedFile->id() );

}

static void Upgrade26to27( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v26.sql" );
}

static void Upgrade27to28( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v27.sql" );
}

static void Upgrade29to30( DbModel* T )
{
    T->CommonMigrationTest( SRC_DIR "/test/unittest/db_v29.sql" );

    auto playlists = T->ml->playlists( nullptr )->all();
    ASSERT_EQ( 1u, playlists.size() );
    auto pl = playlists[0];
    QueryParameters params{};
    auto plMedia = pl->media( &params )->all();
    ASSERT_EQ( 2u, plMedia.size() );
    ASSERT_EQ( 1u, plMedia[0]->id() );
    ASSERT_EQ( 2u, plMedia[1]->id() );

    params.includeMissing = true;
    plMedia = pl->media( &params )->all();
    ASSERT_EQ( 3u, plMedia.size() );

    ASSERT_EQ( 3u, playlists[0]->nbMedia() );
    ASSERT_EQ( 2u, playlists[0]->nbPresentMedia() );

    auto mediaGroups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 2u, mediaGroups.size() );

    ASSERT_EQ( "A group", mediaGroups[0]->name() );
    ASSERT_EQ( 3u, mediaGroups[0]->nbTotalMedia() );
    ASSERT_EQ( 2u, mediaGroups[0]->nbPresentMedia() );

    ASSERT_EQ( "Z group", mediaGroups[1]->name() );
    ASSERT_EQ( 2u, mediaGroups[1]->nbTotalMedia() );
    ASSERT_EQ( 2u, mediaGroups[1]->nbPresentMedia() );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( DbModel )

    ADD_TEST( NbTriggers );
    ADD_TEST( Upgrade3to5 );
    ADD_TEST( Upgrade4to5 );
    ADD_TEST( Upgrade7to8 );
    ADD_TEST( Upgrade8to9 );
    ADD_TEST( Upgrade12to13 );
    ADD_TEST( Upgrade13to14 );
    ADD_TEST( Upgrade14to15 );
    ADD_TEST( Upgrade15to16 );
    ADD_TEST( Upgrade16to17 );
    ADD_TEST( Upgrade17to18 );
    ADD_TEST( Upgrade18to19Broken );
    ADD_TEST( Upgrade18to19Noop );
    ADD_TEST( Upgrade19to20 );
    ADD_TEST( Upgrade20to21 );
    ADD_TEST( Upgrade21to22 );
    ADD_TEST( Upgrade22to23 );
    ADD_TEST( Upgrade23to24 );
    ADD_TEST( Upgrade24to25 );
    ADD_TEST( Upgrade25to26 );
    ADD_TEST( Upgrade26to27 );
    ADD_TEST( Upgrade27to28 );
    ADD_TEST( Upgrade29to30 );

    END_TESTS
}
