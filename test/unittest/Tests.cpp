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

#include "Tests.h"

#include "mocks/FileSystem.h"

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
}

void Tests::Reload( std::shared_ptr<fs::IFileSystemFactory> fs /*= nullptr*/, IMediaLibraryCb* metadataCb /*= nullptr*/ )
{
    InstantiateMediaLibrary();
    if ( fs == nullptr )
    {
        fs = std::shared_ptr<fs::IFileSystemFactory>( new mock::NoopFsFactory );
    }
    if ( metadataCb == nullptr )
    {
        if ( cbMock == nullptr )
            cbMock.reset( new mock::NoopCallback );
        metadataCb = cbMock.get();
    }
    // Instantiate it here to avoid fiddling with multiple SetUp overloads
    if ( mockDeviceLister == nullptr )
        mockDeviceLister = std::make_shared<mock::MockDeviceLister>();

    ml->setFsFactory( fs );
    ml->setDeviceLister( mockDeviceLister );
    ml->setVerbosity( LogLevel::Error );
    auto res = ml->initialize( "test.db", "/tmp", metadataCb );
    ASSERT_EQ( InitializeResult::Success, res );
    auto startRes = ml->start();
    ASSERT_TRUE( startRes );
    ml->reload();
}


void Tests::SetUp()
{
    unlink("test.db");
    Reload();
}

void Tests::InstantiateMediaLibrary()
{
    ml.reset( new MediaLibraryTester );
}

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);

