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
    auto mg = ml->createMediaGroup( name );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "group", mg->name() );
    ASSERT_EQ( 0u, mg->nbVideo() );
    ASSERT_EQ( 0u, mg->nbAudio() );
    ASSERT_EQ( 0u, mg->nbMedia() );
    ASSERT_EQ( 0u, mg->nbUnknown() );
    ASSERT_EQ( true, mg->userInteracted() );

    mg = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( mg->id() ) );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "group", mg->name() );
}

TEST_F( MediaGroups, ListAll )
{
    auto mg1 = ml->createMediaGroup( "weasels group" );
    auto mg2 = ml->createMediaGroup( "pangolin group" );
    auto mg3 = ml->createMediaGroup( "otters group" );

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
    auto mg = ml->createMediaGroup( "group" );
    ASSERT_NE( nullptr, mg );

    auto mg2 = ml->mediaGroup( mg->id() );
    ASSERT_NE( nullptr, mg2 );
    ASSERT_EQ( mg->id(), mg2->id() );
}

TEST_F( MediaGroups, Search )
{
    auto mg1 = ml->createMediaGroup( "otter group" );
    auto mg2 = ml->createMediaGroup( "weasels group" );

    auto q = ml->searchMediaGroups( "12", nullptr );
    ASSERT_EQ( nullptr, q );

    QueryParameters params { SortingCriteria::Alpha, false };
    q = ml->searchMediaGroups( "group", &params );
    ASSERT_NE( nullptr, q );
    ASSERT_EQ( 2u, q->count() );
    auto groups = q->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );

    params.desc = true;
    groups = ml->searchMediaGroups( "group", &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    groups = ml->searchMediaGroups( "otter", nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
}

TEST_F( MediaGroups, FetchMedia )
{
    auto mg = ml->createMediaGroup( "group" );
    auto video = ml->addMedia( "video.mkv", IMedia::Type::Video );
    auto audio = ml->addMedia( "audio.mp3", IMedia::Type::Audio );
    ASSERT_NE( nullptr, mg );
    ASSERT_NE( nullptr, video );
    ASSERT_NE( nullptr, audio );

    auto mediaQuery = mg->media( IMedia::Type::Unknown, nullptr );
    ASSERT_EQ( 0u, mediaQuery->count() );
    auto media = mediaQuery->all();
    ASSERT_EQ( 0u, media.size() );

    auto res = mg->add( *video );
    ASSERT_TRUE( res );
    res = mg->add( *audio );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, mediaQuery->count() );
    media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( audio->id(), media[0]->id() );
    ASSERT_EQ( video->id(), media[1]->id() );

    media = mg->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( video->id(), media[0]->id() );

    // Now remove media from groups
    res = mg->remove( *video );
    ASSERT_TRUE( res );

    media = mg->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    // Check for the cached value
    ASSERT_EQ( 1u, mg->nbMedia() );
    // And check that the DB was updated
    mg = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( mg->id() ) );
    ASSERT_EQ( 1u, mg->nbMedia() );

    // Don't remove more, since the media group would be deleted.
    // This is checked by the DeleteEmpty test
}

TEST_F( MediaGroups, SearchMedia )
{
    auto mg = ml->createMediaGroup( "" );
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
    auto group1 = ml->createMediaGroup( "group" );
    auto group2 = ml->createMediaGroup( "group2" );
    ASSERT_NE( nullptr, group1 );
    ASSERT_NE( nullptr, group2 );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 0u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 0u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 0u, group2->nbUnknown() );

    // Insert an unknown media in a group
    // Also insert a media for each group, to avoid their count to reach 0 which
    // would cause the group to be deleted
    auto m = ml->addMedia( "media.mkv", IMedia::Type::Unknown );
    auto m2 = ml->addMedia( "media2.avi", IMedia::Type::Video );
    auto m3 = ml->addMedia( "media3.mp3", IMedia::Type::Audio );
    group1->add( *m );
    group1->add( *m2 );
    group2->add( *m3 );

    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 1u, group1->nbVideo() );
    ASSERT_EQ( 1u, group1->nbUnknown() );
    ASSERT_EQ( 2u, group1->nbMedia() );
    ASSERT_EQ( 1u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 0u, group2->nbUnknown() );
    ASSERT_EQ( 1u, group2->nbMedia() );

    // Move that media to another group
    group2->add( *m );

    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 1u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 1u, group1->nbMedia() );
    ASSERT_EQ( 1u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 1u, group2->nbUnknown() );
    ASSERT_EQ( 2u, group2->nbMedia() );

    // Now change the media type
    m->setType( IMedia::Type::Audio );
    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 1u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 2u, group2->nbAudio() );
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
    ASSERT_EQ( 2u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 1u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 0u, group2->nbUnknown() );

    // Now remove the media from the group:
    group1->remove( m->id() );
    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 1u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
    ASSERT_EQ( 1u, group2->nbAudio() );
    ASSERT_EQ( 0u, group2->nbVideo() );
    ASSERT_EQ( 0u, group2->nbUnknown() );
}

