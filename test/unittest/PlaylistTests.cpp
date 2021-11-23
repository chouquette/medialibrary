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

#include "UnitTests.h"

#include "File.h"
#include "Playlist.h"
#include "Media.h"
#include "parser/Task.h"

struct PlaylistTests : public Tests
{
    std::shared_ptr<Playlist> pl;

    virtual void TestSpecificSetup() override
    {
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

static void Create( PlaylistTests* T )
{
    ASSERT_NE( nullptr, T->pl );
    ASSERT_NE( 0u, T->pl->id() );
    ASSERT_EQ( "test playlist", T->pl->name() );
    ASSERT_NE( 0u, T->pl->creationDate() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );
    ASSERT_EQ( 0u, T->pl->nbPresentVideo() );
    ASSERT_EQ( 0u, T->pl->nbPresentAudio() );
    ASSERT_EQ( 0u, T->pl->nbPresentUnknown() );
}

static void CreateDuplicate( PlaylistTests* T )
{
    auto p = T->ml->createPlaylist(T->pl->name());
    ASSERT_NE( nullptr, p );
    ASSERT_NE( p->id(), T->pl->id() );
}

static void Fetch( PlaylistTests* T )
{
    auto pl2 = T->ml->playlist( T->pl->id() );
    ASSERT_NE( nullptr, pl2 );
    ASSERT_EQ( T->pl->id(), pl2->id() );

    auto playlists = T->ml->playlists( PlaylistType::All, nullptr )->all();
    ASSERT_EQ( 1u, playlists.size() );
    ASSERT_EQ( T->pl->id(), playlists[0]->id() );
}

static void DeletePlaylist( PlaylistTests* T )
{
    auto res = T->ml->deletePlaylist( T->pl->id() );
    ASSERT_TRUE( res );
    auto playlists = T->ml->playlists( PlaylistType::All, nullptr )->all();
    ASSERT_EQ( 0u, playlists.size() );
}

static void SetName( PlaylistTests* T )
{
    ASSERT_EQ( "test playlist", T->pl->name() );
    auto newName = "new name";
    auto res = T->pl->setName( newName );
    ASSERT_TRUE( res );
    ASSERT_EQ( newName, T->pl->name() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( newName, T->pl->name() );
}

static void FetchAll( PlaylistTests* T )
{
    T->pl->setName( "T->pl 1" );
    T->ml->createPlaylist( "T->pl 2" );
    T->ml->createPlaylist( "T->pl 3" );
    T->ml->createPlaylist( "T->pl 4" );

    auto playlists = T->ml->playlists( PlaylistType::All, nullptr )->all();
    ASSERT_EQ( 4u, playlists.size() );
    for ( auto& p : playlists )
    {
        auto name = std::string{ "T->pl " } + std::to_string( p->id() );
        ASSERT_EQ( name, p->name() );
    }
}

static void Add( PlaylistTests* T )
{
    auto m = T->ml->addMedia( "file.mkv", IMedia::Type::Video );
    auto res = T->pl->append( *m );

    T->CheckContiguity();

    ASSERT_TRUE( res );
    auto media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m->id(), media[0]->id() );
}

static void Append( PlaylistTests* T )
{
    for ( auto i = 0; i < 5; ++i )
    {
        auto m = T->ml->addMedia( "media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
        ASSERT_NE( nullptr, m );
        T->pl->append( *m );
    }
    auto media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 5u, media.size() );
    for ( auto i = 0u; i < media.size(); ++i )
    {
        auto name = "media" + std::to_string( i ) + ".mkv";
        ASSERT_EQ( media[i]->title(), name );
    }
    T->CheckContiguity();
}

static void Insert( PlaylistTests* T )
{
    for ( auto i = 1; i < 4; ++i )
    {
        auto m = T->ml->addMedia( "media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
        ASSERT_NE( nullptr, m );
        auto res = T->pl->append( m->id() );
        ASSERT_TRUE( res );
    }
    // [<1,0>,<2,1>,<3,2>]
    auto firstMedia = T->ml->addMedia( "first.mkv", IMedia::Type::Video );

    T->pl->add( *firstMedia, 0 );
    T->CheckContiguity();

    // [<4,0>,<1,1>,<2,2>,<3,3>]
    auto middleMedia = T->ml->addMedia( "middle.mkv", IMedia::Type::Video );
    T->pl->add( *middleMedia, 2 );
    T->CheckContiguity();
    // [<4,0>,<1,1>,<5,2>,<2,3>,<3,4>]
    auto media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 4u, media[0]->id() );
    ASSERT_EQ( 1u, media[1]->id() );
    ASSERT_EQ( 5u, media[2]->id() );
    ASSERT_EQ( 2u, media[3]->id() );
    ASSERT_EQ( 3u, media[4]->id() );
}

static void Move( PlaylistTests* T )
{
    for ( auto i = 1; i < 6; ++i )
    {
        auto m = T->ml->addMedia( "media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
        ASSERT_NE( nullptr, m );
        auto res = T->pl->append( *m );
        ASSERT_TRUE( res );
    }
    T->CheckContiguity();

    // [<1,0>,<2,1>,<3,2>,<4,3>,<5,4>]
    auto res = T->pl->move( 4, 0 );
    ASSERT_TRUE( res );
    T->CheckContiguity();
    // [<5,0>,<1,1>,<2,2>,<3,3>,<4,4>]
    auto media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 5u, media[0]->id() );
    ASSERT_EQ( 1u, media[1]->id() );
    ASSERT_EQ( 2u, media[2]->id() );
    ASSERT_EQ( 3u, media[3]->id() );
    ASSERT_EQ( 4u, media[4]->id() );

    // Now move some item forward
    T->pl->move( 1, 4 );
    // [<5,0>,<2,1>,<3,2>,<4,3>,<1,4>]
    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 5u, media[0]->id() );
    ASSERT_EQ( 2u, media[1]->id() );
    ASSERT_EQ( 3u, media[2]->id() );
    ASSERT_EQ( 4u, media[3]->id() );
    ASSERT_EQ( 1u, media[4]->id() );
    T->CheckContiguity();

    // Move an item past the theorical last element
    T->pl->move( 1, 10 );
    // [<5,0>,<3,1>,<4,2>,<1,3>,<2,10/4>]
    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 5u, media[0]->id() );
    ASSERT_EQ( 3u, media[1]->id() );
    ASSERT_EQ( 4u, media[2]->id() );
    ASSERT_EQ( 1u, media[3]->id() );
    ASSERT_EQ( 2u, media[4]->id() );
    T->CheckContiguity();

