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
#include "Album.h"
#include "utils/Filename.h"

class Thumbnails : public Tests
{
};

TEST_F( Thumbnails, Create )
{
    std::string mrl = "file:///path/to/thumbnail.png";
    auto t = Thumbnail::create( ml.get(), mrl, Thumbnail::Origin::UserProvided,
                                ThumbnailSizeType::Thumbnail, false );
    ASSERT_NE( t, nullptr );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );
}

TEST_F( Thumbnails, MediaSetThumbnail )
{
    std::string mrl = "file:///path/to/thumbnail.png";
    auto m = ml->addMedia( "/path/to/media.mp3" );
    ASSERT_FALSE( m->isThumbnailGenerated( ThumbnailSizeType::Thumbnail ) );
    auto res = m->setThumbnail( mrl, ThumbnailSizeType::Thumbnail );
    ASSERT_TRUE( res );
    ASSERT_TRUE( m->isThumbnailGenerated( ThumbnailSizeType::Thumbnail ) );
    ASSERT_EQ( m->thumbnailMrl( ThumbnailSizeType::Thumbnail ), mrl );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( m->thumbnailMrl( ThumbnailSizeType::Thumbnail ), mrl );
    ASSERT_TRUE( m->isThumbnailGenerated( ThumbnailSizeType::Thumbnail ) );
}

TEST_F( Thumbnails, Update )
{
    std::string mrl = "file:///path/to/thumbnail.png";
    auto t = Thumbnail::create( ml.get(), mrl, Thumbnail::Origin::UserProvided,
                                ThumbnailSizeType::Thumbnail, false );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "test.mkv" ) );
    m->setThumbnail( t );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );

    // Just update the mrl first
    mrl = "file:///better/thumbnail.gif";
    auto res = m->setThumbnail( mrl, Thumbnail::Origin::UserProvided,
                                ThumbnailSizeType::Thumbnail, false );
    ASSERT_TRUE( res );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );

    Reload();

    m = ml->media( m->id() );
    t = m->thumbnail( t->sizeType() );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );

    // Now update the origin
    res = m->setThumbnail( mrl, Thumbnail::Origin::AlbumArtist,
                           ThumbnailSizeType::Thumbnail, false );
    ASSERT_TRUE( res );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::AlbumArtist );

    Reload();

    m = ml->media( m->id() );
    t = m->thumbnail( t->sizeType() );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::AlbumArtist );
}

TEST_F( Thumbnails, MarkFailure )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv" ) );

    ASSERT_FALSE( m->isThumbnailGenerated( ThumbnailSizeType::Thumbnail ) );
    auto res = m->setThumbnail( "", Thumbnail::Origin::Media,
                                ThumbnailSizeType::Thumbnail, false );
    ASSERT_TRUE( res );

    ASSERT_TRUE( m->isThumbnailGenerated( ThumbnailSizeType::Thumbnail ) );

    Reload();

    m = std::static_pointer_cast<Media>( ml->media( m->id() ) );
    ASSERT_TRUE( m->isThumbnailGenerated( ThumbnailSizeType::Thumbnail ) );
    auto t = m->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_TRUE( t->isFailureRecord() );
}

TEST_F( Thumbnails, UnshareMedia )
{
    // Check that all thumbnails are shared, until we want to update the
    // shared version, in which case it should be duplicated

    auto t = Thumbnail::create( ml.get(), "file:///tmp/thumb.jpg",
                                Thumbnail::Origin::CoverFile,
                                ThumbnailSizeType::Thumbnail, false );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3" ) );
    auto a = ml->createArtist(  "artist" );

    m->setThumbnail( t );
    a->setThumbnail( t );

    ASSERT_EQ( 1u, ml->countNbThumbnails() );

    auto artistThumbnail = a->thumbnail( ThumbnailSizeType::Thumbnail );
    auto mediaThumbnail = m->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( artistThumbnail->id(), mediaThumbnail->id() );
    ASSERT_EQ( artistThumbnail->id(), t->id() );

    // Both the artist and the media have the same thumbnail_id. Now change the
    // media thumbnail, and check that the artist still has the same thumbnail &
    // thumbnail id, while the media has its own thumbnail

    auto newThumbnail = Thumbnail::create( ml.get(), "file:///tmp/newthumb.jpg",
                                           Thumbnail::Origin::UserProvided,
                                           ThumbnailSizeType::Thumbnail, false );
    m->setThumbnail( newThumbnail );
    ASSERT_EQ( 2u, ml->countNbThumbnails() );

    artistThumbnail = a->thumbnail( ThumbnailSizeType::Thumbnail );
    mediaThumbnail = m->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NE( nullptr, artistThumbnail );
    ASSERT_EQ( t->mrl(), artistThumbnail->mrl() );
    ASSERT_EQ( t->id(), artistThumbnail->id() );

    ASSERT_NE( nullptr, mediaThumbnail );
    ASSERT_EQ( newThumbnail->mrl(), mediaThumbnail->mrl() );

    ASSERT_NE( artistThumbnail->id(), mediaThumbnail->id() );

    // Now let's re-update the media thumbnail, and check that it was only updated
    auto newMrl = std::string{ "file:///tmp/super_duper_new_thumbnail.png" };
    auto res = m->setThumbnail( newMrl, Thumbnail::Origin::UserProvided,
                                ThumbnailSizeType::Thumbnail, false );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, ml->countNbThumbnails() );

    auto newMediaThumbnail = m->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( mediaThumbnail->id(), newMediaThumbnail->id() );
    ASSERT_EQ( newMrl, newMediaThumbnail->mrl() );
}