TEST_F( MediaGroups, UpdateNbMediaNoDelete )
{
    auto group1 = ml->createMediaGroup( "group" );
    ASSERT_NE( nullptr, group1 );
    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 0u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );

    // Insert an unknown media in a group
    // Also insert a media for each group, to avoid their count to reach 0 which
    // would cause the group to be deleted
    auto m = ml->addMedia( "media.mkv", IMedia::Type::Unknown );
    group1->add( *m );

    ASSERT_EQ( 0u, group1->nbAudio() );
    ASSERT_EQ( 0u, group1->nbVideo() );
    ASSERT_EQ( 1u, group1->nbUnknown() );

    // Now change the media type
    m->setType( IMedia::Type::Audio );

    group1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( group1->id() ) );
    ASSERT_EQ( 1u, group1->nbAudio() );
    ASSERT_EQ( 0u, group1->nbVideo() );
    ASSERT_EQ( 0u, group1->nbUnknown() );
}

TEST_F( MediaGroups, SortByNbMedia )
{
    auto mg1 = ml->createMediaGroup( "A group" );
    auto mg2 = ml->createMediaGroup( "Z group" );

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

TEST_F( MediaGroups, Rename )
{
    auto m = ml->addMedia( "media.mkv", IMedia::Type::Video );
    ASSERT_NE( nullptr, m );
    auto mg = ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
    ASSERT_TRUE( mg->userInteracted() );
    ASSERT_NE( nullptr, mg );

    auto groupMedia = mg->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groupMedia.size() );

    std::string newName{ "better name" };
    auto res = mg->rename( newName );
    ASSERT_TRUE( res );
    ASSERT_TRUE( mg->userInteracted() );
    ASSERT_EQ( newName, mg->name() );

    mg = ml->mediaGroup( mg->id() );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( newName, mg->name() );
    ASSERT_TRUE( mg->userInteracted() );

    groupMedia = mg->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groupMedia.size() );

    res = mg->rename( "" );
    ASSERT_FALSE( res );
}

