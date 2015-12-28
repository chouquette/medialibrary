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

class Medias : public Tests
{
};


TEST_F( Medias, Init )
{
    // only test for correct test fixture behavior
}

TEST_F( Medias, Create )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_NE( f, nullptr );

    ASSERT_EQ( f->playCount(), 0 );
    ASSERT_EQ( f->albumTrack(), nullptr );
    ASSERT_EQ( f->showEpisode(), nullptr );
    ASSERT_TRUE( f->isStandAlone() );
    ASSERT_FALSE( f->isParsed() );
    ASSERT_EQ( f->duration(), -1 );
    ASSERT_NE( 0u, f->insertionDate() );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 1u );
    ASSERT_EQ( files[0]->mrl(), f->mrl() );
}

TEST_F( Medias, Fetch )
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

TEST_F( Medias, Delete )
{
    auto f = ml->addFile( "media.avi", nullptr );
    auto f2 = ml->file( "media.avi" );

    ASSERT_EQ( f, f2 );

    ml->deleteFile( f.get() );
    f2 = ml->file( "media.avi" );
    ASSERT_EQ( f2, nullptr );
}

TEST_F( Medias, LastModificationDate )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_NE( 0u, f->lastModificationDate() );

    Reload();
    auto f2 = std::static_pointer_cast<Media>( ml->file( "media.avi" ) );
    ASSERT_EQ( f->lastModificationDate(), f2->lastModificationDate() );
}

TEST_F( Medias, Duration )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_EQ( f->duration(), -1 );

    // Use a value that checks we're using a 64bits value
    int64_t d = int64_t(1) << 40;

    f->setDuration( d );
    f->save();
    ASSERT_EQ( f->duration(), d );

    Reload();

    auto f2 = ml->file( "media.avi" );
    ASSERT_EQ( f2->duration(), d );
}


TEST_F( Medias, Artist )
{
    auto f = std::static_pointer_cast<Media>( ml->addFile( "media.avi", nullptr ) );
    ASSERT_EQ( f->artist(), "" );

    std::string newArtist( "Rage Against The Otters" );

    f->setArtist( newArtist );
    f->save();
    ASSERT_EQ( f->artist(), newArtist );

    Reload();

    auto f2 = ml->file( "media.avi" );
    ASSERT_EQ( f2->artist(), newArtist );
}

TEST_F( Medias, Snapshot )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_EQ( f->snapshot(), "" );

    std::string newSnapshot( "/path/to/snapshot" );

    f->setSnapshot( newSnapshot );
    f->save();
    ASSERT_EQ( f->snapshot(), newSnapshot );

    Reload();

    auto f2 = ml->file( "media.avi" );
    ASSERT_EQ( f2->snapshot(), newSnapshot );
}

TEST_F( Medias, PlayCount )
{
    auto f = ml->addFile( "media.avi", nullptr );
    ASSERT_EQ( 0, f->playCount() );
    f->increasePlayCount();
    ASSERT_EQ( 1, f->playCount() );
    f->save();

    Reload();

    f = std::static_pointer_cast<Media>( ml->file( "media.avi" ) );
    ASSERT_EQ( 1, f->playCount() );
}
