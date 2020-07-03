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

#include "File.h"
#include "Playlist.h"
#include "Media.h"
#include "parser/Task.h"

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

    void CheckContiguity()
    {
        medialibrary::sqlite::Statement stmt{
            ml->getConn()->handle(),
            "SELECT position FROM PlaylistMediaRelation "
                "WHERE playlist_id=? ORDER BY position"
        };
        stmt.execute( pl->id() );
        auto expected = 0u;
        sqlite::Row row;
        while ( ( row = stmt.row() ) != nullptr )
        {
            uint32_t pos;
            row >> pos;
            ASSERT_EQ( pos, expected );
            ++expected;
        }
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

    pl = std::static_pointer_cast<Playlist>( ml->playlist( pl->id() ) );
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
    auto m = ml->addMedia( "file.mkv", IMedia::Type::Video );
    auto res = pl->append( *m );

    CheckContiguity();

    ASSERT_TRUE( res );
    auto media = pl->media()->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m->id(), media[0]->id() );
}

TEST_F( Playlists, Append )
{
    for ( auto i = 0; i < 5; ++i )
    {
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
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
    CheckContiguity();
}

TEST_F( Playlists, Insert )
{
    for ( auto i = 1; i < 4; ++i )
    {
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
        ASSERT_NE( nullptr, m );
        auto res = pl->append( m->id() );
        ASSERT_TRUE( res );
    }
    // [<1,0>,<2,1>,<3,2>]
    auto firstMedia = ml->addMedia( "first.mkv", IMedia::Type::Video );

    pl->add( *firstMedia, 0 );
    CheckContiguity();

    // [<4,0>,<1,1>,<2,2>,<3,3>]
    auto middleMedia = ml->addMedia( "middle.mkv", IMedia::Type::Video );
    pl->add( *middleMedia, 2 );
    CheckContiguity();
    // [<4,0>,<1,1>,<5,2>,<2,3>,<3,4>]
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
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
        ASSERT_NE( nullptr, m );
        auto res = pl->append( *m );
        ASSERT_TRUE( res );
    }
    CheckContiguity();

    // [<1,0>,<2,1>,<3,2>,<4,3>,<5,4>]
    auto res = pl->move( 4, 0 );
    ASSERT_TRUE( res );
    CheckContiguity();
    // [<5,0>,<1,1>,<2,2>,<3,3>,<4,4>]
    auto media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 5u, media[0]->id() );
    ASSERT_EQ( 1u, media[1]->id() );
    ASSERT_EQ( 2u, media[2]->id() );
    ASSERT_EQ( 3u, media[3]->id() );
    ASSERT_EQ( 4u, media[4]->id() );

    // Now move some item forward
    pl->move( 1, 4 );
    // [<5,0>,<2,1>,<3,2>,<4,3>,<1,4>]
    media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 5u, media[0]->id() );
    ASSERT_EQ( 2u, media[1]->id() );
    ASSERT_EQ( 3u, media[2]->id() );
    ASSERT_EQ( 4u, media[3]->id() );
    ASSERT_EQ( 1u, media[4]->id() );
    CheckContiguity();

    // Move an item past the theorical last element
    pl->move( 1, 10 );
    // [<5,0>,<3,1>,<4,2>,<1,3>,<2,10/4>]
    media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 5u, media[0]->id() );
    ASSERT_EQ( 3u, media[1]->id() );
    ASSERT_EQ( 4u, media[2]->id() );
    ASSERT_EQ( 1u, media[3]->id() );
    ASSERT_EQ( 2u, media[4]->id() );
    CheckContiguity();

    // But check that this didn't create a gap in the items (if we move an item
    // to position 9, it should still be the last element in the playlist
    pl->move( 1, 9 );
    // [<5,0>,<4,1>,<1,2>,<2,3>,<3,9/4>]
    media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 5u, media[0]->id() );
    ASSERT_EQ( 4u, media[1]->id() );
    ASSERT_EQ( 1u, media[2]->id() );
    ASSERT_EQ( 2u, media[3]->id() );
    ASSERT_EQ( 3u, media[4]->id() );

    // Try to remove a dummy element and check that it's handled properly
    res = pl->move( 123, 2 );
    ASSERT_FALSE( res );
}