TEST_F( MediaGroups, CheckDbModel )
{
    auto res = MediaGroup::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( MediaGroups, Delete )
{
    auto mg = ml->createMediaGroup( "group" );
    auto m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "sea otters.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                ml->addMedia( "fluffy otters.mkv", IMedia::Type::Video ) );
    mg->add( m1->id() );
    mg->add( m2->id() );

    auto groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    m1 = ml->media( m1->id() );
    ASSERT_NE( nullptr, m1->group() );
    ASSERT_EQ( mg->id(), m1->groupId() );

    ml->deleteMediaGroup( mg->id() );

    m1 = ml->media( m1->id() );
    m2 = ml->media( m2->id() );
    ASSERT_NE( nullptr, m1->group() );
    auto lockedGroup = std::static_pointer_cast<MediaGroup>( m1->group() );
    ASSERT_TRUE( lockedGroup->isForcedSingleton() );
    ASSERT_EQ( m1->title(), lockedGroup->name() );

    lockedGroup = std::static_pointer_cast<MediaGroup>( m2->group() );
    ASSERT_TRUE( lockedGroup->isForcedSingleton() );
    ASSERT_EQ( m2->title(), lockedGroup->name() );

    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( m2->groupId(), groups[0]->id() );
    ASSERT_EQ( m1->groupId(), groups[1]->id() );
}

TEST_F( MediaGroups, DeleteMedia )
{
    auto mg = ml->createMediaGroup( "group" );
    auto m1 = ml->addMedia( "otters.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "squeaking otters.mp3", IMedia::Type::Audio );
    auto m3 = ml->addMedia( "unknown otters.ts", IMedia::Type::Unknown );
    ASSERT_NE( nullptr, mg );
    ASSERT_NE( nullptr, m1 );
    ASSERT_NE( nullptr, m2 );
    ASSERT_NE( nullptr, m3 );

    auto res = mg->add( *m1 );
    ASSERT_TRUE( res );
    res = mg->add( *m2 );
    ASSERT_TRUE( res );
    res = mg->add( *m3 );
    ASSERT_TRUE( res );

    ASSERT_EQ( 3u, mg->nbMedia() );
    ASSERT_EQ( 1u, mg->nbAudio() );
    ASSERT_EQ( 1u, mg->nbVideo() );
    ASSERT_EQ( 1u, mg->nbUnknown() );
    // Ensure the value in DB is correct
    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( 3u, mg->nbMedia() );
    ASSERT_EQ( 1u, mg->nbAudio() );
    ASSERT_EQ( 1u, mg->nbVideo() );
    ASSERT_EQ( 1u, mg->nbUnknown() );

    // Delete media and ensure the group media count is updated
    ml->deleteMedia( m1->id() );
    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( 2u, mg->nbMedia() );
    ASSERT_EQ( 0u, mg->nbVideo() );

    ml->deleteMedia( m2->id() );
    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( 1u, mg->nbMedia() );
    ASSERT_EQ( 0u, mg->nbAudio() );

    ml->deleteMedia( m3->id() );
    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( nullptr, mg );
}


TEST_F( MediaGroups, DeleteEmpty )
{
    /*
     * Create 3 groups with the 3 different media type, and check that deleting
     * every media is causing the group to be deleted as well
     */
    auto m1 = ml->addMedia( "media1.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "media2.mp3", IMedia::Type::Audio );
    auto m3 = ml->addMedia( "media3.ts", IMedia::Type::Unknown );

    auto mg1 = ml->createMediaGroup( std::vector<int64_t>{ m1->id() } );
    auto mg2 = ml->createMediaGroup( std::vector<int64_t>{ m2->id() } );
    auto mg3 = ml->createMediaGroup( std::vector<int64_t>{ m3->id() } );

    auto groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 3u, groups.size() );

    ml->deleteMedia( m1->id() );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    mg1 = ml->mediaGroup( mg1->id() );
    ASSERT_EQ( nullptr, mg1 );
    mg2 = ml->mediaGroup( mg2->id() );
    ASSERT_NE( nullptr, mg2 );
    mg3 = ml->mediaGroup( mg3->id() );
    ASSERT_NE( nullptr, mg3 );

    ml->deleteMedia( m2->id() );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    mg2 = ml->mediaGroup( mg2->id() );
    ASSERT_EQ( nullptr, mg2 );
    mg3 = ml->mediaGroup( mg3->id() );
    ASSERT_NE( nullptr, mg3 );

    ml->deleteMedia( m3->id() );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );
    mg3 = ml->mediaGroup( mg3->id() );
    ASSERT_EQ( nullptr, mg3 );
}

TEST_F( MediaGroups, CommonPattern )
{
    auto res = MediaGroup::commonPattern( "", "" );
    ASSERT_EQ( "", res );

    res = MediaGroup::commonPattern( "The ", "The " );
    ASSERT_EQ( "", res );

    res = MediaGroup::commonPattern( "This matches perfectly",
                                    "This matches perfectly" );
    ASSERT_EQ( "This matches perfectly", res );

    res = MediaGroup::commonPattern( "This matches perfectly.mkv",
                                    "This matches perfectly.avi" );
    ASSERT_EQ( "This matches perfectly.", res );

    res = MediaGroup::commonPattern( "THIS KIND OF MATCHES.avi",
                                    "this KiNd of MatchES.mkv" );
    ASSERT_EQ( "THIS KIND OF MATCHES.", res );

    // Not enough character match, so this returns a no-match
    res = MediaGroup::commonPattern( "Smallmatch", "smalldifference" );
    ASSERT_EQ( "", res );

    res = MediaGroup::commonPattern( "Small", "sma" );
    ASSERT_EQ( "", res );

    res = MediaGroup::commonPattern( "The match is real", "match is real" );
    ASSERT_EQ( "match is real", res );

    res = MediaGroup::commonPattern( "match is real", "The match is real" );
    ASSERT_EQ( "match is real", res );
}

TEST_F( MediaGroups, AssignToGroups )
{
    auto m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "The otters are fluffy.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                ml->addMedia( "otters are cute.mkv", IMedia::Type::Video ) );
    /*
     * Add a media with a title smaller than the common prefix for groups. It
     * shouldn't be grouped with anything else, and will have its own group
     * Since the title sanitizer doesn't run here, we need to omit the extension
     * for the title to be actually less than 6 chars
     */
    auto m3 = std::static_pointer_cast<Media>(
                ml->addMedia( "the otter", IMedia::Type::Video ) );

    auto groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );

    /*
     * Create a forced singleton group to ensure it's not considered a valid
     * candidate for grouping
     */
    auto m4 = ml->addMedia( "otters are so fluffy.mkv", IMedia::Type::Video );
    auto mg = ml->createMediaGroup( std::vector<int64_t>{ m4->id() } );
    auto res = mg->remove( *m4 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_TRUE( static_cast<MediaGroup*>( groups[0].get() )->isForcedSingleton() );

    /*
     * First assign m1, then m2, and do it again for a different group to check
     * that the "the " prefix is correctly handled regardless of the insertion
     * order
     */
    res = MediaGroup::assignToGroup( ml.get(), *m1 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->nbVideo() );
    ASSERT_EQ( groups[0]->name(), "otters are fluffy.mkv" );
    ASSERT_EQ( m4->groupId(), groups[1]->id() );
    ASSERT_TRUE( static_cast<MediaGroup*>( groups[1].get() )->isForcedSingleton() );
    ASSERT_EQ( m4->title(), groups[1]->name() );

    res = MediaGroup::assignToGroup( ml.get(), *m2 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 2u, groups[0]->nbVideo() );
    ASSERT_EQ( groups[0]->name(), "otters are " );

    res = MediaGroup::assignToGroup( ml.get(), *m3 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->nbVideo() );
    ASSERT_EQ( groups[0]->name(), "otter" );
    ASSERT_EQ( 2u, groups[1]->nbVideo() );
    ASSERT_EQ( groups[1]->name(), "otters are " );

    /*
     * Delete the media since they have been grouped, and assignToGroup asserts
     * that the media have never been grouped, which is a valid assertion as
     * grouping comes from the metadataparser, which only groups media when they
     * were never grouped
     */
    ml->deleteMedia( m1->id() );
    ml->deleteMedia( m2->id() );
    ml->deleteMedia( m3->id() );
    ml->deleteMedia( m4->id() );

    /* Which should delete all the groups */
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );

    m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "The otters are fluffy.mkv", IMedia::Type::Video ) );
    m2 = std::static_pointer_cast<Media>(
                ml->addMedia( "otters are cute.mkv", IMedia::Type::Video ) );
    m3 = std::static_pointer_cast<Media>(
                ml->addMedia( "the otter", IMedia::Type::Video ) );

    m4 = ml->addMedia( "otters are so fluffy.mkv", IMedia::Type::Video );
    mg = ml->createMediaGroup( std::vector<int64_t>{ m4->id() } );
    res = mg->remove( *m4 );

    /* Now try again with the other ordering */
    res = MediaGroup::assignToGroup( ml.get(), *m3 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->nbVideo() );
    ASSERT_EQ( groups[0]->name(), "otter" );

    res = MediaGroup::assignToGroup( ml.get(), *m2 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( 1u, groups[1]->nbVideo() );
    ASSERT_EQ( groups[1]->name(), "otters are cute.mkv" );

    res = MediaGroup::assignToGroup( ml.get(), *m1 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( 2u, groups[1]->nbVideo() );
    ASSERT_EQ( groups[1]->name(), "otters are " );
}

TEST_F( MediaGroups, CreateFromMedia )
{
    auto m1 = ml->addMedia( "media1.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "media2.mkv", IMedia::Type::Video );
    auto m3 = ml->addMedia( "media3.mkv", IMedia::Type::Video );

    auto mg = ml->createMediaGroup( std::vector<int64_t>{ m1->id(), m2->id() } );
    ASSERT_NE( nullptr, mg );
    ASSERT_TRUE( mg->userInteracted() );

    ASSERT_EQ( 2u, mg->nbVideo() );
    ASSERT_EQ( 0u, mg->nbAudio() );
    ASSERT_EQ( 2u, mg->nbMedia() );

    auto mediaQuery = mg->media( IMedia::Type::Video, nullptr );
    ASSERT_EQ( 2u, mediaQuery->count() );
    auto media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );

    auto mg2 = ml->createMediaGroup( std::vector<int64_t>{ m3->id(), m2->id() } );
    ASSERT_NE( nullptr, mg2 );

    ASSERT_EQ( 2u, mg2->nbVideo() );
    ASSERT_EQ( 0u, mg2->nbAudio() );
    ASSERT_EQ( 2u, mg2->nbMedia() );

    mediaQuery = mg2->media( IMedia::Type::Video, nullptr );
    ASSERT_EQ( 2u, mediaQuery->count() );
    media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );

    // Double check that m2 was removed from mg
    mediaQuery = mg->media( IMedia::Type::Video, nullptr );
    ASSERT_EQ( 1u, mediaQuery->count() );
    media = mediaQuery->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

TEST_F( MediaGroups, CreateWithUnknownMedia )
{
    auto mg = ml->createMediaGroup( std::vector<int64_t>{ 7, 8, 9, 10 } );
    ASSERT_EQ( nullptr, mg );

    auto m = ml->addMedia( "media.mkv", IMedia::Type::Video );
    ASSERT_NE( nullptr, m );
    mg = ml->createMediaGroup( std::vector<int64_t>{ m->id(), 13, 12 } );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( 1u, mg->nbMedia() );
}

TEST_F( MediaGroups, CheckFromMediaName )
{
    /*
     * First create a group with an existing match, then create a group with
     * no match. Ensure the first group has a name, and the 2nd doesn't
     */
    auto m1 = ml->addMedia( "otters are fluffy.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "otters are cute.mkv", IMedia::Type::Video );
    auto m3 = ml->addMedia( "pangolins don't match otters.mkv", IMedia::Type::Video );

    auto mg = ml->createMediaGroup( std::vector<int64_t>{ m1->id(), m2->id() } );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "otters are ", mg->name() );
    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( "otters are ", mg->name() );

    mg = ml->createMediaGroup( std::vector<int64_t>{ m3->id(), m2->id() } );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "", mg->name() );
    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( "", mg->name() );
}

