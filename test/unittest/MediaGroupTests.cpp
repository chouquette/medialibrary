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

#include "UnitTests.h"

#include "MediaGroup.h"
#include "Media.h"

static void Create( Tests* T )
{
    auto name = std::string{ "group" };
    auto mg = T->ml->createMediaGroup( name );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "group", mg->name() );
    ASSERT_EQ( 0u, mg->nbPresentVideo() );
    ASSERT_EQ( 0u, mg->nbPresentAudio() );
    ASSERT_EQ( 0u, mg->nbPresentUnknown() );
    ASSERT_EQ( 0u, mg->nbPresentMedia() );
    ASSERT_EQ( 0u, mg->nbVideo() );
    ASSERT_EQ( 0u, mg->nbAudio() );
    ASSERT_EQ( 0u, mg->nbUnknown() );
    ASSERT_EQ( 0u, mg->nbTotalMedia() );
    ASSERT_EQ( true, mg->userInteracted() );

    mg = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( mg->id() ) );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "group", mg->name() );
}

static void ListAll( Tests* T )
{
    auto mg1 = T->ml->createMediaGroup( "weasels group" );
    auto m1 = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto res = mg1->add( *m1 );
    ASSERT_TRUE( res );
    auto mg2 = T->ml->createMediaGroup( "pangolin group" );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    res = mg2->add( *m2 );
    ASSERT_TRUE( res );
    auto mg3 = T->ml->createMediaGroup( "otters group" );
    auto m3 = T->ml->addMedia( "media3.mkv", IMedia::Type::Video );
    res = mg3->add( *m3 );
    ASSERT_TRUE( res );

    auto mgQuery = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr );
    ASSERT_EQ( 3u, mgQuery->count() );
    auto groups = mgQuery->all();
    ASSERT_EQ( 3u, groups.size() );

    // Default sort is alphabetical
    ASSERT_EQ( mg3->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
    ASSERT_EQ( mg1->id(), groups[2]->id() );

    QueryParameters params{ SortingCriteria::Alpha, true };
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();

    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
    ASSERT_EQ( mg3->id(), groups[2]->id() );
}

static void FetchOne( Tests* T )
{
    auto mg = T->ml->createMediaGroup( "group" );
    ASSERT_NE( nullptr, mg );

    auto mg2 = T->ml->mediaGroup( mg->id() );
    ASSERT_NE( nullptr, mg2 );
    ASSERT_EQ( mg->id(), mg2->id() );
}

static void Search( Tests* T )
{
    auto mg1 = T->ml->createMediaGroup( "otter group" );
    auto m1 = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto res = mg1->add( *m1 );
    ASSERT_TRUE( res );
    auto mg2 = T->ml->createMediaGroup( "weasels group" );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    res = mg2->add( *m2 );
    ASSERT_TRUE( res );

    auto q = T->ml->searchMediaGroups( "12", nullptr );
    ASSERT_EQ( nullptr, q );

    QueryParameters params { SortingCriteria::Alpha, false };
    q = T->ml->searchMediaGroups( "group", &params );
    ASSERT_NE( nullptr, q );
    ASSERT_EQ( 2u, q->count() );
    auto groups = q->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );

    params.desc = true;
    groups = T->ml->searchMediaGroups( "group", &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    groups = T->ml->searchMediaGroups( "otter", nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
}

static void FetchMedia( Tests* T )
{
    auto mg = T->ml->createMediaGroup( "group" );
    auto video = T->ml->addMedia( "video.mkv", IMedia::Type::Video );
    auto audio = T->ml->addMedia( "audio.mp3", IMedia::Type::Audio );
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
    ASSERT_EQ( 1u, mg->nbPresentMedia() );
    // And check that the DB was updated
    mg = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( mg->id() ) );
    ASSERT_EQ( 1u, mg->nbPresentMedia() );

    // Don't remove more, since the media group would be deleted.
    // This is checked by the DeleteEmpty test
}

