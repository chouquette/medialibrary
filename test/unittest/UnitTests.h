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
#include "utils/Directory.h"
#include "filesystem/libvlc/FileSystemFactory.h"
#include "logging/Logger.h"
#include "logging/IostreamLogger.h"

#include "common/util.h"
#include "mocks/FileSystem.h"

#include <iostream>

template <typename CB = mock::NoopCallback>
struct UnitTests
{
    std::unique_ptr<CB> cbMock;
    std::unique_ptr<MediaLibraryTester> ml;
    std::shared_ptr<mock::FileSystemFactory> fsMock;
    std::shared_ptr<mock::MockDeviceLister> mockDeviceLister;

    UnitTests()
    {
        Log::SetLogger( std::make_shared<IostreamLogger>() );
        Log::setLogLevel( LogLevel::Debug );
    }

    virtual ~UnitTests() = default;

    void InitTestFolder( const std::string& testSuite, const std::string& testName )
    {
        m_testDir = getTempPath( testSuite + "." + testName );
    }

    virtual void SetUp( const std::string& testSuite, const std::string& testName )
    {
        // Instantiate it here to avoid fiddling with multiple SetUp overloads
        if ( mockDeviceLister == nullptr )
            mockDeviceLister = std::make_shared<mock::MockDeviceLister>();

        SetupConfig cfg;
        cfg.deviceListers["file://"] = mockDeviceLister;

        InitTestFolder( testSuite, testName );
        InstantiateMediaLibrary( getDbPath(), m_testDir, &cfg );
        fsMock = std::make_shared<mock::FileSystemFactory>();
        SetupMockFileSystem();
        cbMock.reset( new CB );

        ml->setFsFactory( fsMock );
        Initialize();
        TestSpecificSetup();
    }

    virtual void SetupMockFileSystem()
    {
    }

    virtual void TestSpecificSetup()
    {
    }

    virtual void Initialize()
    {
        auto res = ml->initialize( cbMock.get() );
        ASSERT_EQ( InitializeResult::Success, res );
        auto setupRes = ml->setupDummyFolder();
        ASSERT_TRUE( setupRes );
    }

    virtual void InstantiateMediaLibrary( const std::string& dbPath,
                                          const std::string& mlDir,
                                          const SetupConfig* cfg )
    {
        ml.reset( new MediaLibraryTester( dbPath, mlDir, cfg ) );
    }

    virtual void TearDown()
    {
        ml.reset();
        ASSERT_TRUE( utils::fs::rmdir( m_testDir ) );
    }

    std::string getDbPath()
    {
        return m_testDir + "test.db";
    }
private:
    std::string m_testDir;
};

using Tests = UnitTests<>;

#define INIT_TESTS_COMMON(TestClass, TestSuite) \
    if ( ac != 2 ) { std::cerr << "Missing test name\n"; return 1; } \
    const char* selectedTest = av[1]; \
    auto t = std::make_unique<TestClass>(); \
    auto testSuite = #TestSuite;

#define INIT_TESTS(TestSuite) INIT_TESTS_COMMON(Tests, TestSuite)
#define INIT_TESTS_C(TestClass) INIT_TESTS_COMMON( TestClass, TestClass )

#define ADD_TEST( func ) \
    do { \
        if ( strcmp( #func, selectedTest ) == 0 ) { \
            try { \
                t->SetUp( testSuite, selectedTest ); \
                func( t.get() ); \
                t->TearDown(); \
                return 0; \
            } catch ( const TestFailed& tf ) { \
                std::cerr << "Test " << #func << " failed: " << tf.what() << "\n"; \
            } \
        } \
    } while ( 0 )

#define END_TESTS \
    return 1;