TEST_F( MediaGroups, RemoveMedia )
{
    /*
     * Ensure that when a media is removed from a group, an automatic locked
     * group gets created to contain that media
     */
    auto m = ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto mg = std::static_pointer_cast<MediaGroup>(
                ml->createMediaGroup( std::vector<int64_t>{ m->id() } ) );
    ASSERT_NE( nullptr, mg );
    ASSERT_FALSE( mg->isForcedSingleton() );

    auto groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    // Refresh the media since it needs to know it's part of a group
    m = ml->media( m->id() );

    auto res = m->removeFromGroup();
    ASSERT_TRUE( res );

    /* The previous group will be removed, but a new one should be created */
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    ASSERT_NE( groups[0]->id(), mg->id() );
    auto lockedGroup = std::static_pointer_cast<MediaGroup>( groups[0] );
    ASSERT_TRUE( lockedGroup->isForcedSingleton() );
    ASSERT_EQ( lockedGroup->name(), m->title() );
    ASSERT_EQ( 1u, lockedGroup->nbVideo() );
    ASSERT_EQ( 0u, lockedGroup->nbAudio() );
    ASSERT_EQ( 0u, lockedGroup->nbUnknown() );

    ml->deleteMedia( m->id() );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );

    /* Now try again with the other way of removing media from a group */

    m = ml->addMedia( "media.mkv", IMedia::Type::Video );
    mg = std::static_pointer_cast<MediaGroup>(
                ml->createMediaGroup( std::vector<int64_t>{ m->id() } ) );
    ASSERT_NE( nullptr, mg );
    ASSERT_FALSE( mg->isForcedSingleton() );

    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    res = mg->remove( m->id() );
    ASSERT_TRUE( res );

    /* The previous group will be removed, but a new one should be created */
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    ASSERT_NE( groups[0]->id(), mg->id() );
    lockedGroup = std::static_pointer_cast<MediaGroup>( groups[0] );
    ASSERT_TRUE( lockedGroup->isForcedSingleton() );
    ASSERT_EQ( lockedGroup->name(), m->title() );
    ASSERT_EQ( 1u, lockedGroup->nbVideo() );
    ASSERT_EQ( 0u, lockedGroup->nbAudio() );
    ASSERT_EQ( 0u, lockedGroup->nbUnknown() );

    ml->deleteMedia( m->id() );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );
}