static void SearchMedia( Tests* T )
{
    auto mg = T->ml->createMediaGroup( "" );
    auto m1 = T->ml->addMedia( "audio.mp3", IMedia::Type::Audio );
    m1->setTitle( "The sea otters podcast" );
    auto m2 = T->ml->addMedia( "audio2.mp3", IMedia::Type::Audio );
    m2->setTitle( "A boring podcast" );
    auto v1 = T->ml->addMedia( "a cute otter.mkv", IMedia::Type::Video );
    auto v2 = T->ml->addMedia( "a boring animal.mkv", IMedia::Type::Video );
    auto v3 = T->ml->addMedia( "more fluffy otters.mkv", IMedia::Type::Video );

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

static void UpdateNbMediaTypeChange( Tests* T )
{
    auto group1 = T->ml->createMediaGroup( "group" );
    auto group2 = T->ml->createMediaGroup( "group2" );
    ASSERT_NE( nullptr, group1 );
    ASSERT_NE( nullptr, group2 );
    ASSERT_EQ( 0u, group1->nbPresentAudio() );
    ASSERT_EQ( 0u, group1->nbPresentVideo() );
    ASSERT_EQ( 0u, group1->nbPresentUnknown() );
    ASSERT_EQ( 0u, group2->nbPresentAudio() );
    ASSERT_EQ( 0u, group2->nbPresentVideo() );
    ASSERT_EQ( 0u, group2->nbPresentUnknown() );

    auto m = T->ml->addMedia( "media.mkv", IMedia::Type::Unknown );
    auto m2 = T->ml->addMedia( "media2.avi", IMedia::Type::Video );
    auto m3 = T->ml->addMedia( "media3.mp3", IMedia::Type::Audio );
    group1->add( *m );
    group1->add( *m2 );
    group2->add( *m3 );

    group1 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbPresentAudio() );
    ASSERT_EQ( 1u, group1->nbPresentVideo() );
    ASSERT_EQ( 1u, group1->nbPresentUnknown() );
    ASSERT_EQ( 2u, group1->nbPresentMedia() );
    ASSERT_EQ( 1u, group2->nbPresentAudio() );
    ASSERT_EQ( 0u, group2->nbPresentVideo() );
    ASSERT_EQ( 0u, group2->nbPresentUnknown() );
    ASSERT_EQ( 1u, group2->nbPresentMedia() );

    // Move that media to another group
    group2->add( *m );

    group1 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbPresentAudio() );
    ASSERT_EQ( 1u, group1->nbPresentVideo() );
    ASSERT_EQ( 0u, group1->nbPresentUnknown() );
    ASSERT_EQ( 1u, group1->nbPresentMedia() );
    ASSERT_EQ( 1u, group2->nbPresentAudio() );
    ASSERT_EQ( 0u, group2->nbPresentVideo() );
    ASSERT_EQ( 1u, group2->nbPresentUnknown() );
    ASSERT_EQ( 2u, group2->nbPresentMedia() );

    // Now change the media type
    m->setType( IMedia::Type::Audio );
    group1 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbPresentAudio() );
    ASSERT_EQ( 1u, group1->nbPresentVideo() );
    ASSERT_EQ( 0u, group1->nbPresentUnknown() );
    ASSERT_EQ( 2u, group2->nbPresentAudio() );
    ASSERT_EQ( 0u, group2->nbPresentVideo() );
    ASSERT_EQ( 0u, group2->nbPresentUnknown() );

    // Manually change both group & type to check if we properly support it
    std::string req = "UPDATE " + Media::Table::Name +
            " SET type = ?, group_id = ? WHERE id_media = ?";
    auto res = sqlite::Tools::executeUpdate( T->ml->getConn(), req, IMedia::Type::Video,
                                             group1->id(), m->id() );
    ASSERT_TRUE( res );

    group1 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbPresentAudio() );
    ASSERT_EQ( 2u, group1->nbPresentVideo() );
    ASSERT_EQ( 0u, group1->nbPresentUnknown() );
    ASSERT_EQ( 1u, group2->nbPresentAudio() );
    ASSERT_EQ( 0u, group2->nbPresentVideo() );
    ASSERT_EQ( 0u, group2->nbPresentUnknown() );

    // Now remove the media from the group:
    group1->remove( m->id() );
    group1 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group1->id() ) );
    group2 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group2->id() ) );
    ASSERT_EQ( 0u, group1->nbPresentAudio() );
    ASSERT_EQ( 1u, group1->nbPresentVideo() );
    ASSERT_EQ( 0u, group1->nbPresentUnknown() );
    ASSERT_EQ( 1u, group2->nbPresentAudio() );
    ASSERT_EQ( 0u, group2->nbPresentVideo() );
    ASSERT_EQ( 0u, group2->nbPresentUnknown() );
}

static void UpdateNbMediaNoDelete( Tests* T )
{
    auto group1 = T->ml->createMediaGroup( "group" );
    ASSERT_NE( nullptr, group1 );
    ASSERT_EQ( 0u, group1->nbPresentAudio() );
    ASSERT_EQ( 0u, group1->nbPresentVideo() );
    ASSERT_EQ( 0u, group1->nbPresentUnknown() );
    ASSERT_EQ( 0u, group1->nbPresentMedia() );

    auto m = T->ml->addMedia( "media.mkv", IMedia::Type::Unknown );
    group1->add( *m );

    ASSERT_EQ( 0u, group1->nbPresentAudio() );
    ASSERT_EQ( 0u, group1->nbPresentVideo() );
    ASSERT_EQ( 1u, group1->nbPresentUnknown() );
    ASSERT_EQ( 1u, group1->nbPresentMedia() );

    // Now change the media type
    m->setType( IMedia::Type::Audio );

    group1 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( group1->id() ) );
    ASSERT_EQ( 1u, group1->nbPresentAudio() );
    ASSERT_EQ( 0u, group1->nbPresentVideo() );
    ASSERT_EQ( 0u, group1->nbPresentUnknown() );
    ASSERT_EQ( 1u, group1->nbPresentMedia() );
}

static void SortByNbMedia( Tests* T )
{
    auto mg1 = T->ml->createMediaGroup( "A group" );
    auto mg2 = T->ml->createMediaGroup( "Z group" );

    auto v1 = T->ml->addMedia( "media1.mkv", IMedia::Type::Video );
    auto v2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    mg1->add( *v1 );
    mg1->add( *v2 );

    auto a1 = T->ml->addMedia( "audio1.mp3", IMedia::Type::Audio );
    auto u1 = T->ml->addMedia( "unknown1.ts", IMedia::Type::Unknown );
    auto u2 = T->ml->addMedia( "unknown2.ts", IMedia::Type::Unknown );
    mg2->add( *a1 );
    mg2->add( *u1 );
    mg2->add( *u2 );

    QueryParameters params{ SortingCriteria::NbVideo, false };

    auto query = T->ml->mediaGroups( IMedia::Type::Unknown, &params );
    ASSERT_EQ( 2u, query->count() );
    auto groups = query->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = true;
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );

    params.sort = SortingCriteria::NbMedia;
    // still descending order, so mg2 comes first
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = false;
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
}

