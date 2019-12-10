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

#include "MediaGroup.h"
#include "Media.h"

class MediaGroups : public Tests
{
};

TEST_F( MediaGroups, Create )
{
    auto name = std::string{ "group" };
    auto mg = MediaGroup::create( ml.get(), 0, name );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "group", mg->name() );
    ASSERT_EQ( 0u, mg->nbVideo() );
    ASSERT_EQ( 0u, mg->nbAudio() );
    ASSERT_EQ( 0u, mg->nbMedia() );
    ASSERT_EQ( 0u, mg->nbUnknown() );

    Reload();

    mg = MediaGroup::fetchByName( ml.get(), name );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "group", mg->name() );
}

TEST_F( MediaGroups, Unique )
{
    std::string name{ "group" };
    auto mg = MediaGroup::create( ml.get(), 0, name );
    ASSERT_NE( nullptr, mg );
    mg = MediaGroup::create( ml.get(), 0, name );
    ASSERT_EQ( nullptr, mg );
}

TEST_F( MediaGroups, CaseInsensitive )
{
    auto mg = MediaGroup::create( ml.get(), 0, "GrOUpNAmE" );
    ASSERT_NE( nullptr, mg );

    auto mg2 = MediaGroup::fetchByName( ml.get(), "groupname" );
    ASSERT_NE( nullptr, mg2 );
    ASSERT_EQ( mg->id(), mg2->id() );
}

TEST_F( MediaGroups, SubGroup )
{
    auto name = std::string{ "group" };
    auto mg = MediaGroup::create( ml.get(), 0, name );
    ASSERT_FALSE( mg->isSubgroup() );

    auto subname = std::string{ "subgroup" };
    auto subgroup = mg->createSubgroup( subname );

    ASSERT_NE( nullptr, subgroup );
    ASSERT_EQ( subname, subgroup->name() );
    ASSERT_TRUE( subgroup->isSubgroup() );
    auto parent = subgroup->parent();

    ASSERT_NE( nullptr, parent );
    ASSERT_EQ( mg->id(), parent->id() );

    // Ensure the UNIQUE constraint doesn't span accross all groups, but only
    // across groups from the same parent
    auto g = MediaGroup::create( ml.get(), 0, subname );
    ASSERT_NE( nullptr, g );
}

TEST_F( MediaGroups, ListSubGroups )
{
    auto name = std::string{ "group" };
    auto mg = MediaGroup::create( ml.get(), 0, name );

    for ( auto i = 0; i < 3; ++i )
    {
        auto subname = "subgroup_" + std::to_string( 3 - i );
        mg->createSubgroup( subname );
    }

    auto subgroupQuery = mg->subgroups( nullptr );
    ASSERT_EQ( 3u, subgroupQuery->count() );
    auto subgroups = subgroupQuery->all();
    ASSERT_EQ( 3u, subgroups.size() );
    ASSERT_EQ( "subgroup_1", subgroups[0]->name() );
    ASSERT_EQ( "subgroup_2", subgroups[1]->name() );
    ASSERT_EQ( "subgroup_3", subgroups[2]->name() );

    QueryParameters params { SortingCriteria::Alpha, true };
    subgroups = mg->subgroups( &params )->all();

    ASSERT_EQ( 3u, subgroups.size() );
    ASSERT_EQ( "subgroup_3", subgroups[0]->name() );
    ASSERT_EQ( "subgroup_2", subgroups[1]->name() );
    ASSERT_EQ( "subgroup_1", subgroups[2]->name() );
}

