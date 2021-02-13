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

#include "medialibrary/IMediaLibrary.h"
#include "Media.h"
#include "medialibrary/ILabel.h"
#include "Label.h"

static void Add( Tests* T )
{
    auto f = T->ml->addMedia( "media.avi", IMedia::Type::Video );
    auto l1 = T->ml->createLabel( "sea otter" );
    auto l2 = T->ml->createLabel( "cony the cone" );

    ASSERT_NE( l1, nullptr);
    ASSERT_NE( l2, nullptr);

    auto labels = f->labels()->all();
    ASSERT_EQ( labels.size(), 0u );

    f->addLabel( l1 );
    f->addLabel( l2 );

    labels = f->labels()->all();

    ASSERT_EQ( labels.size(), 2u );
    ASSERT_EQ( labels[0]->name(), "sea otter" );
    ASSERT_EQ( labels[1]->name(), "cony the cone" );
}

static void Remove( Tests* T )
{
    auto m = T->ml->addMedia( "media.avi", IMedia::Type::Video );
    auto l1 = T->ml->createLabel( "sea otter" );
    auto l2 = T->ml->createLabel( "cony the cone" );

    m->addLabel( l1 );
    m->addLabel( l2 );

    auto labels = m->labels()->all();
    ASSERT_EQ( labels.size(), 2u );

    bool res = m->removeLabel( l1 );
    ASSERT_TRUE( res );

    // Check for existing media first
    labels = m->labels()->all();
    ASSERT_EQ( labels.size(), 1u );
    ASSERT_EQ( labels[0]->name(), "cony the cone" );

    // And now clean fetch another instance of the media & check again for DB replication
    auto media = T->ml->media( m->id() );
    labels = media->labels()->all();
    ASSERT_EQ( labels.size(), 1u );
    ASSERT_EQ( labels[0]->name(), "cony the cone" );

    // Remove a non-linked label
    res = m->removeLabel( l1 );
    ASSERT_TRUE( res );

    // Remove the last label
    res = m->removeLabel( l2 );
    ASSERT_TRUE( res );

    labels = m->labels()->all();
    ASSERT_EQ( labels.size(), 0u );

    // Check again for DB replication
    media = T->ml->media( m->id() );
    labels = media->labels()->all();
    ASSERT_EQ( labels.size(), 0u );
}

static void Files( Tests* T )
{
    auto f = T->ml->addMedia( "media.avi", IMedia::Type::Video );
    auto f2 = T->ml->addMedia( "file.mp3", IMedia::Type::Audio );
    auto f3 = T->ml->addMedia( "otter.mkv", IMedia::Type::Video);

    auto l1 = T->ml->createLabel( "label1" );
    auto l2 = T->ml->createLabel( "label2" );

    f->addLabel( l1 );
    f2->addLabel( l2 );
    f3->addLabel( l1 );

    auto label1Files = l1->media()->all();
    auto label2Files = l2->media()->all();

    ASSERT_EQ( label1Files.size(), 2u );
    ASSERT_EQ( label2Files.size(), 1u );

    ASSERT_EQ( label2Files[0]->id(), f2->id() );

    for (auto labelFile : label1Files )
    {
        ASSERT_TRUE( labelFile->id() == f->id() ||
                     labelFile->id() == f3->id() );
    }
}

static void Delete( Tests* T )
{
    auto f = T->ml->addMedia( "media.avi", IMedia::Type::Video );
    auto l1 = T->ml->createLabel( "sea otter" );
    auto l2 = T->ml->createLabel( "cony the cone" );

    f->addLabel( l1 );
    f->addLabel( l2 );

    auto labels = f->labels()->all();
    ASSERT_EQ( labels.size(), 2u );

    T->ml->deleteLabel( l1 );
    labels = f->labels()->all();
    ASSERT_EQ( labels.size(), 1u );

    T->ml->deleteLabel( l2 );
    labels = f->labels()->all();
    ASSERT_EQ( labels.size(), 0u );

    // Nothing to delete anymore, this should just do nothing
    bool res = T->ml->deleteLabel( l1 );
    ASSERT_TRUE( res );
}

static void CheckDbModel( Tests* T )
{
    auto res = Label::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

int main( int ac, char** av )
{
    INIT_TESTS;

    ADD_TEST( Add );
    ADD_TEST( Remove );
    ADD_TEST( Files );
    ADD_TEST( Delete );
    ADD_TEST( CheckDbModel );

    END_TESTS;
}