static void FetchFromMedia( Tests* T )
{
    auto mg = T->ml->createMediaGroup( "group" );
    auto m = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
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

static void Rename( Tests* T )
{
    auto m = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    ASSERT_NE( nullptr, m );
    auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
    ASSERT_TRUE( mg->userInteracted() );
    ASSERT_NE( nullptr, mg );

    auto groupMedia = mg->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groupMedia.size() );

    std::string newName{ "better name" };
    auto res = mg->rename( newName );
    ASSERT_TRUE( res );
    ASSERT_TRUE( mg->userInteracted() );
    ASSERT_EQ( newName, mg->name() );

    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( newName, mg->name() );
    ASSERT_TRUE( mg->userInteracted() );

    groupMedia = mg->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groupMedia.size() );

    res = mg->rename( "" );
    ASSERT_FALSE( res );
}

static void CheckDbModel( Tests* T )
{
    auto res = MediaGroup::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void Delete( Tests* T )
{
    auto mg = T->ml->createMediaGroup( "group" );
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "sea otters.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "fluffy otters.mkv", IMedia::Type::Video ) );
    mg->add( m1->id() );
    mg->add( m2->id() );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    m1 = T->ml->media( m1->id() );
    ASSERT_NE( nullptr, m1->group() );
    ASSERT_EQ( mg->id(), m1->groupId() );

    T->ml->deleteMediaGroup( mg->id() );

    m1 = T->ml->media( m1->id() );
    m2 = T->ml->media( m2->id() );
    ASSERT_NE( nullptr, m1->group() );
    auto lockedGroup = std::static_pointer_cast<MediaGroup>( m1->group() );
    ASSERT_TRUE( lockedGroup->isForcedSingleton() );
    ASSERT_EQ( m1->title(), lockedGroup->name() );

    lockedGroup = std::static_pointer_cast<MediaGroup>( m2->group() );
    ASSERT_TRUE( lockedGroup->isForcedSingleton() );
    ASSERT_EQ( m2->title(), lockedGroup->name() );

    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( m2->groupId(), groups[0]->id() );
    ASSERT_EQ( m1->groupId(), groups[1]->id() );

    /* Ensure we properly handle a non existing group */
    auto res = T->ml->deleteMediaGroup( 123 );
    ASSERT_FALSE( res );
}

static void DeleteMedia( Tests* T )
{
    auto mg = T->ml->createMediaGroup( "group" );
    auto m1 = T->ml->addMedia( "otters.mkv", IMedia::Type::Video );
    auto m2 = T->ml->addMedia( "squeaking otters.mp3", IMedia::Type::Audio );
    auto m3 = T->ml->addMedia( "unknown otters.ts", IMedia::Type::Unknown );
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

    ASSERT_EQ( 3u, mg->nbPresentMedia() );
    ASSERT_EQ( 1u, mg->nbPresentAudio() );
    ASSERT_EQ( 1u, mg->nbPresentVideo() );
    ASSERT_EQ( 1u, mg->nbPresentUnknown() );
    // Ensure the value in DB is correct
    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( 3u, mg->nbPresentMedia() );
    ASSERT_EQ( 1u, mg->nbPresentAudio() );
    ASSERT_EQ( 1u, mg->nbPresentVideo() );
    ASSERT_EQ( 1u, mg->nbPresentUnknown() );

    // Delete media and ensure the group media count is updated
    T->ml->deleteMedia( m1->id() );
    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( 2u, mg->nbPresentMedia() );
    ASSERT_EQ( 0u, mg->nbPresentVideo() );

    T->ml->deleteMedia( m2->id() );
    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( 1u, mg->nbPresentMedia() );
    ASSERT_EQ( 0u, mg->nbPresentAudio() );

    T->ml->deleteMedia( m3->id() );
    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( nullptr, mg );
}


static void DeleteEmpty( Tests* T )
{
    /*
     * Create 3 groups with the 3 different media type, and check that deleting
     * every media is causing the group to be deleted as well
     */
    auto m1 = T->ml->addMedia( "media1.mkv", IMedia::Type::Video );
    auto m2 = T->ml->addMedia( "media2.mp3", IMedia::Type::Audio );
    auto m3 = T->ml->addMedia( "media3.ts", IMedia::Type::Unknown );

    auto mg1 = T->ml->createMediaGroup( std::vector<int64_t>{ m1->id() } );
    auto mg2 = T->ml->createMediaGroup( std::vector<int64_t>{ m2->id() } );
    auto mg3 = T->ml->createMediaGroup( std::vector<int64_t>{ m3->id() } );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 3u, groups.size() );

    T->ml->deleteMedia( m1->id() );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    mg1 = T->ml->mediaGroup( mg1->id() );
    ASSERT_EQ( nullptr, mg1 );
    mg2 = T->ml->mediaGroup( mg2->id() );
    ASSERT_NE( nullptr, mg2 );
    mg3 = T->ml->mediaGroup( mg3->id() );
    ASSERT_NE( nullptr, mg3 );

    T->ml->deleteMedia( m2->id() );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    mg2 = T->ml->mediaGroup( mg2->id() );
    ASSERT_EQ( nullptr, mg2 );
    mg3 = T->ml->mediaGroup( mg3->id() );
    ASSERT_NE( nullptr, mg3 );

    T->ml->deleteMedia( m3->id() );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );
    mg3 = T->ml->mediaGroup( mg3->id() );
    ASSERT_EQ( nullptr, mg3 );
}