    // But check that this didn't create a gap in the items (if we move an item
    // to position 9, it should still be the last element in the playlist
    T->pl->move( 1, 9 );
    // [<5,0>,<4,1>,<1,2>,<2,3>,<3,9/4>]
    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 5u, media.size() );

    ASSERT_EQ( 5u, media[0]->id() );
    ASSERT_EQ( 4u, media[1]->id() );
    ASSERT_EQ( 1u, media[2]->id() );
    ASSERT_EQ( 2u, media[3]->id() );
    ASSERT_EQ( 3u, media[4]->id() );

    // Try to remove a dummy element and check that it's handled properly
    res = T->pl->move( 123, 2 );
    ASSERT_FALSE( res );
}

static void Remove( PlaylistTests* T )
{
    for ( auto i = 1; i < 6; ++i )
    {
        auto m = T->ml->addMedia( "media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
        ASSERT_NE( nullptr, m );
        auto res = T->pl->append( *m );
        ASSERT_TRUE( res );
    }
    // [<1,0>,<2,1>,<3,2>,<4,3>,<5,4>]
    auto media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 5u, media.size() );
    T->CheckContiguity();

    T->pl->remove( 2 );
    // [<1,0>,<2,1>,<4,2>,<5,3>]

    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 4u, media.size() );

    ASSERT_EQ( 1u, media[0]->id() );
    ASSERT_EQ( 2u, media[1]->id() );
    ASSERT_EQ( 4u, media[2]->id() );
    ASSERT_EQ( 5u, media[3]->id() );
    T->CheckContiguity();
}

static void DeleteFile( PlaylistTests* T )
{
    for ( auto i = 1; i < 6; ++i )
    {
        auto m = T->ml->addMedia( "file://media" + std::to_string( i ) + ".mkv", IMedia::Type::Video );
        ASSERT_NE( nullptr, m );
        auto res = T->pl->append( *m );
        ASSERT_TRUE( res );
    }
    // [<1,1>,<2,2>,<3,3>,<4,4>,<5,5>]
    auto media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 5u, media.size() );
    auto m = std::static_pointer_cast<Media>( media[2] );
    auto fs = m->files();
    ASSERT_EQ( 1u, fs.size() );
    m->removeFile( static_cast<File&>( *fs[0] ) );
    // This should trigger the Media removal, which should in
    // turn trigger the playlist item removal
    // So we should now have
    // [<1,1>,<2,2>,<4,4>,<5,5>]

    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 4u, media.size() );

    ASSERT_EQ( 1u, media[0]->id() );
    ASSERT_EQ( 2u, media[1]->id() );
    ASSERT_EQ( 4u, media[2]->id() );
    ASSERT_EQ( 5u, media[3]->id() );
    T->CheckContiguity();

    // Ensure we don't delete an empty playlist:
    auto ms = T->ml->files();
    for ( auto &mptr : ms )
    {
        auto files = mptr->files();
        ASSERT_EQ( 1u, files.size() );
        std::static_pointer_cast<Media>( mptr )->removeFile( static_cast<File&>( *files[0] ) );
    }
    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 0u, media.size() );
    auto pl2 = T->ml->playlist( T->pl->id() );
    ASSERT_NE( nullptr, pl2 );
}

