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

    auto subname = std::string{ "subgroup" };
    auto subgroup = mg->createSubgroup( subname );

    ASSERT_NE( nullptr, subgroup );
    ASSERT_EQ( subname, subgroup->name() );
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