static void CommonPattern( Tests* )
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

    res = MediaGroup::commonPattern( "áßçḋéḟblabla", "áßçḋéḟsomethingelse" );
    ASSERT_EQ( "áßçḋéḟ", res );

    res = MediaGroup::commonPattern( "the áßçḋéḟblabla", "áßçḋéḟsomethingelse" );
    ASSERT_EQ( "áßçḋéḟ", res );

    res = MediaGroup::commonPattern( "áßçḋéḟblabla", "the áßçḋéḟsomethingelse" );
    ASSERT_EQ( "áßçḋéḟ", res );

    res = MediaGroup::commonPattern( "áßç", "áßç" );
    ASSERT_EQ( "", res );

    res = MediaGroup::commonPattern( "áßç\xEC\xB0", "áßç\xEC\xB0" );
    ASSERT_EQ( "", res );

    res = MediaGroup::commonPattern( "áßçḋéḟǵ\xEC", "áßçḋéḟǵ\xEC" );
    ASSERT_EQ( "áßçḋéḟǵ", res );

    res = MediaGroup::commonPattern( "lalalalére", "lalalalère" );
    ASSERT_EQ( "lalalal", res );
}

static void AssignToGroups( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "The otters are fluffy.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "otters are cute.mkv", IMedia::Type::Video ) );
    /*
     * Add a media with a title smaller than the common prefix for groups. It
     * shouldn't be grouped with anything else, and will have its own group
     * Since the title sanitizer doesn't run here, we need to omit the extension
     * for the title to be actually less than 6 chars
     */
    auto m3 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "the otter", IMedia::Type::Video ) );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );

    /*
     * Create a forced singleton group to ensure it's not considered a valid
     * candidate for grouping
     */
    auto m4 = T->ml->addMedia( "otters are so fluffy.mkv", IMedia::Type::Video );
    auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m4->id() } );
    auto res = mg->remove( *m4 );
    ASSERT_TRUE( res );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_TRUE( static_cast<MediaGroup*>( groups[0].get() )->isForcedSingleton() );

    /*
     * First assign m1, then m2, and do it again for a different group to check
     * that the "the " prefix is correctly handled regardless of the insertion
     * order
     */
    res = MediaGroup::assignToGroup( T->ml.get(), *m1 );
    ASSERT_TRUE( res );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->nbPresentVideo() );
    ASSERT_EQ( groups[0]->name(), "otters are fluffy.mkv" );
    ASSERT_EQ( m4->groupId(), groups[1]->id() );
    ASSERT_TRUE( static_cast<MediaGroup*>( groups[1].get() )->isForcedSingleton() );
    ASSERT_EQ( m4->title(), groups[1]->name() );

    res = MediaGroup::assignToGroup( T->ml.get(), *m2 );
    ASSERT_TRUE( res );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 2u, groups[0]->nbPresentVideo() );
    ASSERT_EQ( groups[0]->name(), "otters are " );

    res = MediaGroup::assignToGroup( T->ml.get(), *m3 );
    ASSERT_TRUE( res );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->nbPresentVideo() );
    ASSERT_EQ( groups[0]->name(), "otter" );
    ASSERT_EQ( 2u, groups[1]->nbPresentVideo() );
    ASSERT_EQ( groups[1]->name(), "otters are " );

    /*
     * Delete the media since they have been grouped, and assignToGroup asserts
     * that the media have never been grouped, which is a valid assertion as
     * grouping comes from the metadataparser, which only groups media when they
     * were never grouped
     */
    T->ml->deleteMedia( m1->id() );
    T->ml->deleteMedia( m2->id() );
    T->ml->deleteMedia( m3->id() );
    T->ml->deleteMedia( m4->id() );

    /* Which should delete all the groups */
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );

    m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "The otters are fluffy.mkv", IMedia::Type::Video ) );
    m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "otters are cute.mkv", IMedia::Type::Video ) );
    m3 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "the otter", IMedia::Type::Video ) );

    m4 = T->ml->addMedia( "otters are so fluffy.mkv", IMedia::Type::Video );
    mg = T->ml->createMediaGroup( std::vector<int64_t>{ m4->id() } );
    res = mg->remove( *m4 );

    /* Now try again with the other ordering */
    res = MediaGroup::assignToGroup( T->ml.get(), *m3 );
    ASSERT_TRUE( res );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( 1u, groups[0]->nbPresentVideo() );
    ASSERT_EQ( groups[0]->name(), "otter" );

    res = MediaGroup::assignToGroup( T->ml.get(), *m2 );
    ASSERT_TRUE( res );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( 1u, groups[1]->nbPresentVideo() );
    ASSERT_EQ( groups[1]->name(), "otters are cute.mkv" );

    res = MediaGroup::assignToGroup( T->ml.get(), *m1 );
    ASSERT_TRUE( res );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 3u, groups.size() );
    ASSERT_EQ( 2u, groups[1]->nbPresentVideo() );
    ASSERT_EQ( groups[1]->name(), "otters are " );
}