TEST_F( Thumbnails, UnshareArtist )
{
    // Check that all thumbnails are shared, until we want to update the
    // shared version, in which case it should be duplicated

    auto t = Thumbnail::create( ml.get(), "file:///tmp/thumb.jpg",
                                Thumbnail::Origin::Media,
                                ThumbnailSizeType::Thumbnail, false );
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mp3" ) );
    auto a = ml->createArtist(  "artist" );

    m->setThumbnail( t );
    a->setThumbnail( t );

    ASSERT_EQ( 1u, ml->countNbThumbnails() );

    auto artistThumbnail = a->thumbnail( ThumbnailSizeType::Thumbnail );
    auto mediaThumbnail = m->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( artistThumbnail->id(), mediaThumbnail->id() );
    ASSERT_EQ( artistThumbnail->id(), t->id() );

    // Both the artist and the media have the same thumbnail_id. Now change the
    // media thumbnail, and check that the artist still has the same thumbnail &
    // thumbnail id, while the media has its own thumbnail

    auto newThumbnail = Thumbnail::create( ml.get(), "file:///tmp/newthumb.jpg",
                                           Thumbnail::Origin::UserProvided,
                                           ThumbnailSizeType::Thumbnail, false );
    a->setThumbnail( newThumbnail );
    ASSERT_EQ( 2u, ml->countNbThumbnails() );

    artistThumbnail = a->thumbnail( ThumbnailSizeType::Thumbnail );
    mediaThumbnail = m->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NE( nullptr, mediaThumbnail );
    ASSERT_EQ( t->mrl(), mediaThumbnail->mrl() );
    ASSERT_EQ( t->id(), mediaThumbnail->id() );

    ASSERT_NE( nullptr, artistThumbnail );
    ASSERT_EQ( newThumbnail->mrl(), artistThumbnail->mrl() );

    ASSERT_NE( artistThumbnail->id(), mediaThumbnail->id() );

    // Now let's re-update the media thumbnail, and check that it was only updated
    auto newMrl = std::string{ "file:///tmp/super_duper_new_thumbnail.png" };
    auto res = a->setThumbnail( newMrl, ThumbnailSizeType::Thumbnail );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, ml->countNbThumbnails() );

    auto newArtistThumbnail = a->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_EQ( artistThumbnail->id(), newArtistThumbnail->id() );
    ASSERT_EQ( newMrl, newArtistThumbnail->mrl() );
}

TEST_F( Thumbnails, UpdateIsOwned )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.mkv" ) );
    auto mrl = std::string{ "file://path/to/a/thumbnail.jpg" };
    auto res = m->setThumbnail( mrl, Thumbnail::Origin::Media,
                                ThumbnailSizeType::Thumbnail, false );
    ASSERT_TRUE( res );
    ASSERT_EQ( mrl, m->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );

    auto newMrl = utils::file::toMrl( ml->thumbnailPath() + "thumb.jpg" );
    res = m->setThumbnail( newMrl, Thumbnail::Origin::Media,
                           ThumbnailSizeType::Thumbnail, true );
    ASSERT_TRUE( res );
    ASSERT_EQ( m->thumbnailMrl( ThumbnailSizeType::Thumbnail ), newMrl );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( newMrl, m->thumbnailMrl( ThumbnailSizeType::Thumbnail ) );
}

