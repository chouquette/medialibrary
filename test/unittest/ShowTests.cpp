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

#include "Media.h"
#include "MediaLibrary.h"
#include "Show.h"
#include "ShowEpisode.h"

class Shows : public Tests
{
};

TEST_F( Shows, Create )
{
    auto s = ml->createShow( "show" );
    ASSERT_NE( s, nullptr );

    auto s2 = ml->show( s->id() );
    ASSERT_NE( nullptr, s2 );
}

TEST_F( Shows, Fetch )
{
    auto s = ml->createShow( "show" );

    auto s2 = ml->show( s->id() );
    // The shared pointers are expected to point to different instances
    ASSERT_NE( s, s2 );

    ASSERT_EQ( s->id(), s2->id() );
}

TEST_F( Shows, SetReleaseDate )
{
    auto s = ml->createShow( "show" );

    s->setReleaseDate( 1234 );
    ASSERT_EQ( s->releaseDate(), 1234 );

    auto s2 = ml->show( s->id() );
    ASSERT_EQ( s->releaseDate(), s2->releaseDate() );
}

TEST_F( Shows, SetShortSummary )
{
    auto s = ml->createShow( "show" );

    s->setShortSummary( "summary" );
    ASSERT_EQ( s->shortSummary(), "summary" );


    auto s2 = ml->show( s->id() );
    ASSERT_EQ( s->shortSummary(), s2->shortSummary() );
}

TEST_F( Shows, SetArtworkMrl )
{
    auto s = ml->createShow( "show" );

    s->setArtworkMrl( "artwork" );
    ASSERT_EQ( s->artworkMrl(), "artwork" );

    auto s2 = ml->show( s->id() );
    ASSERT_EQ( s->artworkMrl(), s2->artworkMrl() );
}

TEST_F( Shows, SetTvdbId )
{
    auto s = ml->createShow( "show" );

    s->setTvdbId( "TVDBID" );
    ASSERT_EQ( s->tvdbId(), "TVDBID" );

    auto s2 = ml->show( s->id() );
    ASSERT_EQ( s->tvdbId(), s2->tvdbId() );
}

////////////////////////////////////////////////////
// Episodes:
////////////////////////////////////////////////////

TEST_F( Shows, AddEpisode )
{
    auto show = ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "episode.avi", IMedia::Type::Video ) );
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

TEST_F( Shows, FetchShowFromEpisode )
{
    auto s = ml->createShow( "show" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.avi", IMedia::Type::Video ) );
    auto e = s->addEpisode( *f, 1, 1, "episode title" );
    f->save();

    auto e2 = f->showEpisode();
    auto s2 = e2->show();
    ASSERT_NE( s2, nullptr );
    ASSERT_EQ( s->id(), s2->id() );

    f = std::static_pointer_cast<Media>( ml->media( f->id() ) );
    ASSERT_NE( nullptr, f->showEpisode() );
    s2 = f->showEpisode()->show();
    ASSERT_NE( s2, nullptr );
    ASSERT_EQ( s->title(), s2->title() );
}

TEST_F( Shows, SetEpisodeSummary )
{
    auto show = ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "episode.mkv", IMedia::Type::Video ) );
    auto e = show->addEpisode( *media, 1, 1, "episode title" );
    media->save();
    bool res = e->setShortSummary( "Insert spoilers here" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->shortSummary(), "Insert spoilers here" );

    show = std::static_pointer_cast<Show>( ml->show( show->id() ) );
    auto episodes = show->episodes( nullptr )->all();
    ASSERT_EQ( episodes[0]->showEpisode()->shortSummary(), e->shortSummary() );
}

TEST_F( Shows, SetEpisodeTvdbId )
{
    auto show = ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "episode.mkv", IMedia::Type::Video ) );
    auto e = show->addEpisode( *media, 1, 1, "episode title" );
    media->save();
    bool res = e->setTvdbId( "TVDBID" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->tvdbId(), "TVDBID" );

    show = std::static_pointer_cast<Show>( ml->show( show->id() ) );
    auto episodes = show->episodes( nullptr )->all();
    ASSERT_EQ( episodes[0]->showEpisode()->tvdbId(), e->tvdbId() );
}

TEST_F( Shows, ListAll )
{
    auto show1 = ml->createShow( "aaaa" );
    auto media1 = std::static_pointer_cast<Media>(
                ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    show1->setReleaseDate( 5 );
    media1->save();
    auto show2 = ml->createShow( "zzzz" );
    auto media2 = std::static_pointer_cast<Media>(
                ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    show2->addEpisode( *media2, 1, 1, "episode title" );
    show2->setReleaseDate( 1 );
    media2->save();
    auto show3 = ml->createShow( "pppp" );
    auto media3 = std::static_pointer_cast<Media>(
                ml->addMedia( "media3.mkv", IMedia::Type::Video ) );
    show3->addEpisode( *media3, 1, 1, "episode title" );
    show3->setReleaseDate( 10 );
    media3->save();

    auto shows = ml->shows( nullptr )->all();
    ASSERT_EQ( 3u, shows.size() );
    ASSERT_EQ( show1->id(), shows[0]->id() );
    ASSERT_EQ( show3->id(), shows[1]->id() );
    ASSERT_EQ( show2->id(), shows[2]->id() );

    medialibrary::QueryParameters params { SortingCriteria::Alpha, true };
    shows = ml->shows( &params )->all();
    ASSERT_EQ( 3u, shows.size() );
    ASSERT_EQ( show2->id(), shows[0]->id() );
    ASSERT_EQ( show3->id(), shows[1]->id() );
    ASSERT_EQ( show1->id(), shows[2]->id() );

    params.sort = SortingCriteria::ReleaseDate;
    params.desc = false;
    shows = ml->shows( &params )->all();
    ASSERT_EQ( 3u, shows.size() );
    ASSERT_EQ( show2->id(), shows[0]->id() );
    ASSERT_EQ( show1->id(), shows[1]->id() );
    ASSERT_EQ( show3->id(), shows[2]->id() );
}

TEST_F( Shows, ListEpisodes )
{
    auto show = ml->createShow( "show" );
    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "episode1.avi", IMedia::Type::Video ) );
    auto s02e01 = show->addEpisode( *m1, 2, 1, "episode title" );
    m1->save();

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "episode2.avi", IMedia::Type::Video ) );
    auto s01e01 = show->addEpisode( *m2, 1, 1, "episode title" );
    m2->save();

    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "episode3.avi", IMedia::Type::Video ) );
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

