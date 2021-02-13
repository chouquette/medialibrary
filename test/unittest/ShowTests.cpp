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

#include "Media.h"
#include "MediaLibrary.h"
#include "Show.h"
#include "ShowEpisode.h"

static void Create( Tests* T )
{
    auto s = T->ml->createShow( "show" );
    ASSERT_NE( s, nullptr );

    auto s2 = T->ml->show( s->id() );
    ASSERT_NE( nullptr, s2 );
}

static void Fetch( Tests* T )
{
    auto s = T->ml->createShow( "show" );

    auto s2 = T->ml->show( s->id() );
    // The shared pointers are expected to point to different instances
    ASSERT_NE( s, s2 );

    ASSERT_EQ( s->id(), s2->id() );
}

static void SetReleaseDate( Tests* T )
{
    auto s = T->ml->createShow( "show" );

    s->setReleaseDate( 1234 );
    ASSERT_EQ( s->releaseDate(), 1234 );

    auto s2 = T->ml->show( s->id() );
    ASSERT_EQ( s->releaseDate(), s2->releaseDate() );
}

static void SetShortSummary( Tests* T )
{
    auto s = T->ml->createShow( "show" );

    s->setShortSummary( "summary" );
    ASSERT_EQ( s->shortSummary(), "summary" );


    auto s2 = T->ml->show( s->id() );
    ASSERT_EQ( s->shortSummary(), s2->shortSummary() );
}

static void SetArtworkMrl( Tests* T )
{
    auto s = T->ml->createShow( "show" );

    s->setArtworkMrl( "artwork" );
    ASSERT_EQ( s->artworkMrl(), "artwork" );

    auto s2 = T->ml->show( s->id() );
    ASSERT_EQ( s->artworkMrl(), s2->artworkMrl() );
}

static void SetTvdbId( Tests* T )
{
    auto s = T->ml->createShow( "show" );

    s->setTvdbId( "TVDBID" );
    ASSERT_EQ( s->tvdbId(), "TVDBID" );

    auto s2 = T->ml->show( s->id() );
    ASSERT_EQ( s->tvdbId(), s2->tvdbId() );
}

////////////////////////////////////////////////////
// Episodes:
////////////////////////////////////////////////////

static void AddEpisode( Tests* T )
{
    auto show = T->ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( T->ml->addMedia( "episode.avi", IMedia::Type::Video ) );
    auto e = show->addEpisode( *media, 1, 1, "episode title" );
    media->save();
    ASSERT_NE( e, nullptr );

    ASSERT_EQ( e->episodeId(), 1u );
    ASSERT_EQ( 1u, e->seasonId() );
    ASSERT_EQ( e->show()->id(), show->id() );
    ASSERT_EQ( "episode title", e->title() );
    ASSERT_EQ( "episode.avi", media->title() );

    auto episodes = show->episodes( nullptr )->all();
    ASSERT_EQ( episodes.size(), 1u );
    ASSERT_EQ( episodes[0]->showEpisode()->id(), e->id() );
}

static void FetchShowFromEpisode( Tests* T )
{
    auto s = T->ml->createShow( "show" );
    auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "file.avi", IMedia::Type::Video ) );
    auto e = s->addEpisode( *f, 1, 1, "episode title" );
    f->save();

    auto e2 = f->showEpisode();
    auto s2 = e2->show();
    ASSERT_NE( s2, nullptr );
    ASSERT_EQ( s->id(), s2->id() );

    f = std::static_pointer_cast<Media>( T->ml->media( f->id() ) );
    ASSERT_NE( nullptr, f->showEpisode() );
    s2 = f->showEpisode()->show();
    ASSERT_NE( s2, nullptr );
    ASSERT_EQ( s->title(), s2->title() );
}

