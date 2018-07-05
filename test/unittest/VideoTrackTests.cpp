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

#include "Tests.h"

#include "Media.h"
#include "VideoTrack.h"

class VideoTracks : public Tests
{
};

TEST_F( VideoTracks, AddTrack )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.avi" ) );
    bool res = f->addVideoTrack( "H264", 1920, 1080, 3000, 1001, "language", "description" );
    ASSERT_TRUE( res );
}

TEST_F( VideoTracks, FetchTracks )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.avi" ) );
    f->addVideoTrack( "H264", 1920, 1080, 3000, 100, "l1", "d1" );
    f->addVideoTrack( "VP80", 640, 480, 3000, 100, "l2", "d2" );

    // Testing fetch from initially created instance:
    auto ts = f->videoTracks()->all();
    ASSERT_EQ( ts.size(), 2u );
    auto t2 = ts[0];
    ASSERT_EQ( t2->codec(), "H264" );
    ASSERT_EQ( t2->width(), 1920u );
    ASSERT_EQ( t2->height(), 1080u );
    ASSERT_EQ( t2->fps(), 30.0f );
    ASSERT_EQ( t2->fpsNum(), 3000u );
    ASSERT_EQ( t2->fpsDen(), 100u );
    ASSERT_EQ( t2->language(), "l1" );
    ASSERT_EQ( t2->description(), "d1" );

    // Reload from DB
    Reload();

    auto m = ml->media( f->id() );
    ASSERT_NE( nullptr, m );
    ts = m->videoTracks()->all();
    ASSERT_EQ( ts.size(), 2u );
    t2 = ts[0];
    ASSERT_EQ( t2->codec(), "H264" );
    ASSERT_EQ( t2->width(), 1920u );
    ASSERT_EQ( t2->height(), 1080u );
    ASSERT_EQ( t2->fps(), 30.0f );
    ASSERT_EQ( t2->fpsNum(), 3000u );
    ASSERT_EQ( t2->fpsDen(), 100u );
    ASSERT_EQ( t2->language(), "l1" );
    ASSERT_EQ( t2->description(), "d1" );
}