static void CreateFromMedia( Tests* T )
{
    auto m1 = T->ml->addMedia( "media1.mkv", IMedia::Type::Video );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    auto m3 = T->ml->addMedia( "media3.mkv", IMedia::Type::Video );

    auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m1->id(), m2->id() } );
    ASSERT_NE( nullptr, mg );
    ASSERT_TRUE( mg->userInteracted() );

    ASSERT_EQ( 2u, mg->nbPresentVideo() );
    ASSERT_EQ( 0u, mg->nbPresentAudio() );
    ASSERT_EQ( 2u, mg->nbPresentMedia() );

    auto mediaQuery = mg->media( IMedia::Type::Video, nullptr );
    ASSERT_EQ( 2u, mediaQuery->count() );
    auto media = mediaQuery->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );

    auto mg2 = T->ml->createMediaGroup( std::vector<int64_t>{ m3->id(), m2->id() } );
    ASSERT_NE( nullptr, mg2 );

    ASSERT_EQ( 2u, mg2->nbPresentVideo() );
    ASSERT_EQ( 0u, mg2->nbPresentAudio() );
    ASSERT_EQ( 2u, mg2->nbPresentMedia() );

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

static void CreateWithUnknownMedia( Tests* T )
{
    auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ 7, 8, 9, 10 } );
    ASSERT_EQ( nullptr, mg );

    auto m = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    ASSERT_NE( nullptr, m );
    mg = T->ml->createMediaGroup( std::vector<int64_t>{ m->id(), 13, 12 } );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( 1u, mg->nbPresentMedia() );
}

static void CheckFromMediaName( Tests* T )
{
    /*
     * First create a group with an existing match, then create a group with
     * no match. Ensure the first group has a name, and the 2nd doesn't
     */
    auto m1 = T->ml->addMedia( "otters are fluffy.mkv", IMedia::Type::Video );
    auto m2 = T->ml->addMedia( "otters are cute.mkv", IMedia::Type::Video );
    auto m3 = T->ml->addMedia( "pangolins don't match otters.mkv", IMedia::Type::Video );

    auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m1->id(), m2->id() } );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "otters are ", mg->name() );
    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( "otters are ", mg->name() );

    mg = T->ml->createMediaGroup( std::vector<int64_t>{ m3->id(), m2->id() } );
    ASSERT_NE( nullptr, mg );
    ASSERT_EQ( "", mg->name() );
    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( "", mg->name() );
}

static void RemoveMedia( Tests* T )
{
    /*
     * Ensure that when a media is removed from a group, an automatic locked
     * group gets created to contain that media
     */
    auto m = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto mg = std::static_pointer_cast<MediaGroup>(
                T->ml->createMediaGroup( std::vector<int64_t>{ m->id() } ) );
    ASSERT_NE( nullptr, mg );
    ASSERT_FALSE( mg->isForcedSingleton() );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    // Refresh the media since it needs to know it's part of a group
    m = T->ml->media( m->id() );

    auto res = m->removeFromGroup();
    ASSERT_TRUE( res );

    /* The previous group will be removed, but a new one should be created */
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    ASSERT_NE( groups[0]->id(), mg->id() );
    auto lockedGroup = std::static_pointer_cast<MediaGroup>( groups[0] );
    ASSERT_TRUE( lockedGroup->isForcedSingleton() );
    ASSERT_EQ( lockedGroup->name(), m->title() );
    ASSERT_EQ( 1u, lockedGroup->nbPresentVideo() );
    ASSERT_EQ( 0u, lockedGroup->nbPresentAudio() );
    ASSERT_EQ( 0u, lockedGroup->nbPresentUnknown() );

    T->ml->deleteMedia( m->id() );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );

    /* Now try again with the other way of removing media from a group */

    m = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    mg = std::static_pointer_cast<MediaGroup>(
                T->ml->createMediaGroup( std::vector<int64_t>{ m->id() } ) );
    ASSERT_NE( nullptr, mg );
    ASSERT_FALSE( mg->isForcedSingleton() );

    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    res = mg->remove( m->id() );
    ASSERT_TRUE( res );

    /* The previous group will be removed, but a new one should be created */
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    ASSERT_NE( groups[0]->id(), mg->id() );
    lockedGroup = std::static_pointer_cast<MediaGroup>( groups[0] );
    ASSERT_TRUE( lockedGroup->isForcedSingleton() );
    ASSERT_EQ( lockedGroup->name(), m->title() );
    ASSERT_EQ( 1u, lockedGroup->nbPresentVideo() );
    ASSERT_EQ( 0u, lockedGroup->nbPresentAudio() );
    ASSERT_EQ( 0u, lockedGroup->nbPresentUnknown() );

    T->ml->deleteMedia( m->id() );
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 0u, groups.size() );
}