static void Search( PlaylistTests* T )
{
    T->ml->createPlaylist( "playlist 2" );
    T->ml->createPlaylist( "laylist 3" );

    QueryParameters params{};
    params.includeMissing = true;

    auto playlists = T->ml->searchPlaylists( "play", &params )->all();
    ASSERT_EQ( 2u, playlists.size() );

    params.includeMissing = false;
    playlists = T->ml->searchPlaylists( "play", &params )->all();
    ASSERT_EQ( 0u, playlists.size() );
}

static void SearchAndSort( PlaylistTests* T )
{
    auto pl2 = T->ml->createPlaylist( "playlist 2" );

    QueryParameters params{};
    params.includeMissing = true;
    auto playlists = T->ml->searchPlaylists( "play", &params )->all();
    ASSERT_EQ( 2u, playlists.size() );
    ASSERT_EQ( pl2->id(), playlists[0]->id() );
    ASSERT_EQ( T->pl->id(), playlists[1]->id() );

    params.includeMissing = false;
    playlists = T->ml->searchPlaylists( "play", &params )->all();
    ASSERT_EQ( 0u, playlists.size() );

    params.sort = SortingCriteria::Default;
    params.desc = true;
    params.includeMissing = true;
    playlists = T->ml->searchPlaylists( "play", &params )->all();
    ASSERT_EQ( 2u, playlists.size() );
    ASSERT_EQ( T->pl->id(), playlists[0]->id() );
    ASSERT_EQ( pl2->id(), playlists[1]->id() );
}

static void SearchAfterDelete( PlaylistTests* T )
{
    QueryParameters params{};
    params.includeMissing = true;
    auto pl = T->ml->createPlaylist( "sea otters greatest hits" );
    auto pls = T->ml->searchPlaylists( "sea otters", &params )->all();
    ASSERT_EQ( 1u, pls.size() );

    T->ml->deletePlaylist( pl->id() );

    pls = T->ml->searchPlaylists( "sea otters", &params )->all();
    ASSERT_EQ( 0u, pls.size() );
}

static void SearchAfterUpdate( PlaylistTests* T )
{
    QueryParameters params{};
    params.includeMissing = true;
    auto pl = T->ml->createPlaylist( "sea otters greatest hits" );
    auto pls = T->ml->searchPlaylists( "sea otters", &params )->all();
    ASSERT_EQ( 1u, pls.size() );

    pl->setName( "pangolins are cool too" );

    pls = T->ml->searchPlaylists( "sea otters", &params )->all();
    ASSERT_EQ( 0u, pls.size() );

    pls = T->ml->searchPlaylists( "pangolins", &params )->all();
    ASSERT_EQ( 1u, pls.size() );
}

static void Sort( PlaylistTests* T )
{
    auto pl2 = T->ml->createPlaylist( "A playlist" );

    auto pls = T->ml->playlists( PlaylistType::All, nullptr )->all();
    ASSERT_EQ( 2u, pls.size() );
    ASSERT_EQ( pl2->id(), pls[0]->id() );
    ASSERT_EQ( T->pl->id(), pls[1]->id() );

    QueryParameters params { SortingCriteria::Default, true };
    pls = T->ml->playlists( PlaylistType::All, &params )->all();
    ASSERT_EQ( 2u, pls.size() );
    ASSERT_EQ( pl2->id(), pls[1]->id() );
    ASSERT_EQ( T->pl->id(), pls[0]->id() );

    params.sort = SortingCriteria::NbAudio;
    params.desc = true;
    pls = T->ml->playlists( PlaylistType::All, &params )->all();
    ASSERT_EQ( 2u, pls.size() );
    ASSERT_EQ( T->pl->id(), pls[0]->id() );
    ASSERT_EQ( pl2->id(), pls[1]->id() );
}

