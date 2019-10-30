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

#include "Tests.h"

#include "Media.h"
#include "VideoTrack.h"

class VideoTracks : public Tests
{
};

TEST_F( VideoTracks, AddTrack )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.avi", IMedia::Type::Video ) );
    bool res = f->addVideoTrack( "H264", 1920, 1080, 3000, 1001, 1234,
                                 16, 9, "language", "description" );
    ASSERT_TRUE( res );
}

TEST_F( VideoTracks, FetchTracks )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.avi", IMedia::Type::Video ) );
    f->addVideoTrack( "H264", 1920, 1080, 3000, 100, 5678, 16, 10, "l1", "d1" );
    f->addVideoTrack( "VP80", 640, 480, 3000, 100, 9876, 16, 9, "l2", "d2" );

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
    ASSERT_EQ( t2->bitrate(), 5678u );
    ASSERT_EQ( t2->sarNum(), 16u );
    ASSERT_EQ( t2->sarDen(), 10u );
    ASSERT_EQ( t2->language(), "l1" );
    ASSERT_EQ( t2->description(), "d1" );

    // Reload from DB
    Reload();

    auto m = ml->media( f->id() );
    ASSERT_NE( nullptr, m );
    ts = m->videoTracks()->all();
    ASSERT_EQ( ts.size(), 2u );
    ASSERT_EQ( 2u, m->videoTracks()->count() );
    t2 = ts[0];
    ASSERT_EQ( t2->codec(), "H264" );
    ASSERT_EQ( t2->width(), 1920u );
    ASSERT_EQ( t2->height(), 1080u );
    ASSERT_EQ( t2->fps(), 30.0f );
    ASSERT_EQ( t2->fpsNum(), 3000u );
    ASSERT_EQ( t2->fpsDen(), 100u );
    ASSERT_EQ( t2->bitrate(), 5678u );
    ASSERT_EQ( t2->language(), "l1" );
    ASSERT_EQ( t2->description(), "d1" );
}

TEST_F( VideoTracks, RemoveTrack )
{
    auto f1 = std::static_pointer_cast<Media>( ml->addMedia( "file.avi", IMedia::Type::Video ) );
    auto f2 = std::static_pointer_cast<Media>( ml->addMedia( "file2.avi", IMedia::Type::Video ) );
    bool res = f1->addVideoTrack( "H264", 1920, 1080, 3000, 1001, 1234,
                                 16, 9, "language", "description" );
    ASSERT_TRUE( res );
    res = f2->addVideoTrack( "AV1", 1920, 1080, 3000, 1001, 1234,
                             16, 9, "language", "description" );
    ASSERT_TRUE( res );

    ASSERT_EQ( 1u, f1->videoTracks()->count() );
    ASSERT_EQ( 1u, f2->videoTracks()->count() );

    VideoTrack::removeFromMedia( ml.get(), f1->id() );
    ASSERT_EQ( 0u, f1->videoTracks()->count() );
    ASSERT_EQ( 1u, f2->videoTracks()->count() );
}

TEST_F( VideoTracks, CheckDbModel )
{
    auto res = VideoTrack::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}
