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
    ASSERT_EQ( true, mg->hasBeenRenamed() );

    Reload();

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
    res = mg->add( audio->id() );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, mediaQuery->count() );
    media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( audio->id(), media[0]->id() );
    ASSERT_EQ( video->id(), media[1]->id() );

    media = mg->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( video->id(), media[0]->id() );

    // Now remove most media from groups
    res = mg->remove( *video );
    ASSERT_TRUE( res );
    res = mg->remove( audio->id() );
    ASSERT_TRUE( res );

    media = mg->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
    // Check for the cached value
    ASSERT_EQ( 0u, mg->nbMedia() );
    // And ensure the DB also was updated:
    mg = std::static_pointer_cast<MediaGroup>( ml->mediaGroup( mg->id() ) );
    ASSERT_EQ( 0u, mg->nbMedia() );
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
    ASSERT_FALSE( mg->hasBeenRenamed() );
    ASSERT_NE( nullptr, mg );

    auto groupMedia = mg->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groupMedia.size() );

    std::string newName{ "better name" };
    auto res = mg->rename( newName );
    ASSERT_TRUE( res );
    ASSERT_TRUE( mg->hasBeenRenamed() );
    ASSERT_EQ( newName, mg->name() );

    Reload();

    mg = ml->mediaGroup( mg->id() );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( newName, mg->name() );
    ASSERT_TRUE( mg->hasBeenRenamed() );

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
    auto media = std::static_pointer_cast<Media>(
                ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    mg->add( media->id() );

    Reload();

    media = ml->media( media->id() );
    ASSERT_NE( nullptr, media->group() );
    ASSERT_EQ( mg->id(), media->groupId() );

    MediaGroup::destroy( ml.get(), mg->id() );

    Reload();

    media = ml->media( media->id() );
    ASSERT_EQ( nullptr, media->group() );
    ASSERT_EQ( 0, media->groupId() );
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
    ASSERT_EQ( 0u, mg->nbMedia() );
    ASSERT_EQ( 0u, mg->nbUnknown() );
}

TEST_F( MediaGroups, DeleteGroup )
{
    auto mg1 = ml->createMediaGroup( "group 1" );
    auto mg2 = ml->createMediaGroup( "group 2" );
    auto m1 = ml->addMedia( "otters.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "squeaking otters.mp3", IMedia::Type::Audio );
    mg1->add( *m1 );
    mg2->add( *m2 );

    Reload();

    mg1 = ml->mediaGroup( mg1->id() );
    mg2 = ml->mediaGroup( mg2->id() );
    ASSERT_EQ( 1u, mg1->nbMedia() );
    ASSERT_EQ( 1u, mg2->nbMedia() );

    m1 = ml->media( m1->id() );
    m2 = ml->media( m2->id() );

    auto res = ml->deleteMediaGroup( mg1->id() );
    ASSERT_TRUE( res );

    mg1 = ml->mediaGroup( mg1->id() );
    ASSERT_EQ( nullptr, mg1 );
    m1 = ml->media( m1->id() );

    res = mg2->destroy();
    ASSERT_TRUE( res );

    mg2 = ml->mediaGroup( mg2->id() );
    ASSERT_EQ( nullptr, mg2 );
    m2 = ml->media( m2->id() );
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
     * First assign m1, then m2, and do it again for a different group to check
     * that the "the " prefix is correctly handled regardless of the insertion
     * order
     */
    auto res = MediaGroup::assignToGroup( ml.get(), *m1 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->nbVideo() );
    ASSERT_EQ( groups[0]->name(), "otters are fluffy.mkv" );

    res = MediaGroup::assignToGroup( ml.get(), *m2 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( 2u, groups[0]->nbVideo() );
    ASSERT_EQ( groups[0]->name(), "otters are " );

    res = MediaGroup::assignToGroup( ml.get(), *m3 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->nbVideo() );
    ASSERT_EQ( groups[0]->name(), "otter" );
    ASSERT_EQ( 2u, groups[1]->nbVideo() );
    ASSERT_EQ( groups[1]->name(), "otters are " );

    groups[0]->destroy();
    groups[1]->destroy();

    /*
     * Delete the media since they have been grouped, and assignToGroup asserts
     * that the media have never been grouped, which is a valid assertion as
     * grouping comes from the metadataparser, which only groups media when they
     * were never grouped
     */
    ml->deleteMedia( m1->id() );
    ml->deleteMedia( m2->id() );
    ml->deleteMedia( m3->id() );

    m1 = std::static_pointer_cast<Media>(
                ml->addMedia( "The otters are fluffy.mkv", IMedia::Type::Video ) );
    m2 = std::static_pointer_cast<Media>(
                ml->addMedia( "otters are cute.mkv", IMedia::Type::Video ) );
    m3 = std::static_pointer_cast<Media>(
                ml->addMedia( "the otter", IMedia::Type::Video ) );

    /* Now try again with the other ordering */
    res = MediaGroup::assignToGroup( ml.get(), *m3 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->nbVideo() );
    ASSERT_EQ( groups[0]->name(), "otter" );

    res = MediaGroup::assignToGroup( ml.get(), *m2 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 1u, groups[1]->nbVideo() );
    ASSERT_EQ( groups[1]->name(), "otters are cute.mkv" );

    res = MediaGroup::assignToGroup( ml.get(), *m1 );
    ASSERT_TRUE( res );
    groups = ml->mediaGroups( nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
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
    ASSERT_FALSE( mg->hasBeenRenamed() );

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

TEST_F( MediaGroups, Regroup )
{
    // 2 media we expect to be regrouped
    auto m1 = ml->addMedia( "matching title 1.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "MATCHING TITLE 2.mkv", IMedia::Type::Video );
    // And another one, but with a slightly different name to check we adjust
    // the group name accordingly
    auto m3 = ml->addMedia( "matching trout.mkv", IMedia::Type::Video );
    // A video that won't match the automatic grouping patterns
    auto m4 = ml->addMedia( "fluffy otters is no match for you.avi", IMedia::Type::Video );
    // A video that should match but which will be already grouped
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