static void AddDuplicate( PlaylistTests* T )
{
    auto m = T->ml->addMedia( "file.mkv", IMedia::Type::Video );
    auto res = T->pl->append( *m );
    ASSERT_TRUE( res );
    res = T->pl->append( *m );
    ASSERT_TRUE( res );
    auto media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m->id(), media[0]->id() );
    ASSERT_EQ( m->id(), media[1]->id() );
    T->CheckContiguity();

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 2u, T->pl->nbMedia() );
    ASSERT_EQ( 2u, T->pl->nbPresentMedia() );

    auto count = T->pl->media( nullptr )->count();
    ASSERT_EQ( count, media.size() );

    res = T->pl->remove( 1 );
    ASSERT_TRUE( res );

    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 1u, media.size() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 1u, T->pl->nbMedia() );
    ASSERT_EQ( 1u, T->pl->nbPresentMedia() );

    /*
     * Re-add the duplicated media and check that the count is properly updated
     * on deletion
     */
    T->pl->append( m->id() );
    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 2u, media.size() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 2u, T->pl->nbMedia() );
    ASSERT_EQ( 2u, T->pl->nbPresentMedia() );

    T->ml->deleteMedia( m->id() );

    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 0u, T->pl->nbMedia() );
    ASSERT_EQ( 0u, T->pl->nbPresentMedia() );
}

static void SearchMedia( PlaylistTests* T )
{
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "m1.mp3", IMedia::Type::Audio ) );
    m1->setTitle( "otter", true );

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "m2.mp3", IMedia::Type::Audio ) );
    // Won't match since it's not on the beginning of a word
    m2->setTitle( "ENOOTTER", true );

    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "m3.mp3", IMedia::Type::Audio ) );
    m3->setTitle( "otter otter otter", true );

    T->pl->append( *m1 );
    T->pl->append( *m2 );

    auto media = T->ml->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 2u, media.size() );

    media = T->pl->searchMedia( "otter", nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
}

static void RemoveMedia( PlaylistTests* T )
{
    auto m1 = T->ml->addExternalMedia( "http://sea.otters/fluffy.mkv", -1 );
    auto m2 = T->ml->addMedia( "file:///cute_otters_holding_hands.mp4", IMedia::Type::Video );
    auto m3 = T->ml->addMedia( "media.mp3", IMedia::Type::Audio );
    T->pl->append( *m1 );
    T->pl->append( *m2 );
    T->pl->append( *m3 );

    auto media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    T->CheckContiguity();

    T->ml->deleteMedia( m1->id() );
    T->ml->deleteMedia( m2->id() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );

    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( m3->id(), media[0]->id() );
    T->CheckContiguity();
}

static void ClearContent( PlaylistTests* T )
{
    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "seaotter.mkv", IMedia::Type::Video ) );
    auto m2 = T->ml->addMedia( "fluffyfurball.mp4", IMedia::Type::Video );
    auto pl2 = T->ml->createPlaylist( "playlist 2" );

    m1->setDuration( 123 );

    T->pl->append( *m1 );
    pl2->append( *m2 );

    ASSERT_EQ( 1u, T->pl->media( nullptr )->count() );
    ASSERT_EQ( 1u, pl2->media( nullptr )->count() );
    ASSERT_EQ( 123, T->pl->duration() );

    auto pl = T->ml->playlist( T->pl->id() );
    ASSERT_EQ( 123, pl->duration() );
    ASSERT_EQ( 1u, pl->nbVideo() );

    T->pl->clearContent();

    ASSERT_EQ( 0u, T->pl->media( nullptr )->count() );
    ASSERT_EQ( 1u, pl2->media( nullptr )->count() );
    pl = T->ml->playlist( T->pl->id() );
    ASSERT_EQ( 0, pl->duration() );
    ASSERT_EQ( 0u, pl->nbVideo() );
}

static void RemoveReAddMedia( PlaylistTests* T )
{
    auto m1 = T->ml->addMedia( "one.mp3", IMedia::Type::Audio );
    auto m2 = T->ml->addMedia( "second soufle.mp3", IMedia::Type::Audio );
    auto m3 = T->ml->addMedia( "third quarter storm.mp3", IMedia::Type::Audio );
    T->pl->append( *m1 );
    T->pl->append( *m2 );
    T->pl->append( *m3 );

    auto media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    T->CheckContiguity();

    // Remove the middle element
    T->pl->remove( 1 );

    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    T->CheckContiguity();

    T->pl->add( *m2, 1 );

    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    T->CheckContiguity();

    T->pl->remove( 0 );

    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 2u, media.size() );
    ASSERT_EQ( m2->id(), media[0]->id() );
    ASSERT_EQ( m3->id(), media[1]->id() );
    T->CheckContiguity();

    T->pl->add( *m1, 0 );

    media = T->pl->media( nullptr )->all();
    ASSERT_EQ( 3u, media.size() );
    ASSERT_EQ( m1->id(), media[0]->id() );
    ASSERT_EQ( m2->id(), media[1]->id() );
    ASSERT_EQ( m3->id(), media[2]->id() );
    T->CheckContiguity();
}

