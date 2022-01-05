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

#include "Device.h"

namespace
{

class MediaLibraryTesterDevices : public MediaLibraryTester
{
    using MediaLibraryTester::MediaLibraryTester;
    virtual bool setupDummyFolder() override { return true; }
};

}

class DeviceTests : public Tests
{
    virtual void InstantiateMediaLibrary( const std::string& dbPath,
                                          const std::string& mlFolderDir,
                                          const SetupConfig* cfg ) override
    {
        ml.reset( new MediaLibraryTesterDevices( dbPath, mlFolderDir, cfg ) );
    }
};


// Database/Entity tests

static void Create( DeviceTests* T )
{
    auto d = T->ml->addDevice( "dummy", "file://", true );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    ASSERT_TRUE( d->isPresent() );

    d = T->ml->device( "dummy", "file://" );
    ASSERT_NE( nullptr, d );
    ASSERT_EQ( "dummy", d->uuid() );
    ASSERT_TRUE( d->isRemovable() );
    // The device won't be marked absent until the discoverer starts, but it won't
    // happen in unit case configuration
}

static void SetPresent( DeviceTests* T )
{
    auto d = T->ml->addDevice( "dummy", "file://", true );
    ASSERT_NE( nullptr, d );
    ASSERT_TRUE( d->isPresent() );

    d->setPresent( false );
    ASSERT_FALSE( d->isPresent() );

    d = T->ml->device( "dummy", "file://" );
    ASSERT_FALSE( d->isPresent() );
}