static void RegroupLocked( Tests* T )
{
    /*
     * We only regroup media that are part of a locked group, so we need a bit
     * of setup to create the associated locked groups. We will add each media
     * to a group, and remove those from there immediatly after
     */
    auto createLockedGroup = [T]( MediaPtr m ) {
        auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
        auto res = mg->remove( *m );
        ASSERT_TRUE( res );
    };
    // 2 media we expect to be regrouped
    auto m1 = T->ml->addMedia( "matching title 1.mkv", IMedia::Type::Video );
    createLockedGroup( m1 );
    auto m2 = T->ml->addMedia( "MATCHING TITLE 2.mkv", IMedia::Type::Video );
    createLockedGroup( m2 );
    // And another one, but with a slightly different name to check we adjust
    // the group name accordingly
    auto m3 = T->ml->addMedia( "matching trout.mkv", IMedia::Type::Video );
    createLockedGroup( m3 );
    // A video that won't match the automatic grouping patterns
    auto m4 = T->ml->addMedia( "fluffy otters is no match for you.avi", IMedia::Type::Video );
    createLockedGroup( m4 );
    // A video that should match but which will is not part of a locked group
    auto m5 = T->ml->addMedia( "matching title 3.mkv", IMedia::Type::Video );

    auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m5->id() } );
    m5 = T->ml->media( m5->id() );
    ASSERT_EQ( mg->id(), m5->groupId() );

    auto res = m1->regroup();
    ASSERT_TRUE( res );

    m2 = T->ml->media( m2->id() );
    ASSERT_EQ( m1->groupId(), m2->groupId() );

    m3 = T->ml->media( m3->id() );
    ASSERT_EQ( m1->groupId(), m3->groupId() );

    m4 = T->ml->media( m4->id() );

    m5 = T->ml->media( m5->id() );
    ASSERT_NE( m5->groupId(), m1->groupId() );

    auto newGroup = m1->group();
    ASSERT_EQ( 3u, newGroup->nbPresentVideo() );
    ASSERT_EQ( "matching t", newGroup->name() );

    // Ensure we refuse to regroup an already grouped media
    res = m5->regroup();
    ASSERT_FALSE( res );
}

static void ForcedSingletonRestrictions( Tests* T )
{
    auto m = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
    auto res = mg->remove( *m );
    ASSERT_TRUE( res );
    mg = m->group();

    res = mg->rename( "Another name" );
    ASSERT_FALSE( res );
    ASSERT_EQ( "media.mkv", mg->name() );

    res = mg->destroy();
    ASSERT_FALSE( res );
    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( groups[0]->id(), mg->id() );
}

static void AddToForcedSingleton( Tests* T )
{
    auto createForcedSingleton = [T]( MediaPtr m ) {
        auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
        auto res = mg->remove( *m );
        ASSERT_TRUE( res );
    };
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    createForcedSingleton( m1 );
    createForcedSingleton( m2 );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
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
    ASSERT_EQ( 2u, g1->nbPresentMedia() );

    g1 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( g1->id() ) );
    ASSERT_FALSE( g1->isForcedSingleton() );
    ASSERT_EQ( 2u, g1->nbPresentMedia() );

    g2 = std::static_pointer_cast<MediaGroup>( T->ml->mediaGroup( g2->id() ) );
    ASSERT_EQ( nullptr, g2 );
}

static void RenameForcedSingleton( Tests* T )
{
    auto m = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
    mg->remove( *m );
    auto singleton = m->group();
    ASSERT_NE( mg->id(), singleton->id() );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( singleton->id(), groups[0]->id() );
    ASSERT_EQ( m->title(), groups[0]->name() );

    auto res = m->setTitle( "sea otter makes a better title" );
    ASSERT_TRUE( res );

    singleton = T->ml->mediaGroup( singleton->id() );
    ASSERT_EQ( m->title(), singleton->name() );

    /* Ensure we don't do anything for non singleton groups */
    mg = T->ml->createMediaGroup( "otters group" );
    res = mg->add( *m );
    ASSERT_TRUE( res );

    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    res = m->setTitle( "Better otter related title" );
    ASSERT_TRUE( res );

    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_NE( mg->name(), m->title() );
}

static void UpdateDuration( Tests* T )
{
    auto mg = T->ml->createMediaGroup( "test group" );
    ASSERT_EQ( 0, mg->duration() );

    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );

    /* Insert a media with an already known duration */
    m1->setDuration( 123 );
    m1->save();
    auto res = mg->add( *m1 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 123, mg->duration() );

    /* Check the result in DB as well */
    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( 123, mg->duration() );

    /*
     * Now add a media with its default duration to the group, and only then
     * update the media duration
     */
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    res = mg->add( *m2 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 123, mg->duration() );

    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( 123, mg->duration() );

    m2->setDuration( 999 );
    m2->save();

    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( 123 + 999, mg->duration() );

    /* Now remove the first media and check that the duration is updated accordingly */
    res = mg->remove( *m1 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 999, mg->duration() );

    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( 999, mg->duration() );

    /* Ensure the new singleton group has the proper duration set */
    m1 = T->ml->media( m1->id() );
    auto singleton = m1->group();
    ASSERT_NE( nullptr, singleton );
    ASSERT_NE( singleton->id(), mg->id() );
    ASSERT_EQ( m1->duration(), singleton->duration() );

    /*
     * Now add a media with a default duration and remove it, check that
     * nothing changes
     */
    auto m3 = T->ml->addMedia( "media3.mkv", IMedia::Type::Video );
    res = mg->add( *m3 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 999, mg->duration() );

    mg = T->ml->mediaGroup( mg->id() );
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
    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( nullptr, mg );
}

