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

#include "gtest/gtest.h"

#include "utils/Url.h"

using namespace medialibrary;

TEST( UrlUtils, encode )
{
    ASSERT_EQ( "meow", utils::url::encode( "meow" ) );
    ASSERT_EQ( "plain%20space", utils::url::encode(  "plain space" ) );
    ASSERT_EQ( "/%C3%A1%C3%A9%C3%BA%C3%AD%C3%B3/f00/%C3%9Far", utils::url::encode( "/áéúíó/f00/ßar" ) );
    ASSERT_EQ( "/file/with%23sharp", utils::url::encode( "/file/with#sharp" ) );
}
