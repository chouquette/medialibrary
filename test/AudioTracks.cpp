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
#include "AudioTrack.h"

class AudioTracks : public Tests
{
};

TEST_F( AudioTracks, AddTrack )
{
    auto f = std::static_pointer_cast<Media>( ml->addFile( "file.mp3", nullptr ) );
    bool res = f->addAudioTrack( "PCM", 128, 44100, 2, "fr", "test" );
    ASSERT_TRUE( res );
}

TEST_F( AudioTracks, GetSetProperties )
{
    auto f = std::static_pointer_cast<Media>( ml->addFile( "file.mp3", nullptr ) );
    ASSERT_NE( f, nullptr );
    f->addAudioTrack( "PCM", 128, 44100, 2, "en", "test desc" );
    auto tracks = f->audioTracks();
    ASSERT_EQ( tracks.size(), 1u );
    auto t = tracks[0];
    ASSERT_NE( t, nullptr );
    ASSERT_EQ( t->codec(), "PCM" );
    ASSERT_EQ( t->sampleRate(), 44100u );
    ASSERT_EQ( t->bitrate(), 128u );
    ASSERT_EQ( t->nbChannels(), 2u );
    ASSERT_EQ( t->language(), "en" );
    ASSERT_EQ( t->description(), "test desc" );

    Reload();

    auto f2 = ml->file( "file.mp3" );
    tracks = f2->audioTracks();
    ASSERT_EQ( tracks.size(), 1u );
    t = tracks[0];
    ASSERT_EQ( t->codec(), "PCM" );
    ASSERT_EQ( t->sampleRate(), 44100u );
    ASSERT_EQ( t->bitrate(), 128u );
    ASSERT_EQ( t->nbChannels(), 2u );
    ASSERT_EQ( t->language(), "en" );
    ASSERT_EQ( t->description(), "test desc" );
}

TEST_F( AudioTracks, FetchTracks )
{
    auto f = std::static_pointer_cast<Media>( ml->addFile( "file.mp3", nullptr ) );
    f->addAudioTrack( "PCM", 128, 44100, 2, "en", "test desc" );
    f->addAudioTrack( "WMA", 128, 48000, 2, "fr", "test desc 2" );

    auto ts = f->audioTracks();
    ASSERT_EQ( ts.size(), 2u );
}

TEST_F( AudioTracks, CheckUnique )
{
    auto f = std::static_pointer_cast<Media>( ml->addFile( "file.mp3", nullptr ) );
    f->addAudioTrack( "PCM", 128, 44100, 2, "en", "test desc" );

    auto f2 = std::static_pointer_cast<Media>( ml->addFile( "file2.mp3", nullptr ) );
    f2->addAudioTrack( "PCM", 128, 44100, 2, "en", "test desc" );

    auto ts = f->audioTracks();

    auto ts2 = f2->audioTracks();

    ASSERT_EQ( ts.size(), 1u );
    ASSERT_EQ( ts2.size(), 1u );

    // Check that only 1 track has been created in DB
    ASSERT_EQ( ts[0]->id(), ts2[0]->id() );
}