static void InsertRemoveDuplicateMedia( PlaylistTests* T )
{
    MediaPtr m;
    for ( auto i = 0u; i < 5; ++i )
    {
        m = T->ml->addMedia( std::to_string( i + 1 ) + ".mp3", IMedia::Type::Audio );
        T->pl->append( *m );
    }
    T->pl->append( *m );

    auto items = T->pl->media( nullptr )->all();
    ASSERT_EQ( 6u, items.size() );
    ASSERT_EQ( 1u, items[0]->id() );
    ASSERT_EQ( 2u, items[1]->id() );
    ASSERT_EQ( 3u, items[2]->id() );
    ASSERT_EQ( 4u, items[3]->id() );
    ASSERT_EQ( 5u, items[4]->id() );
    ASSERT_EQ( 5u, items[5]->id() );
    T->CheckContiguity();

    auto res = T->pl->remove( 4 );

    ASSERT_TRUE( res );
    items = T->pl->media( nullptr )->all();

    ASSERT_EQ( 5u, items.size() );
    ASSERT_EQ( 1u, items[0]->id() );
    ASSERT_EQ( 2u, items[1]->id() );
    ASSERT_EQ( 3u, items[2]->id() );
    ASSERT_EQ( 4u, items[3]->id() );
    ASSERT_EQ( 5u, items[4]->id() );
    T->CheckContiguity();
}

static void AutoRemoveTask( PlaylistTests* T )
{
    auto t = parser::Task::createLinkTask( T->ml.get(), "file:///tmp/playlist.m3u",
                                           T->pl->id(), parser::Task::LinkType::Playlist,
                                           0 );
    ASSERT_NON_NULL( t );
    ASSERT_EQ( 1u, T->ml->countNbTasks() );
    Playlist::destroy( T->ml.get(), T->pl->id() );
    ASSERT_EQ( 0u, T->ml->countNbTasks() );
}