TEST_F( MediaGroups, RegroupLocked )
{
    /*
     * We only regroup media that are part of a locked group, so we need a bit
     * of setup to create the associated locked groups. We will add each media
     * to a group, and remove those from there immediatly after
     */
    auto createLockedGroup = [this]( MediaPtr m ) {
        auto mg = ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
        auto res = mg->remove( *m );
        ASSERT_TRUE( res );
    };
    // 2 media we expect to be regrouped
    auto m1 = ml->addMedia( "matching title 1.mkv", IMedia::Type::Video );
    createLockedGroup( m1 );
    auto m2 = ml->addMedia( "MATCHING TITLE 2.mkv", IMedia::Type::Video );
    createLockedGroup( m2 );
    // And another one, but with a slightly different name to check we adjust
    // the group name accordingly
    auto m3 = ml->addMedia( "matching trout.mkv", IMedia::Type::Video );
    createLockedGroup( m3 );
    // A video that won't match the automatic grouping patterns
    auto m4 = ml->addMedia( "fluffy otters is no match for you.avi", IMedia::Type::Video );
    createLockedGroup( m4 );
    // A video that should match but which will is not part of a locked group
    auto m5 = ml->addMedia( "matching title 3.mkv", IMedia::Type::Video );

    auto mg = ml->createMediaGroup( std::vector<int64_t>{ m5->id() } );
    m5 = ml->media( m5->id() );
    ASSERT_EQ( mg->id(), m5->groupId() );

    auto res = m1->regroup();
    ASSERT_TRUE( res );

    m2 = ml->media( m2->id() );
    ASSERT_EQ( m1->groupId(), m2->groupId() );

    m3 = ml->media( m3->id() );
    ASSERT_EQ( m1->groupId(), m3->groupId() );

    m4 = ml->media( m4->id() );

    m5 = ml->media( m5->id() );
    ASSERT_NE( m5->groupId(), m1->groupId() );

    auto newGroup = m1->group();
    ASSERT_EQ( 3u, newGroup->nbVideo() );
    ASSERT_EQ( "matching t", newGroup->name() );

    // Ensure we refuse to regroup an already grouped media
    res = m5->regroup();
    ASSERT_FALSE( res );
}

