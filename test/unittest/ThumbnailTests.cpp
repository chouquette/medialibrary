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

class Thumbnails : public Tests
{
};

TEST_F( Thumbnails, Create )
{
    std::string mrl = "/path/to/thumbnail.png";
    auto t = Thumbnail::create( ml.get(), mrl, Thumbnail::Origin::UserProvided );
    ASSERT_NE( t, nullptr );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );
}

TEST_F( Thumbnails, MediaSetThumbnail )
{
    std::string mrl = "/path/to/thumbnail.png";
    auto m = ml->addFile( "/path/to/media.mp3" );
    ASSERT_FALSE( m->isThumbnailGenerated() );
    auto res = m->setThumbnail( mrl );
    ASSERT_TRUE( res );
    ASSERT_TRUE( m->isThumbnailGenerated() );
    ASSERT_EQ( m->thumbnail(), mrl );

    Reload();

    m = ml->media( m->id() );
    ASSERT_EQ( m->thumbnail(), mrl );
    ASSERT_TRUE( m->isThumbnailGenerated() );
}

TEST_F( Thumbnails, Update )
{
    std::string mrl = "/path/to/thumbnail.png";
    auto t = Thumbnail::create( ml.get(), mrl, Thumbnail::Origin::UserProvided );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );

    mrl = "/better/thumbnail.gif";
    auto res = t->update( mrl, Thumbnail::Origin::UserProvided );
    ASSERT_TRUE( res );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );

    Reload();

    t = Thumbnail::fetch( ml.get(), t->id() );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::UserProvided );

    res = t->update( mrl, Thumbnail::Origin::AlbumArtist );
    ASSERT_TRUE( res );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::AlbumArtist );

    Reload();

    t = Thumbnail::fetch( ml.get(), t->id() );
    ASSERT_EQ( t->mrl(), mrl );
    ASSERT_EQ( t->origin(), Thumbnail::Origin::AlbumArtist );
}
