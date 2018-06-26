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
    ASSERT_EQ( s, s2 );
}

TEST_F( Shows, Fetch )
{
    auto s = ml->createShow( "show" );

    // Clear the cache
    Reload();

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

    Reload();

    auto s2 = ml->show( s->id() );
    ASSERT_EQ( s->releaseDate(), s2->releaseDate() );
}

TEST_F( Shows, SetShortSummary )
{
    auto s = ml->createShow( "show" );

    s->setShortSummary( "summary" );
    ASSERT_EQ( s->shortSummary(), "summary" );


    Reload();

    auto s2 = ml->show( s->id() );
    ASSERT_EQ( s->shortSummary(), s2->shortSummary() );
}

TEST_F( Shows, SetArtworkMrl )
{
    auto s = ml->createShow( "show" );

    s->setArtworkMrl( "artwork" );
    ASSERT_EQ( s->artworkMrl(), "artwork" );

    Reload();

    auto s2 = ml->show( s->id() );
    ASSERT_EQ( s->artworkMrl(), s2->artworkMrl() );
}

TEST_F( Shows, SetTvdbId )
{
    auto s = ml->createShow( "show" );

    s->setTvdbId( "TVDBID" );
    ASSERT_EQ( s->tvdbId(), "TVDBID" );

    Reload();

    auto s2 = ml->show( s->id() );
    ASSERT_EQ( s->tvdbId(), s2->tvdbId() );
}

////////////////////////////////////////////////////
// Episodes:
////////////////////////////////////////////////////

TEST_F( Shows, AddEpisode )
{
    auto show = ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "episode.avi" ) );
    auto e = show->addEpisode( *media, 1 );
    ASSERT_NE( e, nullptr );

    ASSERT_EQ( e->episodeNumber(), 1u );
    ASSERT_EQ( e->show(), show );

    auto episodes = show->episodes()->all();
    ASSERT_EQ( episodes.size(), 1u );
    ASSERT_EQ( episodes[0], e );
}

TEST_F( Shows, FetchShowFromEpisode )
{
    auto s = ml->createShow( "show" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.avi" ) );
    auto e = s->addEpisode( *f, 1 );
    f->save();

    auto e2 = f->showEpisode();
    auto s2 = e2->show();
    ASSERT_NE( s2, nullptr );
    ASSERT_EQ( s, s2 );

    Reload();

    f = std::static_pointer_cast<Media>( ml->media( f->id() ) );
    ASSERT_NE( nullptr, f->showEpisode() );
    s2 = f->showEpisode()->show();
    ASSERT_NE( s2, nullptr );
    ASSERT_EQ( s->name(), s2->name() );
}

TEST_F( Shows, SetEpisodeSeasonNumber )
{
    auto show = ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "episode.mkv" ) );
    auto e = show->addEpisode( *media, 1 );
    bool res = e->setSeasonNumber( 42 );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->seasonNumber(), 42u );

    Reload();

    show = std::static_pointer_cast<Show>( ml->show( show->id() ) );
    auto episodes = show->episodes()->all();
    ASSERT_EQ( episodes[0]->seasonNumber(), e->seasonNumber() );
}

TEST_F( Shows, SetEpisodeSummary )
{
    auto show = ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "episode.mkv" ) );
    auto e = show->addEpisode( *media, 1 );
    bool res = e->setShortSummary( "Insert spoilers here" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->shortSummary(), "Insert spoilers here" );

    Reload();

    show = std::static_pointer_cast<Show>( ml->show( show->id() ) );
    auto episodes = show->episodes()->all();
    ASSERT_EQ( episodes[0]->shortSummary(), e->shortSummary() );
}

TEST_F( Shows, SetEpisodeTvdbId )
{
    auto show = ml->createShow( "show" );
    auto media = std::static_pointer_cast<Media>( ml->addMedia( "episode.mkv" ) );
    auto e = show->addEpisode( *media, 1 );
    bool res = e->setTvdbId( "TVDBID" );
    ASSERT_TRUE( res );
    ASSERT_EQ( e->tvdbId(), "TVDBID" );

    Reload();

    show = std::static_pointer_cast<Show>( ml->show( show->id() ) );
    auto episodes = show->episodes()->all();
    ASSERT_EQ( episodes[0]->tvdbId(), e->tvdbId() );
}

TEST_F( Shows, ListAll )
{
    auto show1 = ml->createShow( "aaaa" );
    auto show2 = ml->createShow( "zzzz" );
    auto show3 = ml->createShow( "pppp" );

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
}

////////////////////////////////////////////////////
// Files links:
////////////////////////////////////////////////////

TEST_F( Shows, FileSetShowEpisode )
{
    auto show = ml->createShow( "show" );
    auto f = std::static_pointer_cast<Media>( ml->addMedia( "file.avi" ) );
    ASSERT_EQ( f->showEpisode(), nullptr );

    auto e = show->addEpisode( *f, 1 );

    ASSERT_EQ( f->showEpisode(), e );

    Reload();

    f = ml->media( f->id() );
    auto e2 = f->showEpisode();
    ASSERT_NE( e2, nullptr );
}

