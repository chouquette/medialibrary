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

#include "Mountpoint.h"

class Mountpoints : public Tests
{
};

TEST_F( Mountpoints, Create )
{
    auto m = ml->addMountpoint( "dummy", true );
    ASSERT_NE( nullptr, m );
    ASSERT_EQ( "dummy", m->uuid() );
    ASSERT_TRUE( m->isRemovable() );
    ASSERT_TRUE( m->isPresent() );

    Reload();

    m = ml->mountpoint( "dummy" );
    ASSERT_NE( nullptr, m );
    ASSERT_EQ( "dummy", m->uuid() );
    ASSERT_TRUE( m->isRemovable() );
    ASSERT_TRUE( m->isPresent() );
}

TEST_F( Mountpoints, SetPresent )
{
    auto m = ml->addMountpoint( "dummy", true );
    ASSERT_NE( nullptr, m );
    ASSERT_TRUE( m->isPresent() );

    m->setPresent( false );
    ASSERT_FALSE( m->isPresent() );

    Reload();

    m = ml->mountpoint( "dummy" );
    ASSERT_FALSE( m->isPresent() );
}
