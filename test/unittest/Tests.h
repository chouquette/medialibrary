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

#include "gtest/gtest.h"

#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "common/NoopCallback.h"
#include "mocks/MockDeviceLister.h"
#include "MediaLibraryTester.h"
#include "medialibrary/filesystem/IDirectory.h"

class Tests : public testing::Test
{
protected:
    Tests();
    std::unique_ptr<MediaLibraryTester> ml;
    std::unique_ptr<mock::NoopCallback> cbMock;
    IMediaLibraryCb* mlCb;
    std::shared_ptr<fs::IFileSystemFactory> fsFactory;
    std::shared_ptr<mock::MockDeviceLister> mockDeviceLister;

    virtual void SetUp() override;
    virtual void InstantiateMediaLibrary();
    virtual void TearDown() override;
};