TEST_F( Playlists, Remove )
{
    for ( auto i = 1; i < 6; ++i )
    {
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
        ASSERT_NE( nullptr, m );
        auto res = pl->append( *m );
        ASSERT_TRUE( res );
    }
    // [<1,0>,<2,1>,<3,2>,<4,3>,<5,4>]
    auto media = pl->media()->all();
    ASSERT_EQ( 5u, media.size() );
    CheckContiguity();

    pl->remove( 2 );
    // [<1,0>,<2,1>,<4,2>,<5,3>]

    media = pl->media()->all();
    ASSERT_EQ( 4u, media.size() );

    ASSERT_EQ( 1u, media[0]->id() );
    ASSERT_EQ( 2u, media[1]->id() );
    ASSERT_EQ( 4u, media[2]->id() );
    ASSERT_EQ( 5u, media[3]->id() );
    CheckContiguity();
}

TEST_F( Playlists, DeleteFile )
{
    for ( auto i = 1; i < 6; ++i )
    {
        auto m = ml->addMedia( "media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
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
    CheckContiguity();

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
    auto pl2 = ml->playlist( pl->id() );
    ASSERT_NE( nullptr, pl2 );
}

TEST_F( Playlists, Search )
{
    ml->createPlaylist( "playlist 2" );
    ml->createPlaylist( "laylist 3" );

    auto query = ml->searchPlaylists( "", nullptr );
    ASSERT_EQ( nullptr, query );

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

    params.sort = SortingCriteria::NbAudio;
    params.desc = true;
    pls = ml->playlists( &params )->all();
    ASSERT_EQ( 2u, pls.size() );
    ASSERT_EQ( pl->id(), pls[0]->id() );
    ASSERT_EQ( pl2->id(), pls[1]->id() );
}

TEST_F( Playlists, AddDuplicate )
{
    auto m = ml->addMedia( "file.mkv", IMedia::Type::Video );
    auto res = pl->append( *m );
    ASSERT_TRUE( res );
    res = pl->append( *m );
    ASSERT_TRUE( res );
    auto media = pl->media()->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m->id(), media[0]->id() );
    ASSERT_EQ( m->id(), media[1]->id() );
    CheckContiguity();

    auto count = pl->media()->count();
    ASSERT_EQ( count, media.size() );
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
    auto m1 = ml->addExternalMedia( "http://sea.otters/fluffy.mkv" );
    auto m2 = ml->addExternalMedia( "https:///cuteotters.org/holding_hands.mp4" );
    auto m3 = ml->addMedia( "media.mp3", IMedia::Type::Audio );
    pl->append( *m1 );
    pl->append( *m2 );
    pl->append( *m3 );

    auto media = pl->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    CheckContiguity();

    // Ensure the media we fetch are recreated by checking their ids before/after
    auto m1Id = m1->id();
    auto m2Id = m2->id();

    ml->deleteMedia( m1->id() );
    ml->deleteMedia( m2->id() );
    CheckContiguity();

    pl = std::static_pointer_cast<Playlist>( ml->playlist( pl->id() ) );

    m1 = ml->addExternalMedia( "http://sea.otters/fluffy.mkv" );
    m2 = ml->addExternalMedia( "https:///cuteotters.org/holding_hands.mp4" );

    media = pl->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    ASSERT_NE( m1->id(), m1Id );
    ASSERT_NE( m2->id(), m2Id );
    CheckContiguity();
}

TEST_F( Playlists, RemoveMedia )
{
    auto m1 = ml->addExternalMedia( "http://sea.otters/fluffy.mkv" );
    auto m2 = ml->addMedia( "file:///cute_otters_holding_hands.mp4", IMedia::Type::Video );
    auto m3 = ml->addMedia( "media.mp3", IMedia::Type::Audio );
    pl->append( *m1 );
    pl->append( *m2 );
    pl->append( *m3 );

    auto media = pl->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    CheckContiguity();

    ml->deleteMedia( m1->id() );
    ml->deleteMedia( m2->id() );

    pl = std::static_pointer_cast<Playlist>( ml->playlist( pl->id() ) );

    media = pl->media()->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m3->id(), media[0]->id() );
    CheckContiguity();
}

TEST_F( Playlists, ClearContent )
{
    auto m1 = ml->addMedia( "seaotter.mkv", IMedia::Type::Video );
    auto m2 = ml->addMedia( "fluffyfurball.mp4", IMedia::Type::Video );
    auto pl2 = ml->createPlaylist( "playlist 2" );

    pl->append( *m1 );
    pl2->append( *m2 );

    ASSERT_EQ( 1u, pl->media()->count() );
    ASSERT_EQ( 1u, pl2->media()->count() );

    pl->clearContent();

    ASSERT_EQ( 0u, pl->media()->count() );
    ASSERT_EQ( 1u, pl2->media()->count() );
}

