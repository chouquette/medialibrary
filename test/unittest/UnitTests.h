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

#pragma once

#include "common/Tests.h"

#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "common/NoopCallback.h"
#include "mocks/MockDeviceLister.h"
#include "MediaLibraryTester.h"
#include "medialibrary/filesystem/IDirectory.h"

#include "common/util.h"
#include "mocks/FileSystem.h"

template <typename CB = mock::NoopCallback>
struct UnitTests
{
    std::unique_ptr<MediaLibraryTester> ml;
    std::unique_ptr<CB> cbMock;
    std::shared_ptr<fs::IFileSystemFactory> fsFactory;
    std::shared_ptr<mock::MockDeviceLister> mockDeviceLister;

    UnitTests() = default;
    virtual ~UnitTests() = default;

    virtual void SetUp()
    {
        auto mlDir = getTempPath( "ml_folder" );
        InstantiateMediaLibrary( "test.db", mlDir );
        if ( fsFactory == nullptr )
        {
            fsFactory = std::shared_ptr<fs::IFileSystemFactory>( new mock::NoopFsFactory );
        }
        cbMock.reset( new CB );

        // Instantiate it here to avoid fiddling with multiple SetUp overloads
        if ( mockDeviceLister == nullptr )
            mockDeviceLister = std::make_shared<mock::MockDeviceLister>();

        ml->setFsFactory( fsFactory );
        ml->registerDeviceLister( mockDeviceLister, "file://" );
        ml->setVerbosity( LogLevel::Debug );
        auto res = ml->initialize( cbMock.get() );
        ASSERT_EQ( InitializeResult::Success, res );
        auto setupRes = ml->setupDummyFolder();
        ASSERT_TRUE( setupRes );
        TestSpecificSetup();
    }

    virtual void TestSpecificSetup()
    {
    }

    virtual void InstantiateMediaLibrary( const std::string& dbPath,
                                          const std::string& mlDir )
    {
        ml.reset( new MediaLibraryTester( dbPath, mlDir ) );
    }

    virtual void TearDown()
    {
        ml.reset();
    }
};

using Tests = UnitTests<>;

#define INIT_TESTS_C(TestClass) \
    if ( ac != 2 ) { fprintf(stderr, "Missing test name\n" ); return 1; } \
    const char* selectedTest = av[1]; \
    auto t = std::make_unique<TestClass>();

#define INIT_TESTS INIT_TESTS_C(Tests)

#define ADD_TEST( func ) \
    do { \
        if ( strcmp( #func, selectedTest ) == 0 ) { \
            try { \
                t->SetUp(); \
                func( t.get() ); \
                t->TearDown(); \
                return 0; \
            } catch ( const TestFailed& tf ) { \
                fprintf(stderr, "Test %s failed: %s\n", #func, tf.what() ); \
            } \
        } \
    } while ( 0 )

#define END_TESTS \
    return 1;