static void OrderByDuration( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media.mkv", IMedia::Type::Video ) );
    m1->setDuration( 999 );
    m1->save();
    auto mg1 = T->ml->createMediaGroup( std::vector<int64_t>{ m1->id() } );
    mg1->rename( "a" );

    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    m2->setDuration( 111 );
    m2->save();
    auto mg2 = T->ml->createMediaGroup( std::vector<int64_t>{ m2->id() } );
    mg2->rename( "z" );

    QueryParameters params{ SortingCriteria::Default, false };
    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( m1->duration(), groups[0]->duration() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
    ASSERT_EQ( m2->duration(), groups[1]->duration() );

    params.sort = SortingCriteria::Duration;
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = true;
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
}

static void CreationDate( Tests* T )
{
    /*
     * Check that the creation date makes sense.
     */
    auto now = time( nullptr );
    auto mg = T->ml->createMediaGroup( "group" );
    ASSERT_NE( nullptr, mg );
    /*
     * We can't check for equality since we might create get the current time on
     * the edge of a second, and do the insertion on the other edge
     */
    auto diff = mg->creationDate() - now;
    ASSERT_LE( diff, 1 );

    auto m1 = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    now = time( nullptr );
    mg = T->ml->createMediaGroup( std::vector<int64_t>{ m1->id() } );
    ASSERT_NE( nullptr, mg );
    diff = mg->creationDate() - now;
    ASSERT_LE( diff, 1 );
}

static void OrderByCreationDate( Tests* T )
{
    auto forceCreationDate = [T]( int64_t groupId, time_t t ) {
        const std::string req = "UPDATE " + MediaGroup::Table::Name +
                " SET creation_date = ? WHERE id_group = ?";
        return sqlite::Tools::executeUpdate( T->ml->getConn(), req, t, groupId );
    };
    auto mg1 = T->ml->createMediaGroup( "a" );
    auto m1 = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto res = mg1->add( *m1 );
    ASSERT_TRUE( res );
    forceCreationDate( mg1->id(), 999 );
    auto mg2 = T->ml->createMediaGroup( "z" );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    res = mg2->add( *m2 );
    ASSERT_TRUE( res );
    forceCreationDate( mg2->id(), 111 );

    QueryParameters params{ SortingCriteria::InsertionDate, false };
    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = true;
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
}

static void LastModificationDate( Tests* T )
{
    auto resetModificationDate = [T]( int64_t groupId ) {
        const std::string req = "UPDATE " + MediaGroup::Table::Name +
                " SET last_modification_date = 0 WHERE id_group = ?";
        return sqlite::Tools::executeUpdate( T->ml->getConn(), req, groupId );
    };
    auto mg = T->ml->createMediaGroup( "group" );
    ASSERT_EQ( mg->creationDate(), mg->lastModificationDate() );
    resetModificationDate( mg->id() );

    /* Ensure we update the modification date on insertion */
    auto m1 = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    mg->add( *m1 );

    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_NE( 0, mg->lastModificationDate() );

    /* Ensure we update the modification date on renaming */
    resetModificationDate( mg->id() );
    auto res = mg->rename( "new name" );
    ASSERT_TRUE( res );
    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_NE( 0, mg->lastModificationDate() );

    /*
     * Ensure we update the modification date on removal.
     * We need to add a 2nd media beforehand, otherwise the group will be deleted
     */
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    mg->add( *m2 );
    resetModificationDate( mg->id() );
    mg->remove( *m2 );

    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_NE( 0, mg->lastModificationDate() );

    /* Ensure we update the modification date on media deletion */
    m2 = T->ml->addMedia( "media3.mkv", IMedia::Type::Video );
    mg->add( *m2 );
    resetModificationDate( mg->id() );
    T->ml->deleteMedia( m2->id() );

    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_NE( 0, mg->lastModificationDate() );
}

static void OrderByLastModificationDate( Tests* T )
{
    auto forceLastModificationDate = [T]( int64_t groupId, time_t t ) {
        const std::string req = "UPDATE " + MediaGroup::Table::Name +
                " SET last_modification_date = ? WHERE id_group = ?";
        return sqlite::Tools::executeUpdate( T->ml->getConn(), req, t, groupId );
    };
    auto mg1 = T->ml->createMediaGroup( "a" );
    auto m1 = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto res = mg1->add( *m1 );
    ASSERT_TRUE( res );
    forceLastModificationDate( mg1->id(), 999 );
    auto mg2 = T->ml->createMediaGroup( "z" );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    res = mg2->add( *m2 );
    ASSERT_TRUE( res );
    forceLastModificationDate( mg2->id(), 111 );

    QueryParameters params{ SortingCriteria::LastModificationDate, false };
    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg2->id(), groups[0]->id() );
    ASSERT_EQ( mg1->id(), groups[1]->id() );

    params.desc = true;
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, groups.size() );
    ASSERT_EQ( mg1->id(), groups[0]->id() );
    ASSERT_EQ( mg2->id(), groups[1]->id() );
}

static void Destroy( Tests* T )
{
    /* Ensures a deleted media group exposes a valid state */
    auto mg = T->ml->createMediaGroup( "group" );
    auto m1 = T->ml->addMedia( "media1.mkv", IMedia::Type::Video );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    mg->add( *m1 );
    mg->add( *m2 );

    ASSERT_EQ( 2u, mg->nbPresentMedia() );

    mg = T->ml->mediaGroup( mg->id() );

    auto res = mg->destroy();
    ASSERT_TRUE( res );
    ASSERT_EQ( 0u, mg->nbPresentMedia() );

    mg = T->ml->mediaGroup( mg->id() );
    ASSERT_EQ( nullptr, mg );
}