TEST_F( Playlists, RemoveReAddMedia )
{
    auto m1 = ml->addMedia( "one.mp3", IMedia::Type::Audio );
    auto m2 = ml->addMedia( "second soufle.mp3", IMedia::Type::Audio );
    auto m3 = ml->addMedia( "third quarter storm.mp3", IMedia::Type::Audio );
    pl->append( *m1 );
    pl->append( *m2 );
    pl->append( *m3 );

    auto media = pl->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    CheckContiguity();

    // Remove the middle element
    pl->remove( 1 );

    media = pl->media()->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    CheckContiguity();

    pl->add( *m2, 1 );

    media = pl->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    CheckContiguity();

    pl->remove( 0 );

    media = pl->media()->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    CheckContiguity();

    pl->add( *m1, 0 );

    media = pl->media()->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    CheckContiguity();
}

TEST_F( Playlists, InsertRemoveDuplicateMedia )
{
    MediaPtr m;
    for ( auto i = 0u; i < 5; ++i )
    {
        m = ml->addMedia( std::to_string( i + 1 ) + ".mp3", IMedia::Type::Audio );
        pl->append( *m );
    }
    pl->append( *m );

    auto items = pl->media()->all();
    ASSERT_EQ( 6u, items.size() );
    ASSERT_EQ( 1u, items[0]->id() );
    ASSERT_EQ( 2u, items[1]->id() );
    ASSERT_EQ( 3u, items[2]->id() );
    ASSERT_EQ( 4u, items[3]->id() );
    ASSERT_EQ( 5u, items[4]->id() );
    ASSERT_EQ( 5u, items[5]->id() );
    CheckContiguity();

    auto res = pl->remove( 4 );

    ASSERT_TRUE( res );
    items = pl->media()->all();

    ASSERT_EQ( 5u, items.size() );
    ASSERT_EQ( 1u, items[0]->id() );
    ASSERT_EQ( 2u, items[1]->id() );
    ASSERT_EQ( 3u, items[2]->id() );
    ASSERT_EQ( 4u, items[3]->id() );
    ASSERT_EQ( 5u, items[4]->id() );
    CheckContiguity();
}

TEST_F( Playlists, AutoRemoveTask )
{
    auto t = parser::Task::createLinkTask( ml.get(), "file:///tmp/playlist.m3u",
                                           pl->id(), parser::Task::LinkType::Playlist,
                                           0 );
    ASSERT_EQ( 1u, ml->countNbTasks() );
    Playlist::destroy( ml.get(), pl->id() );
    ASSERT_EQ( 0u, ml->countNbTasks() );
}

TEST_F( Playlists, CheckDbModel )
{
    auto res = Playlist::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( Playlists, IsReadOnly )
{
    ASSERT_FALSE( pl->isReadOnly() );
    ASSERT_EQ( "", pl->mrl() );
}

TEST_F( Playlists, SortByCreationDate )
{
    auto pl2 = ml->createPlaylist( "zzzzzz" );
    auto pl3 = ml->createPlaylist( "aaaaaa" );
    auto forceCreationDate = [this]( int64_t plId, time_t t ) {
        const std::string req = "UPDATE " + Playlist::Table::Name +
                " SET creation_date = ? WHERE id_playlist = ?";
        return sqlite::Tools::executeUpdate( ml->getConn(), req, t, plId );
    };
    forceCreationDate( pl->id(),  99999 );
    forceCreationDate( pl2->id(), 11111 );
    forceCreationDate( pl3->id(), 12345 );

    QueryParameters params{};
    params.sort = SortingCriteria::InsertionDate;
    params.desc = false;
    auto playlists = ml->playlists( &params )->all();
    ASSERT_EQ( 3u, playlists.size() );
    ASSERT_EQ( pl2->id(), playlists[0]->id() );
    ASSERT_EQ( pl3->id(), playlists[1]->id() );
    ASSERT_EQ( pl->id(), playlists[2]->id() );

    params.desc = true;
    playlists = ml->playlists( &params )->all();
    ASSERT_EQ( 3u, playlists.size() );
    ASSERT_EQ( pl->id(), playlists[0]->id() );
    ASSERT_EQ( pl3->id(), playlists[1]->id() );
    ASSERT_EQ( pl2->id(), playlists[2]->id() );
}
