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

#include "Tests.h"

#include <memory>

#include "filesystem/IFile.h"
#include "filesystem/IDirectory.h"
#include "utils/Filename.h"
#include "discoverer/FsDiscoverer.h"
#include "mocks/FileSystem.h"
#include "Media.h"
#include "Folder.h"

class TestEnv : public ::testing::Environment
{
    public:
        virtual void SetUp()
        {
            // Always clean the DB in case a previous test crashed
            unlink("test.db");
        }
};

void Tests::TearDown()
{
    ml.reset();
    unlink("test.db");
}

void Tests::Reload(std::shared_ptr<factory::IFileSystem> fs /*= nullptr*/, IMediaLibraryCb* metadataCb /*= nullptr*/ )
{
    InstantiateMediaLibrary();
    if ( fs == nullptr )
    {
        fs = std::shared_ptr<factory::IFileSystem>( new mock::NoopFsFactory );
    }
    ml->setFsFactory( fs );
    ml->setVerbosity( LogLevel::Error );
    bool res = ml->initialize( "test.db", "/tmp", metadataCb );
    ASSERT_TRUE( res );
}


void Tests::SetUp()
{
    Reload();
}

void Tests::InstantiateMediaLibrary()
{
    ml.reset( new MediaLibraryWithoutBackground );
}

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);

std::shared_ptr<Media> MediaLibraryTester::media( unsigned int id )
{
    return Media::fetch( m_dbConnection.get(), id );
}

MediaPtr MediaLibraryTester::media( const std::string& path )
{
    auto medias = files();
    for ( auto& f : medias )
    {
        if ( f->mrl() == path )
            return f;
    }
    return nullptr;
}

std::shared_ptr<Folder> MediaLibraryTester::folder( const std::string& path )
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name +
            " WHERE is_blacklisted IS NULL AND is_present = 1";
    auto folders = Folder::DatabaseHelpers::fetchAll<Folder>( m_dbConnection.get(), req );
    for ( auto &f : folders )
    {
        if ( f->path() == path )
            return f;
    }
    return nullptr;
}