static void CheckDbModel( DeviceTests* T )
{
    auto res = Device::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void MultipleScheme( DeviceTests* T )
{
    auto d1 = T->ml->addDevice( "dummy", "file://", false );
    auto d2 = T->ml->addDevice( "dummy", "smb://", true );
    ASSERT_NE( nullptr, d1 );
    ASSERT_NE( nullptr, d2 );
    ASSERT_NE( d1->id(), d2->id() );

    auto d = Device::fromUuid( T->ml.get(), d1->uuid(), d1->scheme() );
    ASSERT_EQ( d->id(), d1->id() );

    d = Device::fromUuid( T->ml.get(), d2->uuid(), d2->scheme() );
    ASSERT_EQ( d->id(), d2->id() );
}

static void IsKnown( DeviceTests* T )
{
    const std::string uuid{ "{device-uuid}" };
    const std::string localMountpoint{ "file:///path/to/device" };
    const std::string smbMountpoint{ "smb:///1.3.1.2/" };

    auto res = T->ml->isDeviceKnown( uuid, localMountpoint, false );
    ASSERT_FALSE( res );
    res = T->ml->isDeviceKnown( uuid, localMountpoint, false );
    ASSERT_TRUE( res );

    /* Ensure the removable flag doesn't change anything regarding probing */
    res = T->ml->isDeviceKnown( uuid, localMountpoint, true );
    ASSERT_TRUE( res );

    /*
     * Now check that device are uniquely identified by their scheme & uuid, not
     * just their uuid
     */
    res = T->ml->isDeviceKnown( uuid, smbMountpoint, false );
    ASSERT_FALSE( res );
    res = T->ml->isDeviceKnown( uuid, smbMountpoint, false );
    ASSERT_TRUE( res );
}

static void DeleteRemovable( DeviceTests* T )
{
    auto d = Device::create( T->ml.get(), "fake-device", "file://", false, false );
    ASSERT_NE( nullptr, d );
    d = Device::create( T->ml.get(), "fake-removable-device", "file://", true, false );
    ASSERT_NE( nullptr, d );
    d = Device::create( T->ml.get(), "another-removable-device", "file://", true, false );
    ASSERT_NE( nullptr, d );

    auto devices = Device::fetchAll( T->ml.get() );
    ASSERT_EQ( 3u, devices.size() );

    auto res = T->ml->deleteRemovableDevices();
    ASSERT_TRUE( res );

    devices = Device::fetchAll( T->ml.get() );
    ASSERT_EQ( 1u, devices.size() );
}

static void Mountpoints( DeviceTests* T )
{
    std::string mrl{ "smb://1.2.3.4/folder/file.mkv" };
    auto match = Device::fromMountpoint( T->ml.get(), mrl );
    ASSERT_EQ( 0u, std::get<0>( match ) );
    ASSERT_EQ( "", std::get<1>( match ) );

    auto d = Device::create( T->ml.get(), "{fake-uuid}", "smb://", true, true );
    ASSERT_NE( nullptr, d );
    auto res = d->addMountpoint( "smb://1.2.3.4", 0 );
    ASSERT_TRUE( res );

    match = Device::fromMountpoint( T->ml.get(), mrl );
    ASSERT_EQ( d->id(), std::get<0>( match ) );
    ASSERT_EQ( "smb://1.2.3.4/", std::get<1>( match ) );

    /*
     * Now insert another device with a more recent seen date and the
     * same mountpoint
     */
    auto d2 = Device::create( T->ml.get(), "{fake-uuid2}", "smb://", true, true );
    ASSERT_NE( nullptr, d );
    res = d2->addMountpoint( "smb://1.2.3.4", 100 );
    ASSERT_TRUE( res );

    match = Device::fromMountpoint( T->ml.get(), mrl );
    ASSERT_EQ( d2->id(), std::get<0>( match ) );
    ASSERT_EQ( "smb://1.2.3.4/", std::get<1>( match ) );

    /*
     * Now bump the seen date for the first device and check that it's returned
     * instead of the 2nd device
     */
    res = d->addMountpoint( "smb://1.2.3.4", 1000 );
    ASSERT_TRUE( res );

    match = Device::fromMountpoint( T->ml.get(), mrl );
    ASSERT_EQ( d->id(), std::get<0>( match ) );
    ASSERT_EQ( "smb://1.2.3.4/", std::get<1>( match ) );
}

static void FetchCachedMountpoint( DeviceTests* T )
{
    auto d = Device::create( T->ml.get(), "{fake-uuid}", "smb://", true, true );
    ASSERT_NE( nullptr, d );

    auto mountpoint = d->cachedMountpoint();
    ASSERT_TRUE( mountpoint.empty() );

    auto res = d->addMountpoint( "smb://1.2.3.4", 0 );
    ASSERT_TRUE( res );

    mountpoint = d->cachedMountpoint();
    ASSERT_EQ( "smb://1.2.3.4/", mountpoint );

    /* Ensure that we get the last known mountpoint */
    res = d->addMountpoint( "smb://4.3.2.1", 999 );
    ASSERT_TRUE( res );
    mountpoint = d->cachedMountpoint();
    ASSERT_EQ( "smb://4.3.2.1/", mountpoint );

    /* Now bump the previous mountpoint and check that it's now returned */
    res = d->addMountpoint( "smb://1.2.3.4", 1000 );
    ASSERT_TRUE( res );
    mountpoint = d->cachedMountpoint();
    ASSERT_EQ( "smb://1.2.3.4/", mountpoint );
}

static void UpdateLastSeen( DeviceTests* T )
{
    /*
     * Simply call the method and check that it successfully runs.
     * LastSeen isn't exposed outside of the database
     */
    auto d = Device::create( T->ml.get(), "{fake-uuid}", "smb://", true, true );
    d->updateLastSeen();
}

int main( int ac, char** av )
{
    INIT_TESTS_C( DeviceTests );

    ADD_TEST( Create );
    ADD_TEST( SetPresent );
    ADD_TEST( CheckDbModel );
    ADD_TEST( MultipleScheme );
    ADD_TEST( IsKnown );
    ADD_TEST( DeleteRemovable );
    ADD_TEST( Mountpoints );
    ADD_TEST( FetchCachedMountpoint );
    ADD_TEST( UpdateLastSeen );

    END_TESTS
}