TEST_F( MediaGroups, ListAll )
{
    auto mg1 = MediaGroup::create( ml.get(), 0, "weasels group" );
    auto mg2 = MediaGroup::create( ml.get(), 0, "pangolin group" );
    auto mg3 = MediaGroup::create( ml.get(), 0, "otters group" );
    // Subgroups shouldn't be listed
    auto sg = mg1->createSubgroup( "subotters" );

    auto mgQuery = ml->mediaGroups( nullptr );
    ASSERT_EQ( 3u, mgQuery->count() );
    auto groups = mgQuery->all();
    ASSERT_EQ( 3u, groups.size() );

    // Default sort is alphabetical
    ASSERT_EQ( mg3->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
    ASSERT_EQ( mg1->id(), groups[2]->id() );

    QueryParameters params{ SortingCriteria::Alpha, true };
    groups = ml->mediaGroups( &params )->all();

    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
    ASSERT_EQ( mg3->id(), groups[2]->id() );
}

TEST_F( MediaGroups, FetchOne )
{
    auto mg = MediaGroup::create( ml.get(), 0, "group" );
    ASSERT_NE( nullptr, mg );

    auto mg2 = ml->mediaGroup( mg->id() );
    ASSERT_NE( nullptr, mg2 );
    ASSERT_EQ( mg->id(), mg2->id() );
}

TEST_F( MediaGroups, Search )
{
    auto mg1 = MediaGroup::create( ml.get(), 0, "otter group" );
    // Subgroups are included in search results
    auto mg2 = mg1->createSubgroup( "sea otters group" );
    auto mg3 = MediaGroup::create( ml.get(), 0, "weasels group" );

    auto q = ml->searchMediaGroups( "12", nullptr );
    ASSERT_EQ( nullptr, q );

    QueryParameters params { SortingCriteria::Alpha, false };
    q = ml->searchMediaGroups( "group", &params );
    ASSERT_NE( nullptr, q );
    ASSERT_EQ( 3u, q->count() );
    auto groups = q->all();
    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
    ASSERT_EQ( mg3->id(), groups[2]->id() );

    params.desc = true;
    groups = ml->searchMediaGroups( "group", &params )->all();
    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( mg3->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
    ASSERT_EQ( mg1->id(), groups[2]->id() );

    groups = ml->searchMediaGroups( "otter", nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
}

TEST_F( MediaGroups, Media )
{
    auto mg = MediaGroup::create( ml.get(), 0, "group" );
    auto video = ml->addMedia( "video.mkv", IMedia::Type::Video );
    auto audio = ml->addMedia( "audio.mp3", IMedia::Type::Audio );
    ASSERT_NE( nullptr, mg );
    ASSERT_NE( nullptr, video );
    ASSERT_NE( nullptr, audio );

    auto sg = mg->createSubgroup( "subgroup" );
    auto subvideo = ml->addMedia( "subvideo.mkv", IMedia::Type::Video );
    auto subaudio = ml->addMedia( "subaudio.mkv", IMedia::Type::Audio );
    auto subaudio2 = ml->addMedia( "subaudio2.mkv", IMedia::Type::Audio );
    ASSERT_NE( nullptr, sg );
    ASSERT_NE( nullptr, subvideo );

    auto mediaQuery = mg->media( IMedia::Type::Unknown, nullptr );
    ASSERT_EQ( 0u, mediaQuery->count() );
    auto media = mediaQuery->all();
    ASSERT_EQ( 0u, media.size() );

    auto res = mg->add( *video );
    ASSERT_TRUE( res );
    res = mg->add( audio->id() );
    ASSERT_TRUE( res );

    res = subvideo->addToGroup( *sg );
    ASSERT_TRUE( res );
    res = subaudio->addToGroup( sg->id() );
    ASSERT_TRUE( res );
    res = subaudio2->addToGroup( sg->id() );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, mediaQuery->count() );
    media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( audio->id(), media[0]->id() );
    ASSERT_EQ( video->id(), media[1]->id() );

    media = mg->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( video->id(), media[0]->id() );

    media = sg->media( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( subaudio->id(), media[0]->id() );
    ASSERT_EQ( subaudio2->id(), media[1]->id() );

    // Now remove most media from groups
    res = mg->remove( *video );
    ASSERT_TRUE( res );
    res = mg->remove( audio->id() );
    ASSERT_TRUE( res );

    res = subvideo->removeFromGroup();
    ASSERT_TRUE( res );
    res = subaudio->removeFromGroup();
    ASSERT_TRUE( res );

    media = mg->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
    media = sg->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ ( subaudio2->id(), media[0]->id() );
}

TEST_F( MediaGroups, SearchMedia )
{
    auto mg = MediaGroup::create( ml.get(), 0, "" );
    auto m1 = ml->addMedia( "audio.mp3", IMedia::Type::Audio );
    m1->setTitle( "The sea otters podcast" );
    auto m2 = ml->addMedia( "audio2.mp3", IMedia::Type::Audio );
    m2->setTitle( "A boring podcast" );
    auto v1 = ml->addMedia( "a cute otter.mkv", IMedia::Type::Video );
    auto v2 = ml->addMedia( "a boring animal.mkv", IMedia::Type::Video );
    auto v3 = ml->addMedia( "more fluffy otters.mkv", IMedia::Type::Video );

    mg->add( *m1 );
    mg->add( *v2 );
    mg->add( *v3 );

    auto query = mg->searchMedia( "12", IMedia::Type::Unknown, nullptr );
    ASSERT_EQ( nullptr, query );

    query = mg->searchMedia( "otters", IMedia::Type::Audio );
    ASSERT_EQ( 1u, query->count() );
    auto media = query->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );

    media = mg->searchMedia( "otters", IMedia::Type::Unknown )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( v3->id(), media[0]->id() );
    ASSERT_EQ( m1->id(), media[1]->id() );

    media = mg->searchMedia( "boring", IMedia::Type::Audio )->all();
    ASSERT_EQ( 0u, media.size() );

    media = mg->searchMedia( "otter", IMedia::Type::Video )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( v3->id(), media[0]->id() );
}

TEST_F( MediaGroups, UpdateNbMediaTypeChange )
{
    auto group1 = MediaGroup::create( ml.get(), 0, "group" );
    auto group2 = MediaGroup::create( ml.get(), 0, "group2" );
    ASSERT_NE( nullptr, group1 );
    ASSERT_NE( nullptr, group2 );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 0u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 0u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 0u, group2->nbUnknown() );

    // Insert an unknown media in a group
    auto m = ml->addMedia( "media.mkv", IMedia::Type::Unknown );
    group1->add( *m );

    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 0u, group1->nbVideo() );
    ASSERT_EQ( 1u, group1->nbUnknown() );
    ASSERT_EQ( 0u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 0u, group2->nbUnknown() );

    // Move that media to another group
    group2->add( *m );

    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 0u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 0u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 1u, group2->nbUnknown() );

    // Now change the media type
    m->setType( IMedia::Type::Audio );
    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 0u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 1u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 0u, group2->nbUnknown() );

    // Manually change both group & type to check if we properly support it
    std::string req = "UPDATE " + Media::Table::Name +
            " SET type = ?, group_id = ? WHERE id_media = ?";
    auto res = sqlite::Tools::executeUpdate( ml->getConn(), req, IMedia::Type::Video,
                                             group1->id(), m->id() );
    ASSERT_TRUE( res );

    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 1u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 0u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 0u, group2->nbUnknown() );

    // Now remove the media from the group:
    group1->remove( m->id() );
    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 0u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 0u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 0u, group2->nbUnknown() );
}