static void RegroupAll( Tests* T )
{
    auto createLockedGroup = [T]( MediaPtr m ) {
        auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m->id() } );
        auto res = mg->remove( *m );
        ASSERT_TRUE( res );
    };
    auto m1 = T->ml->addMedia( "otters are not grouped.mkv", IMedia::Type::Video );
    auto m2 = T->ml->addMedia( "pangolins are cute.mkv", IMedia::Type::Video );
    auto m3 = T->ml->addMedia( "otters are not responsible for COVID19.mkv", IMedia::Type::Video );
    auto m4 = T->ml->addMedia( "pangolins are vectors of diseases.mkv", IMedia::Type::Video );
    auto m5 = T->ml->addMedia( "cats are unrelated to otters and pangolins.mkv", IMedia::Type::Video );
    auto m6 = T->ml->addMedia( "otters is already grouped.mkv", IMedia::Type::Video );

    createLockedGroup( m1 );
    createLockedGroup( m2 );
    createLockedGroup( m3 );
    createLockedGroup( m4 );
    createLockedGroup( m5 );
    auto mg = T->ml->createMediaGroup( std::vector<int64_t>{ m6->id() } );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    /* We expect 1 group per media */
    ASSERT_EQ( 6u, groups.size() );

    /* Now regroup all the things!, we expect 4 groups as a result:
     * - otters
     * - pangolins
     * - cats
     * - otters (but the manually grouped one)
     */
    T->ml->regroupAll();
    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 4u, groups.size() );
    ASSERT_EQ( groups[0]->name(), "cats are unrelated to otters and pangolins.mkv" );
    ASSERT_EQ( groups[1]->name(), "otters are not " );
    ASSERT_EQ( groups[2]->name(), "otters is already grouped.mkv" );
    ASSERT_EQ( groups[3]->name(), "pangolins are " );
}

static void MergeAutoCreated( Tests* T )
{
    /*
     * Checks that we can appen a media from an automatically created group
     * into another automatically created group
     */
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    auto res = MediaGroup::assignToGroup( T->ml.get(), *m1 );
    ASSERT_TRUE( res );
    res = MediaGroup::assignToGroup( T->ml.get(), *m2 );
    ASSERT_TRUE( res );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 2u, groups.size() );
    auto group1 = groups[0];
    auto group2 = groups[1];
    ASSERT_EQ( 1u, group1->nbPresentMedia() );
    ASSERT_EQ( 1u, group2->nbPresentMedia() );

    auto group2Media = group2->media( IMedia::Type::Unknown )->all();
    ASSERT_EQ( 1u, group2Media.size() );
    res = group1->add( *group2Media[0] );
    ASSERT_TRUE( res );

    groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );

    ASSERT_EQ( 2u, group1->nbPresentMedia() );

    group1 = T->ml->mediaGroup( group1->id() );
    ASSERT_EQ( 2u, group1->nbPresentMedia() );
}

static void KoreanTitles( Tests* T )
{
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "[NEO지식창고] 03월01일 TEST1.mp4", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "[NEO지식창고] 03월01일 TEST2.mp4", IMedia::Type::Video ) );
    auto m3 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "[NEO지식창고] 03월01일 TEST3.mp4", IMedia::Type::Video ) );
    auto m4 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "[NEO지식창고] 03월01일 TEST4.mp4", IMedia::Type::Video ) );
    auto m5 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "[NEO지식창고] 03월01일 TEST5.mp4", IMedia::Type::Video ) );

    auto res = MediaGroup::assignToGroup( T->ml.get(), *m1 );
    ASSERT_TRUE( res );
    res = MediaGroup::assignToGroup( T->ml.get(), *m2 );
    ASSERT_TRUE( res );
    res = MediaGroup::assignToGroup( T->ml.get(), *m3 );
    ASSERT_TRUE( res );
    res = MediaGroup::assignToGroup( T->ml.get(), *m4 );
    ASSERT_TRUE( res );
    res = MediaGroup::assignToGroup( T->ml.get(), *m5 );
    ASSERT_TRUE( res );

    auto groups = T->ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, groups.size() );
    ASSERT_EQ( "[NEO지식창고] 03월01일 TEST", groups[0]->name() );
}

int main( int ac, char** av )
{
    INIT_TESTS( MediaGroup );

    ADD_TEST( Create );
    ADD_TEST( ListAll );
    ADD_TEST( FetchOne );
    ADD_TEST( Search );
    ADD_TEST( FetchMedia );
    ADD_TEST( SearchMedia );
    ADD_TEST( UpdateNbMediaTypeChange );
    ADD_TEST( UpdateNbMediaNoDelete );
    ADD_TEST( SortByNbMedia );
    ADD_TEST( FetchFromMedia );
    ADD_TEST( Rename );
    ADD_TEST( CheckDbModel );
    ADD_TEST( Delete );
    ADD_TEST( DeleteMedia );
    ADD_TEST( DeleteEmpty );
    ADD_TEST( CommonPattern );
    ADD_TEST( AssignToGroups );
    ADD_TEST( CreateFromMedia );
    ADD_TEST( CreateWithUnknownMedia );
    ADD_TEST( CheckFromMediaName );
    ADD_TEST( RemoveMedia );
    ADD_TEST( RegroupLocked );
    ADD_TEST( ForcedSingletonRestrictions );
    ADD_TEST( AddToForcedSingleton );
    ADD_TEST( RenameForcedSingleton );
    ADD_TEST( UpdateDuration );
    ADD_TEST( OrderByDuration );
    ADD_TEST( CreationDate );
    ADD_TEST( OrderByCreationDate );
    ADD_TEST( LastModificationDate );
    ADD_TEST( OrderByLastModificationDate );
    ADD_TEST( Destroy );
    ADD_TEST( RegroupAll );
    ADD_TEST( MergeAutoCreated );
    ADD_TEST( KoreanTitles );

    END_TESTS;
}