static void SetEpisodeSummary( Tests* T )
{
    auto show = T->ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( T->ml->addMedia( "episode.mkv", IMedia::Type::Video ) );
    auto e = show->addEpisode( *media, 1, 1, "episode title" );
    media->save();
    bool res = e->setShortSummary( "Insert spoilers here" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->shortSummary(), "Insert spoilers here" );

    show = std::static_pointer_cast<Show>( T->ml->show( show->id() ) );
    auto episodes = show->episodes( nullptr )->all();
    ASSERT_EQ( episodes[0]->showEpisode()->shortSummary(), e->shortSummary() );
}

static void SetEpisodeTvdbId( Tests* T )
{
    auto show = T->ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( T->ml->addMedia( "episode.mkv", IMedia::Type::Video ) );
    auto e = show->addEpisode( *media, 1, 1, "episode title" );
    media->save();
    bool res = e->setTvdbId( "TVDBID" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->tvdbId(), "TVDBID" );

    show = std::static_pointer_cast<Show>( T->ml->show( show->id() ) );
    auto episodes = show->episodes( nullptr )->all();
    ASSERT_EQ( episodes[0]->showEpisode()->tvdbId(), e->tvdbId() );
}

static void ListAll( Tests* T )
{
    auto show1 = T->ml->createShow( "aaaa" );
    auto media1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    show1->setReleaseDate( 5 );
    media1->save();
    auto show2 = T->ml->createShow( "zzzz" );
    auto media2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    show2->addEpisode( *media2, 1, 1, "episode title" );
    show2->setReleaseDate( 1 );
    media2->save();
    auto show3 = T->ml->createShow( "pppp" );
    auto media3 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media3.mkv", IMedia::Type::Video ) );
    show3->addEpisode( *media3, 1, 1, "episode title" );
    show3->setReleaseDate( 10 );
    media3->save();

    auto shows = T->ml->shows( nullptr )->all();
    ASSERT_EQ( 3u, shows.size() );
    ASSERT_EQ( show1->id(), shows[0]->id() );
    ASSERT_EQ( show3->id(), shows[1]->id() );
    ASSERT_EQ( show2->id(), shows[2]->id() );

    medialibrary::QueryParameters params { SortingCriteria::Alpha, true };
    shows = T->ml->shows( &params )->all();
    ASSERT_EQ( 3u, shows.size() );
    ASSERT_EQ( show2->id(), shows[0]->id() );
    ASSERT_EQ( show3->id(), shows[1]->id() );
    ASSERT_EQ( show1->id(), shows[2]->id() );

    params.sort = SortingCriteria::ReleaseDate;
    params.desc = false;
    shows = T->ml->shows( &params )->all();
    ASSERT_EQ( 3u, shows.size() );
    ASSERT_EQ( show2->id(), shows[0]->id() );
    ASSERT_EQ( show1->id(), shows[1]->id() );
    ASSERT_EQ( show3->id(), shows[2]->id() );
}

static void ListEpisodes( Tests* T )
{
    auto show = T->ml->createShow( "show" );
    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "episode1.avi", IMedia::Type::Video ) );
    auto s02e01 = show->addEpisode( *m1, 2, 1, "episode title" );
    m1->save();

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "episode2.avi", IMedia::Type::Video ) );
    auto s01e01 = show->addEpisode( *m2, 1, 1, "episode title" );
    m2->save();

    auto m3 = std::static_pointer_cast<Media>( T->ml->addMedia( "episode3.avi", IMedia::Type::Video ) );
    auto s01e02 = show->addEpisode( *m3, 1, 2, "episode title" );
    m3->save();

    auto episodes = show->episodes( nullptr )->all();
    ASSERT_EQ( 3u, episodes.size() );
    ASSERT_EQ( s01e01->id(), episodes[0]->id() );
    ASSERT_EQ( s01e02->id(), episodes[1]->id() );
    ASSERT_EQ( s02e01->id(), episodes[2]->id() );

    QueryParameters params { SortingCriteria::Default, true };
    episodes = show->episodes( &params )->all();
    ASSERT_EQ( 3u, episodes.size() );
    ASSERT_EQ( s02e01->id(), episodes[0]->id() );
    ASSERT_EQ( s01e02->id(), episodes[1]->id() );
    ASSERT_EQ( s01e01->id(), episodes[2]->id() );
}