TEST_F( Shows, Search )
{
    auto show1 = ml->createShow( "Cute fluffy sea otters" );
    auto media1 = std::static_pointer_cast<Media>(
                ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    show1->setReleaseDate( 10 );
    media1->save();
    auto show2 = ml->createShow( "Less cute less fluffy naked mole rats" );
    auto media2 = std::static_pointer_cast<Media>(
                ml->addMedia( "media2.mkv", IMedia::Type::Video ) );
    show2->addEpisode( *media2, 1, 1, "episode title" );
    show2->setReleaseDate( 100 );
    media2->save();

    auto shows = ml->searchShows( "otters" )->all();
    ASSERT_EQ( 1u, shows.size() );
    ASSERT_EQ( show1->id(), shows[0]->id() );

    QueryParameters params = { SortingCriteria::ReleaseDate, true };
    shows = ml->searchShows( "fluffy", &params )->all();
    ASSERT_EQ( 2u, shows.size() );
    ASSERT_EQ( show2->id(), shows[0]->id() );
    ASSERT_EQ( show1->id(), shows[1]->id() );
}

TEST_F( Shows, RemoveFromFts )
{
    auto show1 = ml->createShow( "The otters show" );
    auto media1 = std::static_pointer_cast<Media>(
                ml->addMedia( "media1.mkv", IMedia::Type::Video ) );
    show1->addEpisode( *media1, 1, 1, "episode title" );
    media1->save();

    auto shows = ml->searchShows( "otters" )->all();
    ASSERT_EQ( 1u, shows.size() );

    ml->deleteShow( show1->id() );

    shows = ml->searchShows( "otters" )->all();
    ASSERT_EQ( 0u, shows.size() );
}

////////////////////////////////////////////////////
// Files links:
////////////////////////////////////////////////////

TEST_F( Shows, FileSetShowEpisode )
{
    auto show = ml->createShow( "show" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.avi", IMedia::Type::Video ) );
    ASSERT_EQ( f->showEpisode(), nullptr );

    auto e = show->addEpisode( *f, 1, 1, "episode title" );
    f->save();

    ASSERT_EQ( f->showEpisode(), e );

    f = ml->media( f->id() );
    auto e2 = f->showEpisode();
    ASSERT_NE( e2, nullptr );
}

TEST_F( Shows, SearchEpisodes )
{
    auto show1 = ml->createShow( "Show1" );
    auto show2 = ml->createShow( "show2" );

    auto m1 = std::static_pointer_cast<Media>( ml->addMedia( "episode.mkv", IMedia::Type::Video ) );
    m1->setTitleBuffered( "cute otters" );
    // will save the media in db
    auto ep1 = show1->addEpisode( *m1, 1, 1, "episode title" );
    m1->save();

    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "other episode.mkv", IMedia::Type::Video ) );
    m2->setTitleBuffered( "fluffy otters" );
    auto ep2 = show2->addEpisode( *m2, 1, 1, "episode title" );
    m2->save();

    auto episodes = ml->searchVideo( "otters", nullptr )->all();
    ASSERT_EQ( 2u, episodes.size() );

    episodes = show1->searchEpisodes( "otters", nullptr )->all();
    ASSERT_EQ( 1u, episodes.size() );
    ASSERT_EQ( m1->id(), episodes[0]->id() );
}

TEST_F( Shows, CheckDbModel )
{
    auto res = Show::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( Shows, CheckShowEpisodeDbModel )
{
    auto res = ShowEpisode::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( Shows, NbEpisodes )
{
    auto show = ml->createShow( "The Otters Show" );
    ASSERT_EQ( 0u, show->nbEpisodes() );

    auto media = std::static_pointer_cast<Media>( ml->addMedia( "Fluffy otters.mkv",
                                                                IMedia::Type::Video ) );
    show->addEpisode( *media, 1, 1, "episode title" );
    media->save();
    ASSERT_EQ( 1u, show->nbEpisodes() );

    show = std::static_pointer_cast<Show>( ml->show( show->id() ) );
    ASSERT_EQ( 1u, show->nbEpisodes() );

    auto media2 = std::static_pointer_cast<Media>( ml->addMedia( "Juggling otters.mkv",
                                                                 IMedia::Type::Video ) );
    show->addEpisode( *media2, 1, 2, "episode title" );
    media2->save();
    ASSERT_EQ( 2u, show->nbEpisodes() );

    show = std::static_pointer_cast<Show>( ml->show( show->id() ) );
    ASSERT_EQ( 2u, show->nbEpisodes() );

    ml->deleteMedia( media->id() );
    ml->deleteMedia( media2->id() );

    show = std::static_pointer_cast<Show>( ml->show( show->id() ) );
    ASSERT_EQ( 0u, show->nbEpisodes() );
}
