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

#include "Device.h"
#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"

class Devices : public Tests
{
protected:
    std::shared_ptr<mock::FileSystemFactory> fsMock;
    std::unique_ptr<mock::WaitForDiscoveryComplete> cbMock;

protected:
    virtual void SetUp() override
    {
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        Reload();
    }

    virtual void Reload()
    {
        Tests::Reload( fsMock, cbMock.get() );
    }
};

// Database/Entity tests

TEST_F( Devices, Create )
{
    auto d = ml->addDevice( "dummy", true );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    ASSERT_TRUE( d->isPresent() );

    Reload();

    d = ml->device( "dummy" );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    ASSERT_TRUE( d->isPresent() );
}

TEST_F( Devices, SetPresent )
{
    auto d = ml->addDevice( "dummy", true );
    ASSERT_NE( nullptr, d );
    ASSERT_TRUE( d->isPresent() );

    d->setPresent( false );
    ASSERT_FALSE( d->isPresent() );

    Reload();

    d = ml->device( "dummy" );
    ASSERT_FALSE( d->isPresent() );
}

// Filesystem tests:

TEST_F( Devices, RemoveDisk )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    auto file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_NE( nullptr, file );

    auto subdir = fsMock->directory( mock::FileSystemFactory::SubFolder );
    subdir->removeDevice();

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 2u, files.size() );

    file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_EQ( nullptr, file );
}

TEST_F( Devices, UnmountDisk )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    auto file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_NE( nullptr, file );

    auto subdir = fsMock->directory( mock::FileSystemFactory::SubFolder );
    auto device = std::static_pointer_cast<mock::Device>( subdir->device() );
    device->setPresent( false );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 2u, files.size() );

    file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    ASSERT_EQ( nullptr, file );
}
