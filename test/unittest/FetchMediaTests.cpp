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

#include "UnitTests.h"

#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"

struct FetchMediaTests : public Tests
{
    static const std::string RemovableDeviceUuid;
    static const std::string RemovableDeviceMountpoint;
    std::shared_ptr<mock::FileSystemFactory> fsMock;
    std::unique_ptr<mock::WaitForDiscoveryComplete> cbMock;

    virtual void SetUp() override
    {
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        fsMock->addFolder( "file:///a/mnt/" );
        auto device = fsMock->addDevice( RemovableDeviceMountpoint, RemovableDeviceUuid, true );
        fsMock->addFile( RemovableDeviceMountpoint + "removablefile.mp3" );
        fsFactory = fsMock;
        mlCb = cbMock.get();
        Tests::SetUp();
    }

    virtual void InstantiateMediaLibrary( const std::string& dbPath,
                                          const std::string& mlFolderDir ) override
    {
        ml.reset( new MediaLibraryWithDiscoverer( dbPath, mlFolderDir ) );
    }
};

const std::string FetchMediaTests::RemovableDeviceUuid = "{fake-removable-device}";
const std::string FetchMediaTests::RemovableDeviceMountpoint = "file:///a/mnt/fake-device/";

static void FetchNonRemovable( FetchMediaTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto m = T->ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    ASSERT_NE( nullptr, m );
}

static void FetchRemovable( FetchMediaTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto m = T->ml->media( FetchMediaTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_NE( nullptr, m );
}

static void FetchRemovableUnplugged( FetchMediaTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    T->fsMock->unmountDevice( FetchMediaTests::RemovableDeviceUuid );

    T->ml->reload();
    bool reloaded = T->cbMock->waitReload();
    ASSERT_TRUE( reloaded );

    auto m = T->ml->media( FetchMediaTests::RemovableDeviceMountpoint + "removablefile.mp3" );
    ASSERT_EQ( nullptr, m );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( FetchMediaTests );

    ADD_TEST( FetchNonRemovable );
    ADD_TEST( FetchRemovable );
    ADD_TEST( FetchRemovableUnplugged );

    END_TESTS
}