TEST_F( MediaGroups, ForcedSingletonRestrictions )
{
    auto m = ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "media2.mkv", IMedia::Type::Video );
    auto mg = ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
    auto res = mg->remove( *m );
    ASSERT_TRUE( res );
    mg = m->group();

    res = mg->rename( "Another name" );
    ASSERT_FALSE( res );
    ASSERT_EQ( "media.mkv", mg->name() );

    res = mg->destroy();
    ASSERT_FALSE( res );
    auto groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( groups[0]->id(), mg->id() );
}

TEST_F( MediaGroups, AddToForcedSingleton )
{
    auto createForcedSingleton = [this]( MediaPtr m ) {
        auto mg = ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
        auto res = mg->remove( *m );
        ASSERT_TRUE( res );
    };
    auto m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    createForcedSingleton( m1 );
    createForcedSingleton( m2 );

    auto groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    auto g1 = std::static_pointer_cast<MediaGroup>( groups[0] );
    auto g2 = std::static_pointer_cast<MediaGroup>( groups[1] );

    ASSERT_TRUE( g1->isForcedSingleton() );
    ASSERT_TRUE( g2->isForcedSingleton() );

    auto media = g2->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    auto res = g1->add( *media[0] );
    ASSERT_TRUE( res );
    ASSERT_FALSE( g1->isForcedSingleton() );
    ASSERT_EQ( 2u, g1->nbMedia() );

    g1 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( g1->id() ) );
    ASSERT_FALSE( g1->isForcedSingleton() );
    ASSERT_EQ( 2u, g1->nbMedia() );

    g2 = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( g2->id() ) );
    ASSERT_EQ( nullptr, g2 );
}

TEST_F( MediaGroups, RenameForcedSingleton )
{
    auto m = ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto mg = ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
    mg->remove( *m );
    auto singleton = m->group();
    ASSERT_NE( mg->id(), singleton->id() );

    auto groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( singleton->id(), groups[0]->id() );
    ASSERT_EQ( m->title(), groups[0]->name() );

    auto res = m->setTitle( "sea otter makes a better title" );
    ASSERT_TRUE( res );

    singleton = ml->mediaGroup( singleton->id() );
    ASSERT_EQ( m->title(), singleton->name() );

    /* Ensure we don't do anything for non singleton groups */
    mg = ml->createMediaGroup( "otters group" );
    res = mg->add( *m );
    ASSERT_TRUE( res );

    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    res = m->setTitle( "Better otter related title" );
    ASSERT_TRUE( res );

    mg = ml->mediaGroup( mg->id() );
    ASSERT_NE( mg->name(), m->title() );
}

TEST_F( MediaGroups, UpdateDuration )
{
    auto mg = ml->createMediaGroup( "test group" );
    ASSERT_EQ( 0, mg->duration() );

    auto m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "media.mkv", IMedia::Type::Video ) );

    /* Insert a media with an already known duration */
    m1->setDuration( 123 );
    m1->save();
    auto res = mg->add( *m1 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 123, mg->duration() );

    /* Check the result in DB as well */
    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( 123, mg->duration() );

    /*
     * Now add a media with its default duration to the group, and only then
     * update the media duration
     */
    auto m2 = std::static_pointer_cast<Media>(
                ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    res = mg->add( *m2 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 123, mg->duration() );

    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( 123, mg->duration() );

    m2->setDuration( 999 );
    m2->save();

    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( 123 + 999, mg->duration() );

    /* Now remove the first media and check that the duration is updated accordingly */
    res = mg->remove( *m1 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 999, mg->duration() );

    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( 999, mg->duration() );

    /* Ensure the new singleton group has the proper duration set */
    m1 = ml->media( m1->id() );
    auto singleton = m1->group();
    ASSERT_NE( nullptr, singleton );
    ASSERT_NE( singleton->id(), mg->id() );
    ASSERT_EQ( m1->duration(), singleton->duration() );

    /*
     * Now add a media with a default duration and remove it, check that
     * nothing changes
     */
    auto m3 = ml->addMedia( "media3.mkv", IMedia::Type::Video );
    res = mg->add( *m3 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 999, mg->duration() );

    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( 999, mg->duration() );

    mg->remove( *m3 );
    ASSERT_EQ( 999, mg->duration() );

    /*
     * Now remove the last media from the group, and expect the group to be
     * successfully deleted and that the duration update trigger doesn't conflict
     * with the empty group auto removal
     */
    res = mg->remove( *m2 );
    ASSERT_TRUE( res );
    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( nullptr, mg );
}

