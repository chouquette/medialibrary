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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "Tests.h"

#include "Playlist.h"
#include "Media.h"

class Playlists : public Tests
{
protected:
    std::shared_ptr<Playlist> pl;
    std::shared_ptr<Media> m;

    virtual void SetUp() override
    {
        Tests::SetUp();
        pl = std::static_pointer_cast<Playlist>( ml->createPlaylist( "test playlist" ) );
    }
};

TEST_F( Playlists, Create )
{
    ASSERT_NE( nullptr, pl );
    ASSERT_NE( 0u, pl->id() );
    ASSERT_EQ( "test playlist", pl->name() );
    ASSERT_NE( 0u, pl->creationDate() );
}

TEST_F( Playlists, CreateDuplicate )
{
    auto p = ml->createPlaylist(pl->name());
    ASSERT_NE( nullptr, p );
    ASSERT_NE( p->id(), pl->id() );
}

TEST_F( Playlists, Fetch )
{
    auto pl2 = ml->playlist( pl->id() );
    ASSERT_NE( nullptr, pl2 );
    ASSERT_EQ( pl->id(), pl2->id() );

    auto playlists = ml->playlists( nullptr )->all();
    ASSERT_EQ( 1u, playlists.size() );
    ASSERT_EQ( pl->id(), playlists[0]->id() );
}


TEST_F( Playlists, DeletePlaylist )
{
    auto res = ml->deletePlaylist( pl->id() );
    ASSERT_TRUE( res );
    auto playlists = ml->playlists( nullptr )->all();
    ASSERT_EQ( 0u, playlists.size() );
}

TEST_F( Playlists, SetName )
{
    ASSERT_EQ( "test playlist", pl->name() );
    auto newName = "new name";
    auto res = pl->setName( newName );
    ASSERT_TRUE( res );
    ASSERT_EQ( newName, pl->name() );

    Reload();

    pl = ml->playlist( pl->id() );
    ASSERT_EQ( newName, pl->name() );
}

TEST_F( Playlists, FetchAll )
{
    pl->setName( "pl 1" );
    ml->createPlaylist( "pl 2" );
    ml->createPlaylist( "pl 3" );
    ml->createPlaylist( "pl 4" );

    auto playlists = ml->playlists( nullptr )->all();
    ASSERT_EQ( 4u, playlists.size() );
    for ( auto& p : playlists )
    {
        auto name = std::string{ "pl " } + std::to_string( p->id() );
        ASSERT_EQ( name, p->name() );
    }
}

TEST_F( Playlists, Add )
{
    auto m = ml->addMedia( "file.mkv" );
    auto res = pl->append( *m );
    ASSERT_TRUE( res );
    auto media = pl->media()->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m->id(), media[0]->id() );
}

TEST_F( Playlists, Append )
{
    for ( auto i = 0; i < 5; ++i )
    {
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv" );
        ASSERT_NE( nullptr, m );
        pl->append( *m );
    }
    auto media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );
    for ( auto i = 0u; i < media.size(); ++i )
    {
        auto name = "media" + std::to_string( i ) + ".mkv";
        ASSERT_EQ( media[i]->title(), name );
    }
}

TEST_F( Playlists, Insert )
{
    for ( auto i = 1; i < 4; ++i )
    {
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv" );
        ASSERT_NE( nullptr, m );
        auto res = pl->append( *m );
        ASSERT_TRUE( res );
    }
    // [<1,1>,<2,2>,<3,3>]
    auto firstMedia = ml->addMedia( "first.mkv" );

    pl->add( *firstMedia, 1 );
    // [<4,1>,<1,2>,<2,3>,<3,4>]
    auto middleMedia = ml->addMedia( "middle.mkv" );
    pl->add( *middleMedia, 3 );
    // [<4,1>,<1,2>,<5,3>,<2,4>,<3,5>]
    auto media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 4u, media[0]->id() );
    ASSERT_EQ( 1u, media[1]->id() );
    ASSERT_EQ( 5u, media[2]->id() );
    ASSERT_EQ( 2u, media[3]->id() );
    ASSERT_EQ( 3u, media[4]->id() );
}

