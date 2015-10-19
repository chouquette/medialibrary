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

#include "Tests.h"

#include "IMediaLibrary.h"
#include "Media.h"
#include "Artist.h"
#include "Album.h"
#include "AlbumTrack.h"

class Files : public Tests
{
};


TEST_F( Files, Init )
{
    // only test for correct test fixture behavior
}

TEST_F( Files, Create )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_NE( f, nullptr );

    ASSERT_EQ( f->playCount(), 0 );
    ASSERT_EQ( f->albumTrack(), nullptr );
    ASSERT_EQ( f->showEpisode(), nullptr );
    ASSERT_TRUE( f->isStandAlone() );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 1u );
    ASSERT_EQ( files[0]->mrl(), f->mrl() );
}

TEST_F( Files, Fetch )
{
    auto f = ml->addFile( "media.avi", nullptr );
    auto f2 = std::static_pointer_cast<Media>( ml->file( "media.avi" ) );
    ASSERT_EQ( f->mrl(), f2->mrl() );
    ASSERT_EQ( f, f2 );

    // Flush cache and fetch from DB
    Reload();

    f2 = std::static_pointer_cast<Media>( ml->file( "media.avi" ) );
    ASSERT_EQ( f->mrl(), f2->mrl() );
    ASSERT_TRUE( f2->isStandAlone() );
}

TEST_F( Files, Delete )
{
    auto f = ml->addFile( "media.avi", nullptr );
    auto f2 = ml->file( "media.avi" );

    ASSERT_EQ( f, f2 );

    ml->deleteFile( f );
    f2 = ml->file( "media.avi" );
    ASSERT_EQ( f2, nullptr );
}

TEST_F( Files, Duplicate )
{
    auto f = ml->addFile( "media.avi", nullptr );
    auto f2 = std::static_pointer_cast<Media>( ml->addFile( "media.avi", nullptr ) );

    ASSERT_NE( f, nullptr );
    ASSERT_EQ( f2, nullptr );

    f2 = std::static_pointer_cast<Media>( ml->file( "media.avi" ) );
    ASSERT_EQ( f, f2 );
}

TEST_F( Files, LastModificationDate )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_NE( 0u, f->lastModificationDate() );

    Reload();
    auto f2 = std::static_pointer_cast<Media>( ml->file( "media.avi" ) );
    ASSERT_EQ( f->lastModificationDate(), f2->lastModificationDate() );
}

TEST_F( Files, Duration )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_EQ( f->duration(), -1 );

    // Use a value that checks we're using a 64bits value
    int64_t d = int64_t(1) << 40;

    f->setDuration( d );
    ASSERT_EQ( f->duration(), d );

    Reload();

    auto f2 = ml->file( "media.avi" );
    ASSERT_EQ( f2->duration(), d );
}

TEST_F( Files, Snapshot )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_EQ( f->snapshot(), "" );

    std::string newSnapshot( "/path/to/snapshot" );

    f->setSnapshot( newSnapshot );
    ASSERT_EQ( f->snapshot(), newSnapshot );

    Reload();

    auto f2 = ml->file( "media.avi" );
    ASSERT_EQ( f2->snapshot(), newSnapshot );
}

TEST_F( Files, UnknownArtist )
{
    // If no song has been added, unknown artist should be null.
    auto a = ml->unknownArtist();
    ASSERT_EQ( a, nullptr );

    auto f = ml->addFile( "file.mp3", nullptr );
    // explicitely set the artist to nullptr (aka "unknown artist")
    auto res = f->addArtist( nullptr );
    ASSERT_EQ( res, true );

    // Now, querying unknownArtist should give out some results.
    a = ml->unknownArtist();
    ASSERT_NE( a, nullptr );
    auto tracks = a->media();
    ASSERT_EQ( tracks.size(), 1u );

    Reload();

    // Check that unknown artist tracks listing persists in DB
    auto a2 = ml->unknownArtist();
    ASSERT_NE( a2, nullptr );
    auto tracks2 = a2->media();
    ASSERT_EQ( tracks2.size(), 1u );
}

TEST_F( Files, Artists )
{
    auto artist1 = ml->createArtist( "artist 1" );
    auto artist2 = ml->createArtist( "artist 2" );
    auto album = ml->createAlbum( "album" );

    ASSERT_NE( artist1, nullptr );
    ASSERT_NE( artist2, nullptr );

    for ( auto i = 1; i <= 3; i++ )
    {
        std::string name = "track" + std::to_string(i) + ".mp3";
        auto f = ml->addFile( name, nullptr );
        ASSERT_NE( f, nullptr );
        auto t = album->addTrack( f, i );
        ASSERT_NE( t, nullptr );
    }

    auto tracks = album->tracks();
    for ( auto& it : tracks )
    {
        //FIXME: This should return a std::vector<IMedia>
        auto t = std::static_pointer_cast<AlbumTrack>( it );
        ASSERT_NE( t->files().size(), 0u );
        for ( auto& it : t->files() )
        {
            auto f = std::static_pointer_cast<Media>( it );
            auto res = f->addArtist( artist1 );
            ASSERT_EQ( res, true );
            res = f->addArtist( artist2 );
            ASSERT_EQ( res, true );
            auto artists = f->artists();
            ASSERT_EQ( artists.size(), 2u );
        }
    }

    auto artists = ml->artists();
    for ( auto& a : artists )
    {
        auto media = a->media();
        ASSERT_EQ( media.size(), 3u );
    }

    Reload();

    auto album2 = ml->album( "album" );
    auto tracks2 = album2->tracks();
    for ( auto& t : tracks2 )
    {
        for ( auto& f : t->files() )
        {
            auto artists = f->artists();
            ASSERT_EQ( artists.size(), 2u );
        }
    }

    auto artists2 = ml->artists();
    for ( auto& a : artists2 )
    {
        auto tracks = a->media();
        ASSERT_EQ( tracks.size(), 3u );
    }
}
