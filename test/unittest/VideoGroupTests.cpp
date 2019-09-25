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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Tests.h"

#include "VideoGroup.h"
#include "Media.h"

class VideoGroups : public Tests
{
    virtual void SetUp() override
    {
        Tests::SetUp();
        ml->addMedia( "otters are fluffy.mp3", IMedia::Type::Audio );
        ml->addMedia( "the groupname.mp3", IMedia::Type::Audio );
        ml->addMedia( "lonelyotter.mp3", IMedia::Type::Audio );
    }
};

TEST_F( VideoGroups, List )
{
    ml->addMedia( "video.mkv", IMedia::Type::Video );
    ml->addMedia( "video.avi", IMedia::Type::Video );
    ml->addMedia( "lonelyotter.mkv", IMedia::Type::Video );

    auto groups = ml->videoGroups( nullptr )->all();
    // Default sorting order is alpha, so expect the «lonely» group first
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( "lonelyotter.mkv", groups[0]->name() );
    ASSERT_EQ( "video.", groups[1]->name() );

    ASSERT_EQ( 1u, groups[0]->count() );
    ASSERT_EQ( 2u, groups[1]->count() );
}

TEST_F( VideoGroups, Paging )
{
    ml->addMedia( "video.mkv", IMedia::Type::Video );
    ml->addMedia( "video.avi", IMedia::Type::Video );
    ml->addMedia( "lonelyotter.mkv", IMedia::Type::Video );

    auto groupsQuery = ml->videoGroups( nullptr );
    ASSERT_NE( nullptr, groupsQuery );
    ASSERT_EQ( 2u, groupsQuery->count() );

    auto gs = groupsQuery->items( 1, 0 );
    ASSERT_EQ( 1u, gs.size() );
    ASSERT_EQ( "lonelyotter.mkv", gs[0]->name() );

    gs = groupsQuery->items( 1, 1 );
    ASSERT_EQ( 1u, gs.size() );
    ASSERT_EQ( "video.", gs[0]->name() );

    gs = groupsQuery->items( 100, 2 );
    ASSERT_EQ( 0u, gs.size() );
}

TEST_F( VideoGroups, Sort )
{
    ml->addMedia( "video.mkv", IMedia::Type::Video );
    ml->addMedia( "video.avi", IMedia::Type::Video );
    ml->addMedia( "lonelyotter.mkv", IMedia::Type::Video );

    QueryParameters params{};
    params.sort = SortingCriteria::NbMedia;
    params.desc = false;
    auto groups = ml->videoGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->count() );
    ASSERT_EQ( 2u, groups[1]->count() );

    params.desc = true;
    groups = ml->videoGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 2u, groups[0]->count() );
    ASSERT_EQ( 1u, groups[1]->count() );

    // Descending alpha order: «video.» 1st, «lonely» 2nd
    params.sort = SortingCriteria::Alpha;
    groups = ml->videoGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 2u, groups[0]->count() );
    ASSERT_EQ( 1u, groups[1]->count() );
}

TEST_F( VideoGroups, ListMedia )
{
    ml->addMedia( "avideo.mkv", IMedia::Type::Video );
    ml->addMedia( "avideo.avi", IMedia::Type::Video );
    ml->addMedia( "zsomethingelse.mkv", IMedia::Type::Video );

    auto groups = ml->videoGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    auto g = groups[0];
    ASSERT_EQ( g->name(), "avideo" );
    auto media = g->media( nullptr )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( "avideo.avi", media[0]->title() );
    ASSERT_EQ( "avideo.mkv", media[1]->title() );
}

TEST_F( VideoGroups, SortMedia )
{
    auto m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "avideo.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
        ml->addMedia( "avideo.avi", IMedia::Type::Video ) );
    ml->addMedia( "zsomethingelse.mkv", IMedia::Type::Video );

    m1->setDuration( 9999 );
    m1->save();
    m2->setDuration( 1 );
    m2->save();

    auto groups = ml->videoGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    auto g = groups[0];
    ASSERT_EQ( g->name(), "avideo" );
    QueryParameters params;
    params.sort = SortingCriteria::Duration;
    params.desc = false;
    auto media = g->media( &params )->all();
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( 1, media[0]->duration() );
    ASSERT_EQ( m1->id(), media[1]->id() );
    ASSERT_EQ( 9999, media[1]->duration() );

    params.desc = true;
    media = g->media( &params )->all();
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( 9999, media[0]->duration() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( 1, media[1]->duration() );
}

TEST_F( VideoGroups, MediaPaging )
{
    ml->addMedia( "video.mkv", IMedia::Type::Video );
    ml->addMedia( "video.avi", IMedia::Type::Video );
    ml->addMedia( "lonelyotter.mkv", IMedia::Type::Video );

    QueryParameters params{};
    params.sort = SortingCriteria::NbMedia;
    params.desc = true;
    auto groups = ml->videoGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    auto group = groups[0];
    ASSERT_EQ( 2u, group->count() );
    auto mediaQuery = group->media( nullptr );
    ASSERT_EQ( 2u, mediaQuery->count() );
    auto media = mediaQuery->items( 1, 0 );
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( "video.avi", media[0]->title() );

    media = mediaQuery->items( 1, 1 );
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( "video.mkv", media[0]->title() );

    media = mediaQuery->items( 1, 2 );
    ASSERT_EQ( 0u, media.size() );
}