static void CheckDbModel( PlaylistTests* T )
{
    auto res = Playlist::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void IsReadOnly( PlaylistTests* T )
{
    ASSERT_FALSE( T->pl->isReadOnly() );
    ASSERT_EQ( "", T->pl->mrl() );
}

static void SortByCreationDate( PlaylistTests* T )
{
    auto pl2 = T->ml->createPlaylist( "zzzzzz" );
    auto pl3 = T->ml->createPlaylist( "aaaaaa" );
    auto forceCreationDate = [T]( int64_t plId, time_t t ) {
        const std::string req = "UPDATE " + Playlist::Table::Name +
                " SET creation_date = ? WHERE id_playlist = ?";
        return sqlite::Tools::executeUpdate( T->ml->getConn(), req, t, plId );
    };
    forceCreationDate( T->pl->id(),  99999 );
    forceCreationDate( pl2->id(), 11111 );
    forceCreationDate( pl3->id(), 12345 );

    QueryParameters params{};
    params.sort = SortingCriteria::InsertionDate;
    params.desc = false;
    auto playlists = T->ml->playlists( PlaylistType::All, &params )->all();
    ASSERT_EQ( 3u, playlists.size() );
    ASSERT_EQ( pl2->id(), playlists[0]->id() );
    ASSERT_EQ( pl3->id(), playlists[1]->id() );
    ASSERT_EQ( T->pl->id(), playlists[2]->id() );

    params.desc = true;
    playlists = T->ml->playlists( PlaylistType::All, &params )->all();
    ASSERT_EQ( 3u, playlists.size() );
    ASSERT_EQ( T->pl->id(), playlists[0]->id() );
    ASSERT_EQ( pl3->id(), playlists[1]->id() );
    ASSERT_EQ( pl2->id(), playlists[2]->id() );
}

static void NbMedia( PlaylistTests* T )
{
    ASSERT_EQ( 0u, T->pl->nbMedia() );
    ASSERT_EQ( 0u, T->pl->nbPresentMedia() );
    auto media = T->ml->addMedia( "media.mkv", IMedia::Type::Video );
    auto media2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    auto res = T->pl->append( *media );
    ASSERT_TRUE( res );
    ASSERT_EQ( 1u, T->pl->nbMedia() );
    ASSERT_EQ( 1u, T->pl->nbPresentMedia() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 1u, T->pl->nbMedia() );
    ASSERT_EQ( 1u, T->pl->nbPresentMedia() );

    res = T->pl->add( *media2, 0 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 2u, T->pl->nbMedia() );
    ASSERT_EQ( 2u, T->pl->nbPresentMedia() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 2u, T->pl->nbMedia() );
    ASSERT_EQ( 2u, T->pl->nbPresentMedia() );

    res = T->pl->move( 0, 999 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 2u, T->pl->nbMedia() );
    ASSERT_EQ( 2u, T->pl->nbPresentMedia() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 2u, T->pl->nbMedia() );
    ASSERT_EQ( 2u, T->pl->nbPresentMedia() );

    res = T->pl->remove( 0 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 1u, T->pl->nbMedia() );
    ASSERT_EQ( 1u, T->pl->nbPresentMedia() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 1u, T->pl->nbMedia() );
    ASSERT_EQ( 1u, T->pl->nbPresentMedia() );

    res = T->pl->remove( 0 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 0u, T->pl->nbMedia() );
    ASSERT_EQ( 0u, T->pl->nbPresentMedia() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 0u, T->pl->nbMedia() );
    ASSERT_EQ( 0u, T->pl->nbPresentMedia() );
}

static void Duration( PlaylistTests* T )
{
    ASSERT_EQ( 0, T->pl->duration() );
    ASSERT_EQ( 0u, T->pl->nbDurationUnknown() );

    // Add a media with a known duration
    auto m1 = T->ml->addExternalMedia( "http://media.org/test.mkv", 1300 );
    ASSERT_EQ( m1->duration(), 1300 );

    auto res = T->pl->append( *m1 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 0u, T->pl->nbDurationUnknown() );

    ASSERT_EQ( T->pl->duration(), 1300 );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( T->pl->duration(), 1300 );

    // Now add a media with an unknown duration (-1) and check that it didn't
    // decrement the playlist duration
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file://path/to/media.mkv", IMedia::Type::Video ) );
    ASSERT_EQ( m2->duration(), -1 );
    res = T->pl->append( *m2 );
    ASSERT_TRUE( res );

    ASSERT_EQ( T->pl->duration(), 1300 );
    ASSERT_EQ( 1u, T->pl->nbDurationUnknown() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( T->pl->duration(), 1300 );
    ASSERT_EQ( 1u, T->pl->nbDurationUnknown() );

    // Now remove the media with the unknown duration and check that we still
    // didn't update the playlist duration

    res = T->pl->remove( 1 );
    ASSERT_TRUE( res );

    ASSERT_EQ( T->pl->duration(), 1300 );
    ASSERT_EQ( 0u, T->pl->nbDurationUnknown() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( T->pl->duration(), 1300 );
    ASSERT_EQ( 0u, T->pl->nbDurationUnknown() );

    // Now reinsert the unknown duration media
    res = T->pl->append( *m2 );
    ASSERT_TRUE( res );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( T->pl->duration(), 1300 );
    ASSERT_EQ( 1u, T->pl->nbDurationUnknown() );

    // And update its duration
    res = m2->setDuration( 12 );
    ASSERT_TRUE( res );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( T->pl->duration(), 1312 );
    ASSERT_EQ( 0u, T->pl->nbDurationUnknown() );

    // Ensure that if for whatever reason the duration goes back to unknown, we
    // don't end up adding '-1' to the playlist duration
    res = m2->setDuration( -1 );
    ASSERT_TRUE( res );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( T->pl->duration(), 1300 );
    ASSERT_EQ( 1u, T->pl->nbDurationUnknown() );

    // Now remove both media one after another
    res = T->pl->remove( 1 );
    ASSERT_TRUE( res );

    ASSERT_EQ( T->pl->duration(), 1300 );
    ASSERT_EQ( 0u, T->pl->nbDurationUnknown() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( T->pl->duration(), 1300 );
    ASSERT_EQ( 0u, T->pl->nbDurationUnknown() );

    res = T->pl->remove( 0 );
    ASSERT_TRUE( res );

    ASSERT_EQ( T->pl->duration(), 0 );
    ASSERT_EQ( 0u, T->pl->nbDurationUnknown() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( T->pl->duration(), 0 );
    ASSERT_EQ( 0u, T->pl->nbDurationUnknown() );

    /*
     * Now insert the same media twice and check that it behaves properly when
     * removing and that media.
     */
    res = T->pl->append( *m1 );
    ASSERT_TRUE( res );
    res = T->pl->append( *m1 );
    ASSERT_TRUE( res );
    res = T->pl->append( *m1 );
    ASSERT_TRUE( res );

    ASSERT_EQ( 3 * m1->duration(), T->pl->duration() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 3 * m1->duration(), T->pl->duration() );

    /* Remove the media only once from the playlist */
    res = T->pl->remove( 0 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 2 * m1->duration(), T->pl->duration() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 2 * m1->duration(), T->pl->duration() );

    /* Now delete the media and ensure we removed its duration twice */
    T->ml->deleteMedia( m1->id() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 0, T->pl->duration() );
}

static void UpdateNbMediaOnInsertAndDelete( PlaylistTests* T )
{
    auto m1 = T->ml->addMedia( "media1.mp3", IMedia::Type::Audio );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );

    ASSERT_EQ( 0u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );
    auto res = T->pl->append( *m1 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 1u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 1u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );

    res = T->pl->add( *m2, 0 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 1u, T->pl->nbAudio() );
    ASSERT_EQ( 1u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 1u, T->pl->nbAudio() );
    ASSERT_EQ( 1u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );

    res = T->pl->remove( 0 );
    ASSERT_TRUE( res );
    ASSERT_EQ( 1u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );
    // remove() doesn't update the nb_non_audio since it's not passed a media instance
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 1u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );
}

static void UpdateNbMediaOnMediaRemoval( PlaylistTests* T )
{
    auto pl2 = T->ml->createPlaylist( "pl2" );
    auto m1 = T->ml->addMedia( "media1.mp3", IMedia::Type::Audio );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    auto m3 = T->ml->addMedia( "media3.mp3", IMedia::Type::Audio );

    ASSERT_EQ( 0u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );
    auto res = T->pl->append( *m1 );
    ASSERT_TRUE( res );
    res = T->pl->append( *m2 );
    ASSERT_TRUE( res );
    res = T->pl->append( *m3 );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, T->pl->nbAudio() );
    ASSERT_EQ( 1u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 2u, T->pl->nbAudio() );
    ASSERT_EQ( 1u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );

    res = pl2->append( *m1 );
    ASSERT_TRUE( res );
    res = pl2->append( *m2 );
    ASSERT_TRUE( res );

    ASSERT_EQ( 1u, pl2->nbAudio() );
    ASSERT_EQ( 1u, pl2->nbVideo() );
    ASSERT_EQ( 0u, pl2->nbUnknown() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbAudio() );
    ASSERT_EQ( 1u, pl2->nbVideo() );
    ASSERT_EQ( 0u, pl2->nbUnknown() );

    // Now delete the media, since it involves different path for the triggers
    T->ml->deleteMedia( m2->id() );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 2u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbAudio() );
    ASSERT_EQ( 0u, pl2->nbVideo() );
    ASSERT_EQ( 0u, pl2->nbUnknown() );

    T->ml->deleteMedia( m3->id() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 1u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbAudio() );
    ASSERT_EQ( 0u, pl2->nbVideo() );
    ASSERT_EQ( 0u, pl2->nbUnknown() );

    T->ml->deleteMedia( m1->id() );
    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 0u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 0u, pl2->nbAudio() );
    ASSERT_EQ( 0u, pl2->nbVideo() );
    ASSERT_EQ( 0u, pl2->nbUnknown() );
}

