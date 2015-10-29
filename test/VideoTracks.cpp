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

#include "Media.h"
#include "VideoTrack.h"

class VideoTracks : public Tests
{
};

TEST_F( VideoTracks, AddTrack )
{
    auto f = std::static_pointer_cast<Media>( ml->addFile( "file.avi", nullptr ) );
    bool res = f->addVideoTrack( "H264", 1920, 1080, 29.97 );
    ASSERT_TRUE( res );
}

TEST_F( VideoTracks, FetchTracks )
{
    auto f = std::static_pointer_cast<Media>( ml->addFile( "file.avi", nullptr ) );
    f->addVideoTrack( "H264", 1920, 1080, 29.97 );
    f->addVideoTrack( "VP80", 640, 480, 29.97 );

    auto ts = f->videoTracks();
    ASSERT_EQ( ts.size(), 2u );
}