static void Search( Tests* T )
{
    auto show1 = T->ml->createShow( "Cute fluffy sea otters" );
    auto media1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    show1->setReleaseDate( 10 );
    media1->save();
    auto show2 = T->ml->createShow( "Less cute less fluffy naked mole rats" );
    auto media2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    show2->addEpisode( *media2, 1, 1, "episode title" );
    show2->setReleaseDate( 100 );
    media2->save();

    auto query = T->ml->searchShows( "", nullptr );
    ASSERT_EQ( nullptr, query );

    auto shows = T->ml->searchShows( "otters" )->all();
    ASSERT_EQ( 1u, shows.size() );
    ASSERT_EQ( show1->id(), shows[0]->id() );

    QueryParameters params = { SortingCriteria::ReleaseDate, true };
    shows = T->ml->searchShows( "fluffy", &params )->all();
    ASSERT_EQ( 2u, shows.size() );
    ASSERT_EQ( show2->id(), shows[0]->id() );
    ASSERT_EQ( show1->id(), shows[1]->id() );
}

static void RemoveFromFts( Tests* T )
{
    auto show1 = T->ml->createShow( "The otters show" );
    auto media1 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    media1->save();

    auto shows = T->ml->searchShows( "otters" )->all();
    ASSERT_EQ( 1u, shows.size() );

    T->ml->deleteShow( show1->id() );

    shows = T->ml->searchShows( "otters" )->all();
    ASSERT_EQ( 0u, shows.size() );
}

////////////////////////////////////////////////////
// Files links:
////////////////////////////////////////////////////

static void FileSetShowEpisode( Tests* T )
{
    auto show = T->ml->createShow( "show" );
    auto f = std::static_pointer_cast<Media>( T->ml->addMedia( "file.avi", IMedia::Type::Video ) );
    ASSERT_EQ( f->showEpisode(), nullptr );

    auto e = show->addEpisode( *f, 1, 1, "episode title" );
    f->save();

    ASSERT_EQ( f->showEpisode(), e );

    f = T->ml->media( f->id() );
    auto e2 = f->showEpisode();
    ASSERT_NE( e2, nullptr );
}

static void SearchEpisodes( Tests* T )
{
    auto show1 = T->ml->createShow( "Show1" );
    auto show2 = T->ml->createShow( "show2" );

    auto m1 = std::static_pointer_cast<Media>( T->ml->addMedia( "episode.mkv", IMedia::Type::Video ) );
    m1->setTitleBuffered( "cute otters" );
    // will save the media in db
    auto ep1 = show1->addEpisode( *m1, 1, 1, "episode title" );
    m1->save();

    auto m2 = std::static_pointer_cast<Media>( T->ml->addMedia( "other episode.mkv", IMedia::Type::Video ) );
    m2->setTitleBuffered( "fluffy otters" );
    auto ep2 = show2->addEpisode( *m2, 1, 1, "episode title" );
    m2->save();

    auto episodes = T->ml->searchVideo( "otters", nullptr )->all();
    ASSERT_EQ( 2u, episodes.size() );

    episodes = show1->searchEpisodes( "otters", nullptr )->all();
    ASSERT_EQ( 1u, episodes.size() );
    ASSERT_EQ( m1->id(), episodes[0]->id() );
}

