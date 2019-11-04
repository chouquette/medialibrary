/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs
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

#include "SubtitleTrack.h"
#include "Media.h"

class SubtitleTracks : public Tests
{
};

TEST_F( SubtitleTracks, AddTrack )
{
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    auto res = media->addSubtitleTrack( "sea", "otter", "awareness", "week" );
    ASSERT_TRUE( res );
}

TEST_F( SubtitleTracks, FetchTracks )
{
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    media->addSubtitleTrack( "sea", "otter", "awareness", "week" );
    media->addSubtitleTrack( "best", "time", "of", "year" );

    auto tracks = media->subtitleTracks()->all();
    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( "sea",       tracks[0]->codec() );
    ASSERT_EQ( "otter",     tracks[0]->language() );
    ASSERT_EQ( "awareness", tracks[0]->description() );
    ASSERT_EQ( "week",      tracks[0]->encoding() );

    ASSERT_EQ( "best",      tracks[1]->codec() );
    ASSERT_EQ( "time",      tracks[1]->language() );
    ASSERT_EQ( "of",        tracks[1]->description() );
    ASSERT_EQ( "year",      tracks[1]->encoding() );

    Reload();

    media = ml->media( media->id() );
    tracks = media->subtitleTracks()->all();

    ASSERT_EQ( 2u, tracks.size() );
    ASSERT_EQ( "sea",       tracks[0]->codec() );
    ASSERT_EQ( "otter",     tracks[0]->language() );
    ASSERT_EQ( "awareness", tracks[0]->description() );
    ASSERT_EQ( "week",      tracks[0]->encoding() );

    ASSERT_EQ( "best",      tracks[1]->codec() );
    ASSERT_EQ( "time",      tracks[1]->language() );
    ASSERT_EQ( "of",        tracks[1]->description() );
    ASSERT_EQ( "year",      tracks[1]->encoding() );
}

TEST_F( SubtitleTracks, RemoveTrack )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    auto res = m1->addSubtitleTrack( "sea", "otter", "awareness", "week" );
    ASSERT_TRUE( res );
    auto m2 =  std::static_pointer_cast<Media>( ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    res = m2->addSubtitleTrack( "sea", "otter", "awareness", "week" );
    ASSERT_TRUE( res );

    ASSERT_EQ( 1u, m1->subtitleTracks()->count() );
    ASSERT_EQ( 1u, m2->subtitleTracks()->count() );

    SubtitleTrack::removeFromMedia( ml.get(), m1->id() );

    ASSERT_EQ( 0u, m1->subtitleTracks()->count() );
    ASSERT_EQ( 1u, m2->subtitleTracks()->count() );
}

TEST_F( SubtitleTracks, CheckDbModel )
{
    auto res = SubtitleTrack::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}