static void UpdateNbMediaOnMediaTypeChange( PlaylistTests* T )
{
    auto pl2 = T->ml->createPlaylist( "pl2" );
    auto m1 = T->ml->addMedia( "media1.mp3", IMedia::Type::Audio );
    auto m2 = T->ml->addMedia( "media2.mkv", IMedia::Type::Video );
    auto m3 = T->ml->addMedia( "media3.mp3", IMedia::Type::Audio );

    ASSERT_EQ( 0u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );
    auto res = T->pl->append( *m1 );
    ASSERT_TRUE( res );
    res = T->pl->append( *m2 );
    ASSERT_TRUE( res );
    res = T->pl->append( *m3 );
    ASSERT_TRUE( res );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 2u, T->pl->nbAudio() );
    ASSERT_EQ( 1u, T->pl->nbVideo() );
    ASSERT_EQ( 0u, T->pl->nbUnknown() );

    res = pl2->append( *m1 );
    ASSERT_TRUE( res );
    res = pl2->append( *m2 );
    ASSERT_TRUE( res );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbAudio() );
    ASSERT_EQ( 1u, pl2->nbVideo() );
    ASSERT_EQ( 0u, pl2->nbUnknown() );

    res = m3->setType( IMedia::Type::Unknown );
    ASSERT_TRUE( res );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 1u, T->pl->nbAudio() );
    ASSERT_EQ( 1u, T->pl->nbVideo() );
    ASSERT_EQ( 1u, T->pl->nbUnknown() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 1u, pl2->nbAudio() );
    ASSERT_EQ( 1u, pl2->nbVideo() );
    ASSERT_EQ( 0u, pl2->nbUnknown() );

    res = m2->setType( IMedia::Type::Audio );
    ASSERT_TRUE( res );

    T->pl = std::static_pointer_cast<Playlist>( T->ml->playlist( T->pl->id() ) );
    ASSERT_EQ( 2u, T->pl->nbAudio() );
    ASSERT_EQ( 0u, T->pl->nbVideo() );
    ASSERT_EQ( 1u, T->pl->nbUnknown() );

    pl2 = std::static_pointer_cast<Playlist>( T->ml->playlist( pl2->id() ) );
    ASSERT_EQ( 2u, pl2->nbAudio() );
    ASSERT_EQ( 0u, pl2->nbVideo() );
    ASSERT_EQ( 0u, pl2->nbUnknown() );
}

