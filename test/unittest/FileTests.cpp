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
#include "File.h"

class Files : public Tests
{
protected:
    std::shared_ptr<File> f;
    std::shared_ptr<Media> m;
    virtual void SetUp() override
    {
        Tests::SetUp();
        m = ml->addFile( "media.mkv" );
        auto files = m->files();
        ASSERT_EQ( 1u, files.size() );
        f = std::static_pointer_cast<File>( files[0] );
    }
};

TEST_F( Files, Create )
{
    ASSERT_NE( 0u, f->id() );
    ASSERT_EQ( "media.mkv", f->mrl() );
    ASSERT_NE( 0u, f->lastModificationDate() );
    ASSERT_NE( 0u, f->size() );
    ASSERT_EQ( File::Type::Main, f->type() );
}

TEST_F( Files, Remove )
{
    m->removeFile( *f );
    // The instance should now have 0 files listed:
    auto files = m->files();
    ASSERT_EQ( 0u, files.size() );
    auto media = ml->media( m->id() );
    // This file should now be removed from the DB
    ASSERT_EQ( nullptr, media );
}

TEST_F( Files, Media )
{
    ASSERT_EQ( m->id(), f->media()->id() );

    Reload();

    auto files = m->files();
    ASSERT_EQ( 1u, files.size() );
    f = std::static_pointer_cast<File>( files[0] );
    ASSERT_EQ( m->id(), f->media()->id() );
}