TEST_F( MediaGroups, OrderByDuration )
{
    auto m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    m1->setDuration( 999 );
    m1->save();
    auto mg1 = ml->createMediaGroup( std::vector<int64_t>{ m1->id() } );
    mg1->rename( "a" );

    auto m2 = std::static_pointer_cast<Media>(
                ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    m2->setDuration( 111 );
    m2->save();
    auto mg2 = ml->createMediaGroup( std::vector<int64_t>{ m2->id() } );
    mg2->rename( "z" );

    QueryParameters params{ SortingCriteria::Default, false };
    auto groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( m1->duration(), groups[0]->duration() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
    ASSERT_EQ( m2->duration(), groups[1]->duration() );

    params.sort = SortingCriteria::Duration;
    groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = true;
    groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
}

TEST_F( MediaGroups, CreationDate )
{
    /*
     * Check that the creation date makes sense.
     */
    auto now = time( nullptr );
    auto mg = ml->createMediaGroup( "group" );
    ASSERT_NE( nullptr, mg );
    /*
     * We can't check for equality since we might create get the current time on
     * the edge of a second, and do the insertion on the other edge
     */
    auto diff = mg->creationDate() - now;
    ASSERT_LE( diff, 1 );

    auto m1 = ml->addMedia( "media.mkv", IMedia::Type::Video );
    now = time( nullptr );
    mg = ml->createMediaGroup( std::vector<int64_t>{ m1->id() } );
    ASSERT_NE( nullptr, mg );
    diff = mg->creationDate() - now;
    ASSERT_LE( diff, 1 );
}

TEST_F( MediaGroups, OrderByCreationDate )
{
    auto forceCreationDate = [this]( int64_t groupId, time_t t ) {
        const std::string req = "UPDATE " + MediaGroup::Table::Name +
                " SET creation_date = ? WHERE id_group = ?";
        return sqlite::Tools::executeUpdate( ml->getConn(), req, t, groupId );
    };
    auto mg1 = ml->createMediaGroup( "a" );
    forceCreationDate( mg1->id(), 999 );
    auto mg2 = ml->createMediaGroup( "z" );
    forceCreationDate( mg2->id(), 111 );

    QueryParameters params{ SortingCriteria::InsertionDate, false };
    auto groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = true;
    groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
}

TEST_F( MediaGroups, LastModificationDate )
{
    auto resetModificationDate = [this]( int64_t groupId ) {
        const std::string req = "UPDATE " + MediaGroup::Table::Name +
                " SET last_modification_date = 0 WHERE id_group = ?";
        return sqlite::Tools::executeUpdate( ml->getConn(), req, groupId );
    };
    auto mg = ml->createMediaGroup( "group" );
    ASSERT_EQ( mg->creationDate(), mg->lastModificationDate() );
    resetModificationDate( mg->id() );

    /* Ensure we update the modification date on insertion */
    auto m1 = ml->addMedia( "media.mkv", IMedia::Type::Video );
    mg->add( *m1 );

    mg = ml->mediaGroup( mg->id() );
    ASSERT_NE( 0, mg->lastModificationDate() );

    /* Ensure we update the modification date on renaming */
    resetModificationDate( mg->id() );
    auto res = mg->rename( "new name" );
    ASSERT_TRUE( res );
    mg = ml->mediaGroup( mg->id() );
    ASSERT_NE( 0, mg->lastModificationDate() );

    /*
     * Ensure we update the modification date on removal.
     * We need to add a 2nd media beforehand, otherwise the group will be deleted
     */
    auto m2 = ml->addMedia( "media2.mkv", IMedia::Type::Video );
    mg->add( *m2 );
    resetModificationDate( mg->id() );
    mg->remove( *m2 );

    mg = ml->mediaGroup( mg->id() );
    ASSERT_NE( 0, mg->lastModificationDate() );

    /* Ensure we update the modification date on media deletion */
    m2 = ml->addMedia( "media3.mkv", IMedia::Type::Video );
    mg->add( *m2 );
    resetModificationDate( mg->id() );
    ml->deleteMedia( m2->id() );

    mg = ml->mediaGroup( mg->id() );
    ASSERT_NE( 0, mg->lastModificationDate() );
}

TEST_F( MediaGroups, OrderByLastModificationDate )
{
    auto forceLastModificationDate = [this]( int64_t groupId, time_t t ) {
        const std::string req = "UPDATE " + MediaGroup::Table::Name +
                " SET last_modification_date = ? WHERE id_group = ?";
        return sqlite::Tools::executeUpdate( ml->getConn(), req, t, groupId );
    };
    auto mg1 = ml->createMediaGroup( "a" );
    forceLastModificationDate( mg1->id(), 999 );
    auto mg2 = ml->createMediaGroup( "z" );
    forceLastModificationDate( mg2->id(), 111 );

    QueryParameters params{ SortingCriteria::LastModificationDate, false };
    auto groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = true;
    groups = ml->mediaGroups( &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
}

TEST_F( MediaGroups, Destroy )
{
    /* Ensures a deleted media group exposes a valid state */
    auto mg = ml->createMediaGroup( "group" );
    auto m1 = ml->addMedia( "media1.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "media2.mkv", IMedia::Type::Video );
    mg->add( *m1 );
    mg->add( *m2 );

    ASSERT_EQ( 2u, mg->nbMedia() );

    mg = ml->mediaGroup( mg->id() );

    auto res = mg->destroy();
    ASSERT_TRUE( res );
    ASSERT_EQ( 0u, mg->nbMedia() );

    mg = ml->mediaGroup( mg->id() );
    ASSERT_EQ( nullptr, mg );
}

TEST_F( MediaGroups, RegroupAll )
{
    auto createLockedGroup = [this]( MediaPtr m ) {
        auto mg = ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
        auto res = mg->remove( *m );
        ASSERT_TRUE( res );
    };
    auto m1 = ml->addMedia( "otters are not grouped.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "pangolins are cute.mkv", IMedia::Type::Video );
    auto m3 = ml->addMedia( "otters are not responsible for COVID19.mkv", IMedia::Type::Video );
    auto m4 = ml->addMedia( "pangolins are vectors of diseases.mkv", IMedia::Type::Video );
    auto m5 = ml->addMedia( "cats are unrelated to otters and pangolins.mkv", IMedia::Type::Video );
    auto m6 = ml->addMedia( "otters is already grouped.mkv", IMedia::Type::Video );

    createLockedGroup( m1 );
    createLockedGroup( m2 );
    createLockedGroup( m3 );
    createLockedGroup( m4 );
    createLockedGroup( m5 );
    auto mg = ml->createMediaGroup( std::vector<int64_t>{ m6->id() } );

    auto groups = ml->mediaGroups( nullptr )->all();
    /* We expect 1 group per media */
    ASSERT_EQ( 6u, groups.size() );

    /* Now regroup all the things!, we expect 4 groups as a result:
     * - otters
     * - pangolins
     * - cats
     * - otters (but the manually grouped one)
     */
    ml->regroupAll();
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 4u, groups.size() );
    ASSERT_EQ( groups[0]->name(), "cats are unrelated to otters and pangolins.mkv" );
    ASSERT_EQ( groups[1]->name(), "otters are not " );
    ASSERT_EQ( groups[2]->name(), "otters is already grouped.mkv" );
    ASSERT_EQ( groups[3]->name(), "pangolins are " );
}
TEST_F( MediaGroups, MergeAutoCreated )
{
    /*
     * Checks that we can appen a media from an automatically created group
     * into another automatically created group
     */
    auto m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    auto res = MediaGroup::assignToGroup( ml.get(), *m1 );
    ASSERT_TRUE( res );
    res = MediaGroup::assignToGroup( ml.get(), *m2 );
    ASSERT_TRUE( res );

    auto groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    auto group1 = groups[0];
    auto group2 = groups[1];
    ASSERT_EQ( 1u, group1->nbMedia() );
    ASSERT_EQ( 1u, group2->nbMedia() );

    auto group2Media = group2->media( IMedia::Type::Unknown )->all();
    ASSERT_EQ( 1u, group2Media.size() );
    res = group1->add( *group2Media[0] );
    ASSERT_TRUE( res );

    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    ASSERT_EQ( 2u, group1->nbMedia() );

    group1 = ml->mediaGroup( group1->id() );
    ASSERT_EQ( 2u, group1->nbMedia() );
}
