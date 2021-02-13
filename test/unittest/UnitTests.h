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


struct Tests
{
    Tests();
    virtual ~Tests() = default;
    std::unique_ptr<MediaLibraryTester> ml;
    std::unique_ptr<mock::NoopCallback> cbMock;
    IMediaLibraryCb* mlCb;
    std::shared_ptr<fs::IFileSystemFactory> fsFactory;
    std::shared_ptr<mock::MockDeviceLister> mockDeviceLister;

    virtual void SetUp();
    virtual void InstantiateMediaLibrary();
    virtual void TearDown();
};

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
                fprintf(stderr, "Test %s failed: \n%s", #func, tf.what() ); \
            } \
        } \
    } while ( 0 )

#define END_TESTS \
    return 1;