TEST_F( Thumbnails, CheckMultipleSizes )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.asf" ) );
    std::string smallMrl = "http://small_thumbnail.png";
    std::string largeMrl = "http://large_thumbnail.png";
    auto res = m->setThumbnail( smallMrl, ThumbnailSizeType::Thumbnail );
    ASSERT_TRUE( res );
    ASSERT_TRUE( m->isThumbnailGenerated( ThumbnailSizeType::Thumbnail ) );
    ASSERT_FALSE( m->isThumbnailGenerated( ThumbnailSizeType::Banner ) );

    auto thumbnail = m->thumbnail( ThumbnailSizeType::Thumbnail );
    auto banner = m->thumbnail( ThumbnailSizeType::Banner );
    ASSERT_NE( nullptr, thumbnail );
    ASSERT_EQ( nullptr, banner );
    ASSERT_EQ( 1u, ml->countNbThumbnails() );

    res = m->setThumbnail( largeMrl, ThumbnailSizeType::Banner );
    ASSERT_TRUE( res );
    banner = m->thumbnail( ThumbnailSizeType::Banner );
    ASSERT_NE( nullptr, banner );
    ASSERT_EQ( 2u, ml->countNbThumbnails() );

    ASSERT_EQ( smallMrl, thumbnail->mrl() );
    ASSERT_EQ( largeMrl, banner->mrl() );
    ASSERT_EQ( ThumbnailSizeType::Thumbnail, thumbnail->sizeType() );
    ASSERT_EQ( ThumbnailSizeType::Banner, banner->sizeType() );
    ASSERT_EQ( Thumbnail::Origin::UserProvided, thumbnail->origin() );
    ASSERT_EQ( Thumbnail::Origin::UserProvided, banner->origin() );

    Reload();

    m = ml->media( m->id() );
    thumbnail = m->thumbnail( ThumbnailSizeType::Thumbnail );
    banner = m->thumbnail( ThumbnailSizeType::Banner );
    ASSERT_NE( thumbnail->id(), banner->id() );

    ASSERT_EQ( smallMrl, thumbnail->mrl() );
    ASSERT_EQ( largeMrl, banner->mrl() );
    ASSERT_EQ( ThumbnailSizeType::Thumbnail, thumbnail->sizeType() );
    ASSERT_EQ( ThumbnailSizeType::Banner, banner->sizeType() );
    ASSERT_EQ( Thumbnail::Origin::UserProvided, thumbnail->origin() );
    ASSERT_EQ( Thumbnail::Origin::UserProvided, banner->origin() );
}

TEST_F( Thumbnails, AutoDelete )
{
    /*
     * Add 3 media, and share a thumbnail between 2 of them.
     * When the shared thumbnail gets unlinked from the 1st media, it should
     * stay in db. We then unlink the 2nd media from the shared thumbnail, and
     * expect the thumbnail to be removed afterward.
     */
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media1.mkv" ) );
    auto m2 = std::static_pointer_cast<Media>( ml->addMedia( "media2.mkv" ) );
    auto m3 = std::static_pointer_cast<Media>( ml->addMedia( "media3.mkv" ) );

    auto res = m->setThumbnail( "https://thumbnail.org/otter.gif",
                                ThumbnailSizeType::Thumbnail );
    ASSERT_TRUE( res );
    res = m2->setThumbnail( "https://thumbnail.org/cutter_otter.gif",
                            ThumbnailSizeType::Thumbnail );
    ASSERT_TRUE( res );
    auto thumbnail = m->thumbnail( ThumbnailSizeType::Thumbnail );
    ASSERT_NE( nullptr, thumbnail );
    res = m3->setThumbnail( thumbnail );
    ASSERT_TRUE( res );

    ASSERT_EQ( 2u, ml->countNbThumbnails() );

    m3->removeThumbnail( ThumbnailSizeType::Thumbnail );

    ASSERT_EQ( 2u, ml->countNbThumbnails() );

    m->removeThumbnail( ThumbnailSizeType::Thumbnail );

    ASSERT_EQ( 1u, ml->countNbThumbnails() );
}

TEST_F( Thumbnails, AutoDeleteAfterEntityRemoved )
{
    /*
     * Checks that the thumbnail gets removed when the associated entity is removed
     */
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "test.mkv" ) );
    auto alb = std::static_pointer_cast<Album>( ml->createAlbum( "album" ) );
    auto art = std::static_pointer_cast<Artist>( ml->createArtist( "artist" ) );
    m->setThumbnail( "https://otters.org/fluffy.png", ThumbnailSizeType::Thumbnail );
    alb->setThumbnail( std::make_shared<Thumbnail>( ml.get(), "https://thumbnail.org",
                                                    Thumbnail::Origin::Album,
                                                    ThumbnailSizeType::Thumbnail, false ) );
    art->setThumbnail( "http://thumbnail.org/pangolin.png", ThumbnailSizeType::Thumbnail );

    ASSERT_EQ( 3u, ml->countNbThumbnails() );

    Media::destroy( ml.get(), m->id() );
    ASSERT_EQ( 2u, ml->countNbThumbnails() );

    Album::destroy( ml.get(), alb->id() );
    ASSERT_EQ( 1u, ml->countNbThumbnails() );

    Artist::destroy( ml.get(), art->id() );
    ASSERT_EQ( 0u, ml->countNbThumbnails() );
}