TEST_F( MediaGroups, SortByNbMedia )
{
    auto mg1 = MediaGroup::create( ml.get(), 0, "A group" );
    auto mg2 = MediaGroup::create( ml.get(), 0, "Z group" );

    auto v1 = ml->addMedia( "media1.mkv", IMedia::Type::Video );
    auto v2 = ml->addMedia( "media2.mkv", IMedia::Type::Video );
    mg1->add( *v1 );
    mg1->add( *v2 );

    auto a1 = ml->addMedia( "audio1.mp3", IMedia::Type::Audio );
    auto u1 = ml->addMedia( "unknown1.ts", IMedia::Type::Unknown );
    auto u2 = ml->addMedia( "unknown2.ts", IMedia::Type::Unknown );
    mg2->add( *a1 );
    mg2->add( *u1 );
    mg2->add( *u2 );

    QueryParameters params{ SortingCriteria::NbVideo, false };

    auto query = ml->mediaGroups( &params );
    ASSERT_EQ( 2u, query->count() );
    auto groups = query->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = true;
    groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );

    params.sort = SortingCriteria::NbMedia;
    // still descending order, so mg2 comes first
    groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = false;
    groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
}

TEST_F( MediaGroups, FetchFromMedia )
{
    auto mg = ml->createMediaGroup( "group" );
    auto m = std::static_pointer_cast<Media>(
                ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    ASSERT_EQ( 0, m->groupId() );
    auto g = m->group();
    ASSERT_EQ( nullptr, g );

    auto res = m->addToGroup( *mg );
    ASSERT_TRUE( res );
    ASSERT_EQ( mg->id(), m->groupId() );
    g = m->group();
    ASSERT_NE( nullptr, g );
    ASSERT_EQ( mg->id(), g->id() );
}
