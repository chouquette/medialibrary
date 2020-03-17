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

#include "Tests.h"

#include "mocks/FileSystem.h"

Tests::Tests()
    : mlCb( nullptr )
{
}

void Tests::TearDown()
{
    ml.reset();
}

void Tests::Reload()
{
    InstantiateMediaLibrary();
    if ( fsFactory == nullptr )
    {
        fsFactory = std::shared_ptr<fs::IFileSystemFactory>( new mock::NoopFsFactory );
    }
    if ( mlCb == nullptr )
    {
        if ( cbMock == nullptr )
            cbMock.reset( new mock::NoopCallback );
        mlCb = cbMock.get();
    }
    // Instantiate it here to avoid fiddling with multiple SetUp overloads
    if ( mockDeviceLister == nullptr )
        mockDeviceLister = std::make_shared<mock::MockDeviceLister>();

    ml->setFsFactory( fsFactory );
    ml->registerDeviceLister( mockDeviceLister, "file://" );
    ml->setVerbosity( LogLevel::Error );
    auto res = ml->initialize( "test.db", "/tmp/ml_folder/", mlCb );
    ASSERT_EQ( InitializeResult::Success, res );
    auto setupRes = ml->setupDummyFolder();
    ASSERT_TRUE( setupRes );
    auto startRes = ml->start();
    ASSERT_EQ( StartResult::Success, startRes );
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