TEST_F( VideoGroups, SearchMedia )
{
    ml->addMedia( "groupname foo.mkv", IMedia::Type::Video );
    ml->addMedia( "groupname bar", IMedia::Type::Video );
    ml->addMedia( "foo.avi", IMedia::Type::Video );
    ml->addMedia( "bar.mkv", IMedia::Type::Video );
    ml->addMedia( "12345678.mkv", IMedia::Type::Video );

    QueryParameters params;
    params.sort = SortingCriteria::NbMedia;
    params.desc = true;
    auto groups = ml->videoGroups( &params )->all();
    ASSERT_EQ( 4u, groups.size() );
    auto group = groups[0];
    ASSERT_EQ( "groupn", group->name() );
    ASSERT_EQ( 2u, group->count() );

    auto mediaQuery = group->searchMedia( "no", nullptr );
    ASSERT_EQ( nullptr, mediaQuery );

    mediaQuery = group->searchMedia( "foo", nullptr );
    ASSERT_EQ( 1u, mediaQuery->count() );
    auto media = mediaQuery->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( "groupname foo.mkv", media[0]->title() );

    mediaQuery = group->searchMedia( "plonkitiplonk", nullptr );
    ASSERT_EQ( 0u, mediaQuery->count() );

    // Search for a numerical pattern, but also a partial word (the previous
    // tests were only testing for a full word match)
    group = ml->videoGroup( "123456" );
    ASSERT_NE( nullptr, group );
    mediaQuery = group->searchMedia( "123456", nullptr );
    ASSERT_EQ( 1u, mediaQuery->count() );
    media = mediaQuery->all();
    ASSERT_EQ( 1u, media.size() );
}

TEST_F( VideoGroups, IgnorePrefix )
{
    ml->addMedia( "The groupname.mkv", IMedia::Type::Video );
    ml->addMedia( "The groupname.avi", IMedia::Type::Video );
    ml->addMedia( "the groupname.avi", IMedia::Type::Video );
    ml->addMedia( "groupname.mkv", IMedia::Type::Video );
    ml->addMedia( "Theremin.mkv", IMedia::Type::Video );

    QueryParameters params;
    params.sort = SortingCriteria::NbVideo;
    params.desc = true;
    auto groups = ml->videoGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( "groupn", groups[0]->name() );
    ASSERT_EQ( 4u, groups[0]->count() );
    ASSERT_EQ( "Theremin.mkv", groups[1]->name() );
    ASSERT_EQ( 1u, groups[1]->count() );

    auto mediaQuery = groups[0]->media( nullptr );
    ASSERT_EQ( 4u, mediaQuery->count() );
    ASSERT_EQ( 4u, mediaQuery->all().size() );
    mediaQuery = groups[0]->searchMedia( "groupname", nullptr );
    ASSERT_EQ( 4u, mediaQuery->count() );
    ASSERT_EQ( 4u, mediaQuery->all().size() );
}

TEST_F( VideoGroups, AdjustSize )
{
    ml->addMedia( "Otters are cool.mkv", IMedia::Type::Video );
    ml->addMedia( "Otters are fluffy.mkv", IMedia::Type::Video );
    ml->addMedia( "Otters are cute.mkv", IMedia::Type::Video );
    ml->addMedia( "OtteITSATRAP.mkv", IMedia::Type::Video );

    auto groups = ml->videoGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );

    ml->setVideoGroupsPrefixLength( 4 );
    groups = ml->videoGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
}

TEST_F( VideoGroups, GetByName )
{
    ml->addMedia( "Otters are cool.mkv", IMedia::Type::Video );
    ml->addMedia( "Otters are fluffy.mkv", IMedia::Type::Video );
    ml->addMedia( "Otters are cute.mkv", IMedia::Type::Video );

    auto group = ml->videoGroup( "Otters" );
    ASSERT_NE( nullptr, group );
    ASSERT_EQ( 3u, group->count() );

    group = ml->videoGroup( "otter" );
    ASSERT_EQ( nullptr, group );

    group = ml->videoGroup( "otter are" );
    ASSERT_EQ( nullptr, group );

    ml->setVideoGroupsPrefixLength( 5 );
    group = ml->videoGroup( "otters" );
    ASSERT_EQ( nullptr, group );

    group = ml->videoGroup( "otter" );
    ASSERT_NE( nullptr, group );
    ASSERT_EQ( 3u, group->count() );
}

TEST_F( VideoGroups, CaseInsensitive )
{
    ml->addMedia( "otters are cool.mkv", IMedia::Type::Video );
    ml->addMedia( "OTTERS are fluffy.mkv", IMedia::Type::Video );
    ml->addMedia( "OtTeRs are cute.mkv", IMedia::Type::Video );

    auto groups = ml->videoGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    auto group = ml->videoGroup( "OTTERS" );
    ASSERT_NE( nullptr, group );

    group = ml->videoGroup( "OTtERs" );
    ASSERT_NE( nullptr, group );

    group = ml->videoGroup( "otters" );
    ASSERT_NE( nullptr, group );

    // Now ensure that we are capable of fetching the media
    auto mediaQuery = group->media( nullptr );
    ASSERT_EQ( 3u, mediaQuery->count() );
    auto media = mediaQuery->all();
    ASSERT_EQ( 3u, media.size() );
}

TEST_F( VideoGroups, UseMediaName )
{
    /* Check that a group with a single media reports the media title as its name */
    auto m = ml->addMedia( "otters otters otters.mkv", IMedia::Type::Video );
    auto groups = ml->videoGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( m->title(), groups[0]->name() );

    auto group = ml->videoGroup( m->title() );
    ASSERT_NE( nullptr, group );
    ASSERT_EQ( 1u, group->count() );
    ASSERT_EQ( m->title(), group->name() );
}
