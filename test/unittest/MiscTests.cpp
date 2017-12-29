/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2017 Hugo Beauzée-Luyssen, Videolabs
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

#include <fstream>

#include "Tests.h"
#include "database/SqliteTools.h"
#include "database/SqliteConnection.h"

class Misc : public Tests
{
};

TEST_F( Misc, FileExtensions )
{
    const auto supportedExtensions = ml->getSupportedExtensions();
    for ( auto i = 0u; i < supportedExtensions.size() - 1; i++ )
    {
        ASSERT_LT( strcmp( supportedExtensions[i], supportedExtensions[i + 1] ), 0 );
    }
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
        ml.reset( new MediaLibraryWithoutBackground );
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
            ASSERT_EQ( dbVersion, Settings::DbModelVersion );
        }
        medialibrary::sqlite::Statement::FlushStatementCache();
    }
};

TEST_F( DbModel, Upgrade3to5 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v3.sql" );
    auto res = ml->initialize( "test.db", "/tmp", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    // All is done during the database initialization, we only care about no
    // exception being thrown, and MediaLibrary::initialize() returning true
}

TEST_F( DbModel, Upgrade4to5 )
{
    LoadFakeDB( SRC_DIR "/test/unittest/db_v4.sql" );
    auto res = ml->initialize( "test.db", "/tmp", cbMock.get() );
    ASSERT_EQ( InitializeResult::DbReset, res );

    // The culprit  with V4 was an invalid migration, leading to missing fields
    // in File and most likely Playlist tables. Simply try to create/fetch a file
    auto m = ml->addFile( "test.mkv" );
    ASSERT_NE( m, nullptr );
    auto files = ml->files();
    ASSERT_NE( files.size(), 0u );
}

TEST_F ( DbModel, Upgrade6to8 )
{
    // Model 7 is forcing a re-scan, there is no db model change
    LoadFakeDB( SRC_DIR "/test/unittest/db_v6.sql" ); // DB v6 with a file with failed status
    auto res = ml->initialize( "test.db", "/tmp", cbMock.get() );
    ASSERT_EQ( InitializeResult::Success, res );

    // Checking that the initial file is gone, ready to be parsed again
    auto media = ml->media( 1 );
    ASSERT_EQ( media, nullptr );
}
