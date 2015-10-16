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

#include "File.h"
#include "AudioTrack.h"

class AudioTracks : public Tests
{
};

TEST_F( AudioTracks, AddTrack )
{
    auto f = std::static_pointer_cast<File>( ml->addFile( "file.mp3", nullptr ) );
    bool res = f->addAudioTrack( "PCM", 44100, 128, 2 );
    ASSERT_TRUE( res );
}

TEST_F( AudioTracks, FetchTracks )
{
    auto f = std::static_pointer_cast<File>( ml->addFile( "file.mp3", nullptr ) );
    f->addAudioTrack( "PCM", 44100, 128, 2 );
    f->addAudioTrack( "WMA", 48000, 128, 2 );

    auto ts = f->audioTracks();
    ASSERT_EQ( ts.size(), 2u );
}

TEST_F( AudioTracks, CheckUnique )
{
    auto f = std::static_pointer_cast<File>( ml->addFile( "file.mp3", nullptr ) );
    f->addAudioTrack( "PCM", 44100, 128, 2 );

    auto f2 = std::static_pointer_cast<File>( ml->addFile( "file2.mp3", nullptr ) );
    f2->addAudioTrack( "PCM", 44100, 128, 2 );

    auto ts = f->audioTracks();

    auto ts2 = f2->audioTracks();

    ASSERT_EQ( ts.size(), 1u );
    ASSERT_EQ( ts2.size(), 1u );

    // Check that only 1 track has been created in DB
    ASSERT_EQ( ts[0]->id(), ts2[0]->id() );
}

