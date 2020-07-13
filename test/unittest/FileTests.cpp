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
#include "File.h"

class Files : public Tests
{
protected:
    std::shared_ptr<File> f;
    std::shared_ptr<Media> m;
    virtual void SetUp() override
    {
        Tests::SetUp();
        m = ml->addFile( "media.mkv", IMedia::Type::Unknown );
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
    // This media should now be removed from the DB
    ASSERT_EQ( nullptr, media );
}

TEST_F( Files, Media )
{
    ASSERT_EQ( m->id(), f->media()->id() );

    m = ml->media( m->id() );
    auto files = m->files();
    ASSERT_EQ( 1u, files.size() );
    f = std::static_pointer_cast<File>( files[0] );
    ASSERT_EQ( m->id(), f->media()->id() );
}

TEST_F( Files, SetMrl )
{
    const std::string newMrl = "/sea/otters/rules.mkv";
    f->setMrl( newMrl );
    ASSERT_EQ( newMrl, f->mrl() );

    auto files = m->files();
    ASSERT_EQ( 1u, files.size() );
    f = std::static_pointer_cast<File>( files[0] );
    ASSERT_EQ( f->mrl(), newMrl );
}

TEST_F( Files, UpdateFsInfo )
{
    auto res = f->updateFsInfo( 0, 0 );
    ASSERT_TRUE( res );

    f->updateFsInfo( 123, 456 );
    ASSERT_EQ( 123u, f->lastModificationDate() );
    ASSERT_EQ( 456u, f->size() );

    auto files = m->files();
    ASSERT_EQ( 1u, files.size() );
    f = std::static_pointer_cast<File>( files[0] );
    ASSERT_EQ( 123u, f->lastModificationDate() );
    ASSERT_EQ( 456u, f->size() );
}

TEST_F( Files, Exists )
{
    ASSERT_TRUE( File::exists( ml.get(), "media.mkv" ) );
    ASSERT_FALSE( File::exists( ml.get(), "another%20file.avi" ) );
}

TEST_F( Files, CheckDbModel )
{
    auto res = File::checkDbModel( ml.get() );
    ASSERT_TRUE( res );
}

TEST_F( Files, SetMediaId )
{
    // First media is automatically added by the test SetUp()
    auto media2 = ml->addMedia( "media.ac3", IMedia::Type::Audio );

    auto files = m->files();
    ASSERT_EQ( 1u, files.size() );

    files = media2->files();
    ASSERT_EQ( 1u, files.size() );
    auto file2 = std::static_pointer_cast<File>( files[0] );
    auto res = file2->setMediaId( m->id() );
    ASSERT_TRUE( res );

    media2 = ml->media( media2->id() );
    ASSERT_EQ( nullptr, media2 );

    // Reload media to avoid failing because of an outdated cache
    m = ml->media( m->id() );
    files = m->files();
    ASSERT_EQ( 2u, files.size() );
}

TEST_F( Files, ByMrlNetwork )
{
    auto m1 = Media::createExternal( ml.get(), "smb://1.2.3.4/path/to/file.mkv" );
    ASSERT_NE( nullptr, m1 );
    auto f1 = File::fromExternalMrl( ml.get(), "smb://1.2.3.4/path/to/file.mkv" );
    ASSERT_NE( nullptr, f1 );

    auto f2 = File::fromExternalMrl( ml.get(), "https://1.2.3.4/path/to/file.mkv" );
    ASSERT_EQ( nullptr, f2 );

    f2 = File::fromExternalMrl( ml.get(), "smb://1.2.3.4/path/to/file.mkv" );
    ASSERT_NE( nullptr, f2 );
    ASSERT_EQ( f1->id(), f2->id() );
}