static void FilterByMediaType( PlaylistTests* T )
{
    auto empty = T->ml->createPlaylist( "empty" );
    ASSERT_NON_NULL( empty );
    auto videoOnly = T->ml->createPlaylist( "video only" );
    ASSERT_NON_NULL( videoOnly );

    auto m1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file://media.mkv", IMedia::Type::Video ) );
    auto m2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file://media.mp3", IMedia::Type::Audio ) );
    auto m3 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "file://media.ts", IMedia::Type::Unknown ) );

    /* Check that all playlists are returned when not filtering by media type */
    auto playlists = T->ml->playlists( PlaylistType::All, nullptr )->all();
    ASSERT_EQ( 3u, playlists.size() );

    /*
     * Insert the unknown media and check that it's considered as a video when
     * filtering by media type
     */
    auto res = videoOnly->append( *m3 );
    ASSERT_TRUE( res );
    playlists = T->ml->playlists( PlaylistType::VideoOnly, nullptr )->all();
    ASSERT_EQ( 1u, playlists.size() );
    ASSERT_EQ( videoOnly->id(), playlists[0]->id() );

    /* Insert a video to the playlist and check that it's still returned */
    res = videoOnly->append( *m1 );
    ASSERT_TRUE( res );
    playlists = T->ml->playlists( PlaylistType::VideoOnly, nullptr )->all();
    ASSERT_EQ( 1u, playlists.size() );
    ASSERT_EQ( videoOnly->id(), playlists[0]->id() );

    /* Add an audio media to a playlist and check that it's now returned */
    res = T->pl->append( *m2 );
    ASSERT_TRUE( res );
    playlists = T->ml->playlists( PlaylistType::AudioOnly, nullptr )->all();
    ASSERT_EQ( 1u, playlists.size() );
    ASSERT_EQ( T->pl->id(), playlists[0]->id() );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( PlaylistTests );

    ADD_TEST( Create );
    ADD_TEST( CreateDuplicate );
    ADD_TEST( Fetch );
    ADD_TEST( DeletePlaylist );
    ADD_TEST( SetName );
    ADD_TEST( FetchAll );
    ADD_TEST( Add );
    ADD_TEST( Append );
    ADD_TEST( Insert );
    ADD_TEST( Move );
    ADD_TEST( Remove );
    ADD_TEST( DeleteFile );
    ADD_TEST( Search );
    ADD_TEST( SearchAndSort );
    ADD_TEST( SearchAfterDelete );
    ADD_TEST( SearchAfterUpdate );
    ADD_TEST( Sort );
    ADD_TEST( AddDuplicate );
    ADD_TEST( SearchMedia );
    ADD_TEST( RemoveMedia );
    ADD_TEST( ClearContent );
    ADD_TEST( RemoveReAddMedia );
    ADD_TEST( InsertRemoveDuplicateMedia );
    ADD_TEST( AutoRemoveTask );
    ADD_TEST( CheckDbModel );
    ADD_TEST( IsReadOnly );
    ADD_TEST( SortByCreationDate );
    ADD_TEST( NbMedia );
    ADD_TEST( Duration );
    ADD_TEST( UpdateNbMediaOnInsertAndDelete );
    ADD_TEST( UpdateNbMediaOnMediaRemoval );
    ADD_TEST( UpdateNbMediaOnMediaTypeChange );
    ADD_TEST( FilterByMediaType );

    END_TESTS
}
