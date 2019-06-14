/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Thumbnail.h"
#include "Media.h"
#include "Artist.h"
#include "utils/Filename.h"

class Thumbnails : public Tests
{
};

TEST_F( Thumbnails, Create )
{
    std::string mrl = "file:///path/to/thumbnail.png";
    auto t = Thumbnail::create( ml.get(), mrl, Thumbnail::Origin::UserProvided, false );
    ASSERT_NE( t, nullptr );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );
}

TEST_F( Thumbnails, MediaSetThumbnail )
{
    std::string mrl = "file:///path/to/thumbnail.png";
    auto m = ml->addMedia( "/path/to/media.mp3" );
    ASSERT_FALSE( m->isThumbnailGenerated() );
    auto res = m->setThumbnail( mrl );
    ASSERT_TRUE( res );
    ASSERT_TRUE( m->isThumbnailGenerated() );
    ASSERT_EQ( m->thumbnailMrl(), mrl );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( m->thumbnailMrl(), mrl );
    ASSERT_TRUE( m->isThumbnailGenerated() );
}

TEST_F( Thumbnails, Update )
{
    std::string mrl = "file:///path/to/thumbnail.png";
    auto t = Thumbnail::create( ml.get(), mrl, Thumbnail::Origin::UserProvided, false );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );

    mrl = "file:///better/thumbnail.gif";
    auto res = t->update( mrl, Thumbnail::Origin::UserProvided, false );
    ASSERT_TRUE( res );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );

    Reload();

    t = Thumbnail::fetch( ml.get(), t->id() );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );

    res = t->update( mrl, Thumbnail::Origin::AlbumArtist, false );
    ASSERT_TRUE( res );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::AlbumArtist );

    Reload();

    t = Thumbnail::fetch( ml.get(), t->id() );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::AlbumArtist );
}

TEST_F( Thumbnails, MarkFailure )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv" ) );
    ASSERT_FALSE( m->isThumbnailGenerated() );
    auto res = m->setThumbnail( "", Thumbnail::Origin::Media, false );
    ASSERT_TRUE( res );

    ASSERT_TRUE( m->isThumbnailGenerated() );

    Reload();

    m = std::static_pointer_cast<Media>( ml->media( m->id() ) );
    ASSERT_TRUE( m->isThumbnailGenerated() );
    auto t = m->thumbnail();
    ASSERT_TRUE( t->isFailureRecord() );
}

TEST_F( Thumbnails, UnshareMedia )
{
    // Check that all thumbnails are shared, until we want to update the
    // shared version, in which case it should be duplicated

    auto t = Thumbnail::create( ml.get(), "file:///tmp/thumb.jpg", Thumbnail::Origin::CoverFile, false );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3" ) );
    auto a = ml->createArtist(  "artist" );

    m->setThumbnail( t );
    a->setThumbnail( t );

    auto thumbnails = Thumbnail::fetchAll( ml.get() );
    ASSERT_EQ( 1u, thumbnails.size() );

    auto artistThumbnail = a->thumbnail();
    auto mediaThumbnail = m->thumbnail();
    ASSERT_EQ( artistThumbnail->id(), mediaThumbnail->id() );
    ASSERT_EQ( artistThumbnail->id(), t->id() );

    // Both the artist and the media have the same thumbnail_id. Now change the
    // media thumbnail, and check that the artist still has the same thumbnail &
    // thumbnail id, while the media has its own thumbnail

    auto newThumbnail = Thumbnail::create( ml.get(), "file:///tmp/newthumb.jpg",
                                           Thumbnail::Origin::UserProvided, false );
    m->setThumbnail( newThumbnail );
    thumbnails = Thumbnail::fetchAll( ml.get() );
    ASSERT_EQ( 2u, thumbnails.size() );

    artistThumbnail = a->thumbnail();
    mediaThumbnail = m->thumbnail();
    ASSERT_NE( nullptr, artistThumbnail );
    ASSERT_EQ( t->mrl(), artistThumbnail->mrl() );
    ASSERT_EQ( t->id(), artistThumbnail->id() );

    ASSERT_NE( nullptr, mediaThumbnail );
    ASSERT_EQ( newThumbnail->mrl(), mediaThumbnail->mrl() );

    ASSERT_NE( artistThumbnail->id(), mediaThumbnail->id() );

    // Now let's re-update the media thumbnail, and check that it was only updated
    auto newMrl = std::string{ "file:///tmp/super_duper_new_thumbnail.png" };
    auto res = m->setThumbnail( newMrl, Thumbnail::Origin::UserProvided, false );
    ASSERT_TRUE( res );

    thumbnails = Thumbnail::fetchAll( ml.get() );
    ASSERT_EQ( 2u, thumbnails.size() );

    auto newMediaThumbnail = m->thumbnail();
    ASSERT_EQ( mediaThumbnail->id(), newMediaThumbnail->id() );
    ASSERT_EQ( newMrl, newMediaThumbnail->mrl() );
}

