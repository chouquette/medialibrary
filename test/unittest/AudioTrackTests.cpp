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
#include "AudioTrack.h"
#include "File.h"

class AudioTracks : public Tests
{
};

TEST_F( AudioTracks, AddTrack )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.mp3", IMedia::Type::Audio ) );
    bool res = f->addAudioTrack( "PCM", 128, 44100, 2, "fr", "test", 0 );
    ASSERT_TRUE( res );
}

TEST_F( AudioTracks, GetSetProperties )
{
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.mp3", IMedia::Type::Audio ) );
    ASSERT_NE( f, nullptr );
    f->addAudioTrack( "PCM", 128, 44100, 2, "en", "test desc", 0 );
    auto tracks = f->audioTracks()->all();
    ASSERT_EQ( tracks.size(), 1u );
    auto t = tracks[0];
    ASSERT_NE( t, nullptr );
    ASSERT_EQ( t->codec(), "PCM" );
    ASSERT_EQ( t->sampleRate(), 44100u );
    ASSERT_EQ( t->bitrate(), 128u );
    ASSERT_EQ( t->nbChannels(), 2u );
    ASSERT_EQ( t->language(), "en" );
    ASSERT_EQ( t->description(), "test desc" );

    auto f2 = ml->media( f->id() );
    tracks = f2->audioTracks()->all();
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
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.mp3", IMedia::Type::Audio ) );
    f->addAudioTrack( "PCM", 128, 44100, 2, "en", "test desc", 0 );
    f->addAudioTrack( "WMA", 128, 48000, 2, "fr", "test desc 2", 0 );

    auto ts = f->audioTracks()->all();
    ASSERT_EQ( ts.size(), 2u );
}

TEST_F( AudioTracks, RemoveTracks )
{
    auto f1 = std::static_pointer_cast<Media>( ml->addMedia( "track1.mp3", IMedia::Type::Audio ) );
    auto f2 = std::static_pointer_cast<Media>( ml->addMedia( "track2.mp3", IMedia::Type::Audio ) );
    f1->addAudioTrack( "PCM", 128, 44100, 2, "en", "test desc", 0 );
    f2->addAudioTrack( "WMA", 128, 48000, 2, "fr", "test desc", 0 );

    ASSERT_EQ( 1u, f1->audioTracks()->count() );
    ASSERT_EQ( 1u, f2->audioTracks()->count() );

    AudioTrack::removeFromMedia( ml.get(), f1->id() );

    ASSERT_EQ( 0u, f1->audioTracks()->count() );
    ASSERT_EQ( 1u, f2->audioTracks()->count() );
}

TEST_F( AudioTracks, CheckDbModel )
{
    auto res = AudioTrack::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( AudioTracks, UnlinkExternalTrack )
{
    auto m = std::static_pointer_cast<Media>(
                ml->addMedia( "mainmedia.mkv", IMedia::Type::Video ) );
    ASSERT_NE( nullptr, m );
    auto f = std::static_pointer_cast<File>(
                m->addExternalMrl( "externaltrack.mp3", IFile::Type::Soundtrack ) );
    ASSERT_NE( nullptr, f );
    auto res = m->addAudioTrack( "test", 123, 456, 2, "en", "test", f->id() );
    ASSERT_TRUE( res );

    auto tracks = m->audioTracks()->all();
    ASSERT_EQ( 1u, tracks.size() );

    res = f->destroy();
    ASSERT_TRUE( res );

    tracks = m->audioTracks()->all();
    ASSERT_EQ( 0u, tracks.size() );
}