TEST_F( Playlists, Move )
{
    for ( auto i = 1; i < 6; ++i )
    {
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv" );
        ASSERT_NE( nullptr, m );
        auto res = pl->append( *m );
        ASSERT_TRUE( res );
    }
    // [<1,1>,<2,2>,<3,3>,<4,4>,<5,5>]
    pl->move( 5, 1 );
    // [<5,1>,<1,2>,<2,3>,<3,4>,<4,5>]
    auto media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 5u, media[0]->id() );
    ASSERT_EQ( 1u, media[1]->id() );
    ASSERT_EQ( 2u, media[2]->id() );
    ASSERT_EQ( 3u, media[3]->id() );
    ASSERT_EQ( 4u, media[4]->id() );
}

TEST_F( Playlists, Remove )
{
    for ( auto i = 1; i < 6; ++i )
    {
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv" );
        ASSERT_NE( nullptr, m );
        auto res = pl->append( *m );
        ASSERT_TRUE( res );
    }
    // [<1,1>,<2,2>,<3,3>,<4,4>,<5,5>]
    auto media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );

    pl->remove( 3 );
    // [<1,1>,<2,2>,<4,4>,<5,5>]

    media = pl->media()->all();
    ASSERT_EQ( 4u, media.size() );

    ASSERT_EQ( 1u, media[0]->id() );
    ASSERT_EQ( 2u, media[1]->id() );
    ASSERT_EQ( 4u, media[2]->id() );
    ASSERT_EQ( 5u, media[3]->id() );
}

TEST_F( Playlists, DeleteFile )
{
    for ( auto i = 1; i < 6; ++i )
    {
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv" );
        ASSERT_NE( nullptr, m );
        auto res = pl->append( *m );
        ASSERT_TRUE( res );
    }
    // [<1,1>,<2,2>,<3,3>,<4,4>,<5,5>]
    auto media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );
    auto m = std::static_pointer_cast<Media>( media[2] );
    auto fs = m->files();
    ASSERT_EQ( 1u, fs.size() );
    m->removeFile( static_cast<File&>( *fs[0] ) );
    // This should trigger the Media removal, which should in
    // turn trigger the playlist item removal
    // So we should now have
    // [<1,1>,<2,2>,<4,4>,<5,5>]

    media = pl->media()->all();
    ASSERT_EQ( 4u, media.size() );

    ASSERT_EQ( 1u, media[0]->id() );
    ASSERT_EQ( 2u, media[1]->id() );
    ASSERT_EQ( 4u, media[2]->id() );
    ASSERT_EQ( 5u, media[3]->id() );

    // Ensure we don't delete an empty playlist:
    auto ms = ml->files();
    for ( auto &mptr : ms )
    {
        auto m = std::static_pointer_cast<Media>( mptr );
        auto fs = m->files();
        ASSERT_EQ( 1u, fs.size() );
        m->removeFile( static_cast<File&>( *fs[0] ) );
    }
    media = pl->media()->all();
    ASSERT_EQ( 0u, media.size() );
    pl = ml->playlist( pl->id() );
    ASSERT_NE( nullptr, pl );
}

TEST_F( Playlists, Search )
{
    ml->createPlaylist( "playlist 2" );
    ml->createPlaylist( "laylist 3" );

    auto playlists = ml->searchPlaylists( "play", nullptr )->all();
    ASSERT_EQ( 2u, playlists.size() );
}

TEST_F( Playlists, SearchAndSort )
{
    auto pl2 = ml->createPlaylist( "playlist 2" );

    auto playlists = ml->searchPlaylists( "play", nullptr )->all();
    ASSERT_EQ( 2u, playlists.size() );
    ASSERT_EQ( pl2->id(), playlists[0]->id() );
    ASSERT_EQ( pl->id(), playlists[1]->id() );

    QueryParameters params = { SortingCriteria::Default, true };
    playlists = ml->searchPlaylists( "play", &params )->all();
    ASSERT_EQ( 2u, playlists.size() );
    ASSERT_EQ( pl->id(), playlists[0]->id() );
    ASSERT_EQ( pl2->id(), playlists[1]->id() );
}