static void CheckDbModel( Tests* T )
{
    auto res = Show::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void CheckShowEpisodeDbModel( Tests* T )
{
    auto res = ShowEpisode::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void NbEpisodes( Tests* T )
{
    auto show = T->ml->createShow( "The Otters Show" );
    ASSERT_EQ( 0u, show->nbEpisodes() );

    auto media = std::static_pointer_cast<Media>( T->ml->addMedia( "Fluffy otters.mkv",
                                                                IMedia::Type::Video ) );
    show->addEpisode( *media, 1, 1, "episode title" );
    media->save();
    ASSERT_EQ( 1u, show->nbEpisodes() );

    show = std::static_pointer_cast<Show>( T->ml->show( show->id() ) );
    ASSERT_EQ( 1u, show->nbEpisodes() );

    auto media2 = std::static_pointer_cast<Media>( T->ml->addMedia( "Juggling otters.mkv",
                                                                 IMedia::Type::Video ) );
    show->addEpisode( *media2, 1, 2, "episode title" );
    media2->save();
    ASSERT_EQ( 2u, show->nbEpisodes() );

    show = std::static_pointer_cast<Show>( T->ml->show( show->id() ) );
    ASSERT_EQ( 2u, show->nbEpisodes() );

    T->ml->deleteMedia( media->id() );
    T->ml->deleteMedia( media2->id() );

    show = std::static_pointer_cast<Show>( T->ml->show( show->id() ) );
    ASSERT_EQ( 0u, show->nbEpisodes() );
}

static void DeleteEpisodeByMediaId( Tests* T )
{
    auto show = T->ml->createShow( "The Otters Gambit" );
    ASSERT_NE( nullptr, show );
    auto media = std::static_pointer_cast<Media>(
                T->ml->addMedia( "Openings.mkv", IMedia::Type::Video ) );
    auto media2 = std::static_pointer_cast<Media>(
                T->ml->addMedia( "End game.mkv", IMedia::Type::Video ) );
    ASSERT_NE( nullptr, media );

    auto showEpisode = show->addEpisode( *media, 1, 1, "Openings" );
    ASSERT_NE( nullptr, showEpisode );
    auto showEpisode2 = show->addEpisode( *media2, 1, 6, "End game" );
    ASSERT_NE( nullptr, showEpisode2 );

    auto episodes = show->episodes( nullptr )->all();
    ASSERT_EQ( 2u, episodes.size() );

    auto res = ShowEpisode::deleteByMediaId( T->ml.get(), media->id() );
    ASSERT_TRUE( res );

    episodes = show->episodes( nullptr )->all();
    ASSERT_EQ( 1u, episodes.size() );
    show = std::static_pointer_cast<Show>( T->ml->show( show->id() ) );
    ASSERT_EQ( 1u, show->nbEpisodes() );

    res = ShowEpisode::deleteByMediaId( T->ml.get(), media2->id() );
    ASSERT_TRUE( res );

    episodes = show->episodes( nullptr )->all();
    ASSERT_EQ( 0u, episodes.size() );
    show = std::static_pointer_cast<Show>( T->ml->show( show->id() ) );
    ASSERT_EQ( 0u, show->nbEpisodes() );
}

int main( int ac, char** av )
{
    INIT_TESTS;

    ADD_TEST( Create );
    ADD_TEST( Fetch );
    ADD_TEST( SetReleaseDate );
    ADD_TEST( SetShortSummary );
    ADD_TEST( SetArtworkMrl );
    ADD_TEST( SetTvdbId );
    ADD_TEST( AddEpisode );
    ADD_TEST( FetchShowFromEpisode );
    ADD_TEST( SetEpisodeSummary );
    ADD_TEST( SetEpisodeTvdbId );
    ADD_TEST( ListAll );
    ADD_TEST( ListEpisodes );
    ADD_TEST( Search );
    ADD_TEST( RemoveFromFts );
    ADD_TEST( FileSetShowEpisode );
    ADD_TEST( SearchEpisodes );
    ADD_TEST( CheckDbModel );
    ADD_TEST( CheckShowEpisodeDbModel );
    ADD_TEST( NbEpisodes );
    ADD_TEST( DeleteEpisodeByMediaId );

    END_TESTS;
}