TEST_F( Thumbnails, UnshareArtist )
{
    // Check that all thumbnails are shared, until we want to update the
    // shared version, in which case it should be duplicated

    auto t = Thumbnail::create( ml.get(), "file:///tmp/thumb.jpg", Thumbnail::Origin::Media, false );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3" ) );
    auto a = ml->createArtist(  "artist" );

    m->setThumbnail( t );
    a->setThumbnail( t );

    auto thumbnails = Thumbnail::fetchAll( ml.get() );
    ASSERT_EQ( 1u, thumbnails.size() );

    auto artistThumbnail = a->thumbnail();
    auto mediaThumbnail = m->thumbnail();
    ASSERT_EQ( artistThumbnail->id(), mediaThumbnail->id() );
    ASSERT_EQ( artistThumbnail->id(), t->id() );

    // Both the artist and the media have the same thumbnail_id. Now change the
    // media thumbnail, and check that the artist still has the same thumbnail &
    // thumbnail id, while the media has its own thumbnail

    auto newThumbnail = Thumbnail::create( ml.get(), "file:///tmp/newthumb.jpg",
                                           Thumbnail::Origin::UserProvided, false );
    a->setThumbnail( newThumbnail );
    thumbnails = Thumbnail::fetchAll( ml.get() );
    ASSERT_EQ( 2u, thumbnails.size() );

    artistThumbnail = a->thumbnail();
    mediaThumbnail = m->thumbnail();
    ASSERT_NE( nullptr, mediaThumbnail );
    ASSERT_EQ( t->mrl(), mediaThumbnail->mrl() );
    ASSERT_EQ( t->id(), mediaThumbnail->id() );

    ASSERT_NE( nullptr, artistThumbnail );
    ASSERT_EQ( newThumbnail->mrl(), artistThumbnail->mrl() );

    ASSERT_NE( artistThumbnail->id(), mediaThumbnail->id() );

    // Now let's re-update the media thumbnail, and check that it was only updated
    auto newMrl = std::string{ "file:///tmp/super_duper_new_thumbnail.png" };
    auto res = a->setThumbnail( newMrl );
    ASSERT_TRUE( res );

    thumbnails = Thumbnail::fetchAll( ml.get() );
    ASSERT_EQ( 2u, thumbnails.size() );

    auto newArtistThumbnail = a->thumbnail();
    ASSERT_EQ( artistThumbnail->id(), newArtistThumbnail->id() );
    ASSERT_EQ( newMrl, newArtistThumbnail->mrl() );
}

TEST_F( Thumbnails, UpdateIsOwned )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv" ) );
    auto mrl = std::string{ "file://path/to/a/thumbnail.jpg" };
    auto res = m->setThumbnail( mrl, Thumbnail::Origin::Media, false );
    ASSERT_TRUE( res );
    ASSERT_EQ( mrl, m->thumbnailMrl() );

    auto newMrl = utils::file::toMrl( ml->thumbnailPath() + "thumb.jpg" );
    res = m->setThumbnail( newMrl, Thumbnail::Origin::Media, true );
    ASSERT_TRUE( res );
    ASSERT_EQ( m->thumbnailMrl(), newMrl );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( newMrl, m->thumbnailMrl() );
}