TEST_F( Playlists, SearchAfterDelete )
{
    auto pl = ml->createPlaylist( "sea otters greatest hits" );
    auto pls = ml->searchPlaylists( "sea otters", nullptr )->all();
    ASSERT_EQ( 1u, pls.size() );

    ml->deletePlaylist( pl->id() );

    pls = ml->searchPlaylists( "sea otters", nullptr )->all();
    ASSERT_EQ( 0u, pls.size() );
}

TEST_F( Playlists, SearchAfterUpdate )
{
    auto pl = ml->createPlaylist( "sea otters greatest hits" );
    auto pls = ml->searchPlaylists( "sea otters", nullptr )->all();
    ASSERT_EQ( 1u, pls.size() );

    pl->setName( "pangolins are cool too" );

    pls = ml->searchPlaylists( "sea otters", nullptr )->all();
    ASSERT_EQ( 0u, pls.size() );

    pls = ml->searchPlaylists( "pangolins", nullptr )->all();
    ASSERT_EQ( 1u, pls.size() );
}

TEST_F( Playlists, Sort )
{
    auto pl2 = ml->createPlaylist( "A playlist" );

    auto pls = ml->playlists( nullptr )->all();
    ASSERT_EQ( 2u, pls.size() );
    ASSERT_EQ( pl2->id(), pls[0]->id() );
    ASSERT_EQ( pl->id(), pls[1]->id() );

    QueryParameters params { SortingCriteria::Default, true };
    pls = ml->playlists( &params )->all();
    ASSERT_EQ( 2u, pls.size() );
    ASSERT_EQ( pl2->id(), pls[1]->id() );
    ASSERT_EQ( pl->id(), pls[0]->id() );
}

TEST_F( Playlists, AddDuplicate )
{
    auto m = ml->addMedia( "file.mkv" );
    auto res = pl->append( *m );
    ASSERT_TRUE( res );
    res = pl->append( *m );
    ASSERT_TRUE( res );
    auto media = pl->media()->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m->id(), media[0]->id() );
    ASSERT_EQ( m->id(), media[1]->id() );
}

TEST_F( Playlists, SearchMedia )
{
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "m1.mp3", IMedia::Type::Audio ) );
    m1->setTitleBuffered( "otter" );
    m1->save();

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "m2.mp3", IMedia::Type::Audio ) );
    // Won't match since it's not on the beginning of a word
    m2->setTitleBuffered( "ENOOTTER" );
    m2->save();

    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "m3.mp3", IMedia::Type::Audio ) );
    m3->setTitleBuffered( "otter otter otter" );
    m3->save();

    pl->append( *m1 );
    pl->append( *m2 );

    auto media = ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 2u, media.size() );

    media = pl->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

TEST_F( Playlists, ReinsertMedia )
{
    auto m1 = ml->addMedia( "http://sea.otters/fluffy.mkv" );
    auto m2 = ml->addMedia( "file:///cute_otters_holding_hands.mp4" );
    auto m3 = ml->addMedia( "media.mp3" );
    pl->append( *m1 );
    pl->append( *m2 );
    pl->append( *m3 );

    auto media = pl->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );

    // Ensure the media we fetch are recreated by checking their ids before/after
    auto m1Id = m1->id();
    auto m2Id = m2->id();

    ml->deleteMedia( m1->id() );
    ml->deleteMedia( m2->id() );

    Reload();
    pl = ml->playlist( pl->id() );

    m1 = ml->addMedia( "http://sea.otters/fluffy.mkv" );
    m2 = ml->addMedia( "file:///cute_otters_holding_hands.mp4" );

    media = pl->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    ASSERT_NE( m1->id(), m1Id );
    ASSERT_NE( m2->id(), m2Id );
}

TEST_F( Playlists, RemoveMedia )
{
    auto m1 = ml->addMedia( "http://sea.otters/fluffy.mkv" );
    auto m2 = ml->addMedia( "file:///cute_otters_holding_hands.mp4" );
    auto m3 = ml->addMedia( "media.mp3" );
    pl->append( *m1 );
    pl->append( *m2 );
    pl->append( *m3 );

    auto media = pl->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );

    ml->deleteMedia( m1->id() );
    ml->deleteMedia( m2->id() );

    Reload();

    pl = ml->playlist( pl->id() );

    media = pl->media()->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m3->id(), media[0]->id() );
}
