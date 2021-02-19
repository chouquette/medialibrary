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
#include "File.h"

struct FileTests : public Tests
{
    std::shared_ptr<File> f;
    std::shared_ptr<Media> m;

    virtual void TestSpecificSetup() override
    {
        m = ml->addFile( "media.mkv", IMedia::Type::Unknown );
        auto files = m->files();
        ASSERT_EQ( 1u, files.size() );
        f = std::static_pointer_cast<File>( files[0] );
    }
};

static void Create( FileTests* T )
{
    ASSERT_NE( 0u, T->f->id() );
    ASSERT_EQ( "media.mkv", T->f->mrl() );
    ASSERT_NE( 0u, T->f->lastModificationDate() );
    ASSERT_NE( 0u, T->f->size() );
    ASSERT_EQ( File::Type::Main, T->f->type() );
}

static void Remove( FileTests* T )
{
    T->m->removeFile( *T->f );
    // The instance should now have 0 files listed:
    auto files = T->m->files();
    ASSERT_EQ( 0u, files.size() );
    auto media = T->ml->media( T->m->id() );
    // This media should now be removed from the DB
    ASSERT_EQ( nullptr, media );
}

static void GetMedia( FileTests* T )
{
    ASSERT_EQ( T->m->id(), T->f->media()->id() );

    T->m = T->ml->media( T->m->id() );
    auto files = T->m->files();
    ASSERT_EQ( 1u, files.size() );
    T->f = std::static_pointer_cast<File>( files[0] );
    ASSERT_EQ( T->m->id(), T->f->media()->id() );
}

static void SetMrl( FileTests* T )
{
    const std::string newMrl = "/sea/otters/rules.mkv";
    T->f->setMrl( newMrl );
    ASSERT_EQ( newMrl, T->f->mrl() );

    auto files = T->m->files();
    ASSERT_EQ( 1u, files.size() );
    T->f = std::static_pointer_cast<File>( files[0] );
    ASSERT_EQ( T->f->mrl(), newMrl );
}

static void UpdateFsInfo( FileTests* T )
{
    auto res = T->f->updateFsInfo( 0, 0 );
    ASSERT_TRUE( res );

    T->f->updateFsInfo( 123, 456 );
    ASSERT_EQ( 123u, T->f->lastModificationDate() );
    ASSERT_EQ( 456u, T->f->size() );

    auto files = T->m->files();
    ASSERT_EQ( 1u, files.size() );
    T->f = std::static_pointer_cast<File>( files[0] );
    ASSERT_EQ( 123u, T->f->lastModificationDate() );
    ASSERT_EQ( 456u, T->f->size() );
}

static void Exists( FileTests* T )
{
    ASSERT_TRUE( File::exists( T->ml.get(), "media.mkv" ) );
    ASSERT_FALSE( File::exists( T->ml.get(), "another%20file.avi" ) );
}

static void CheckDbModel( FileTests* T )
{
    auto res = File::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void SetMediaId( FileTests* T )
{
    // First media is automatically added by the test SetUp()
    auto media2 = T->ml->addMedia( "media.ac3", IMedia::Type::Audio );

    auto files = T->m->files();
    ASSERT_EQ( 1u, files.size() );

    files = media2->files();
    ASSERT_EQ( 1u, files.size() );
    auto file2 = std::static_pointer_cast<File>( files[0] );
    auto res = file2->setMediaId( T->m->id() );
    ASSERT_TRUE( res );

    media2 = T->ml->media( media2->id() );
    ASSERT_EQ( nullptr, media2 );

    // Reload media to avoid failing because of an outdated cache
    T->m = T->ml->media( T->m->id() );
    files = T->m->files();
    ASSERT_EQ( 2u, files.size() );
}

static void ByMrlNetwork( FileTests* T )
{
    auto m1 = Media::createExternal( T->ml.get(), "smb://1.2.3.4/path/to/file.mkv", -1 );
    ASSERT_NE( nullptr, m1 );
    auto f1 = File::fromExternalMrl( T->ml.get(), "smb://1.2.3.4/path/to/file.mkv" );
    ASSERT_NE( nullptr, f1 );

    auto f2 = File::fromExternalMrl( T->ml.get(), "https://1.2.3.4/path/to/file.mkv" );
    ASSERT_EQ( nullptr, f2 );

    f2 = File::fromExternalMrl( T->ml.get(), "smb://1.2.3.4/path/to/file.mkv" );
    ASSERT_NE( nullptr, f2 );
    ASSERT_EQ( f1->id(), f2->id() );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( FileTests );

    ADD_TEST( Create );
    ADD_TEST( Remove );
    ADD_TEST( GetMedia );
    ADD_TEST( SetMrl );
    ADD_TEST( UpdateFsInfo );
    ADD_TEST( Exists );
    ADD_TEST( CheckDbModel );
    ADD_TEST( SetMediaId );
    ADD_TEST( ByMrlNetwork );

    END_TESTS
}
