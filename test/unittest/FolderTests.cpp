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
#include "Folder.h"
#include "medialibrary/IMediaLibrary.h"
#include "utils/Filename.h"
#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"

#include <memory>


class FoldersNoDiscover : public Tests
{
protected:
    std::shared_ptr<mock::FileSystemFactory> fsMock;
    std::unique_ptr<mock::WaitForDiscoveryComplete> cbMock;

public:
    virtual void SetUp() override
    {
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        fsFactory = fsMock;
        mlCb = cbMock.get();
        Tests::SetUp();
    }

    virtual void InstantiateMediaLibrary() override
    {
        ml.reset( new MediaLibraryWithDiscoverer );
    }

    virtual void Reload() override
    {
        Tests::Reload();
        auto res = cbMock->waitReload();
        ASSERT_TRUE( res );
    }
};

class Folders : public FoldersNoDiscover
{
    protected:
        virtual void SetUp() override
        {
            FoldersNoDiscover::SetUp();
            ml->discover( mock::FileSystemFactory::Root );
            bool discovered = cbMock->waitDiscovery();
            ASSERT_TRUE( discovered );
        }
};

TEST_F( Folders, Add )
{
    auto files = ml->files();

    ASSERT_EQ( files.size(), 3u );
}

TEST_F( Folders, Load )
{
    Reload();

    auto files = ml->files();
    ASSERT_EQ( files.size(), 3u );
}

TEST_F( FoldersNoDiscover, InvalidPath )
{
    ml->discover( "/invalid/path" );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 0u );
}

TEST_F( Folders, List )
{
    auto f = std::static_pointer_cast<Folder>( ml->folder( mock::FileSystemFactory::Root ) );
    ASSERT_NE( f, nullptr );
    auto files = f->files();
    ASSERT_EQ( files.size(), 2u );

    Reload();

    f = std::static_pointer_cast<Folder>( ml->folder( f->mrl() ) );
    files = f->files();
    ASSERT_EQ( files.size(), 2u );
}

TEST_F( Folders, ListFolders )
{
    auto f = std::static_pointer_cast<Folder>( ml->folder( mock::FileSystemFactory::Root ) );
    ASSERT_NE( f, nullptr );
    auto subFolders = f->folders();
    ASSERT_EQ( 1u, subFolders.size() );

    auto subFolder = subFolders[0];
    auto subFiles = subFolder->files();
    ASSERT_EQ( 1u, subFiles.size() );

    ASSERT_EQ( mock::FileSystemFactory::SubFolder + "subfile.mp4", subFiles[0]->mrl() );

    // Now again, without cache. No need to wait for fs discovery reload here
    Reload();

    f = std::static_pointer_cast<Folder>( ml->folder( f->mrl() ) );
    subFolders = f->folders();
    ASSERT_EQ( 1u, subFolders.size() );

    subFolder = subFolders[0];
    subFiles = subFolder->files();
    ASSERT_EQ( 1u, subFiles.size() );

    ASSERT_EQ( mock::FileSystemFactory::SubFolder + "subfile.mp4", subFiles[0]->mrl() );
}

TEST_F( Folders, NewFolderWithFile )
{
    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    auto newFolder = mock::FileSystemFactory::Root + "newfolder/";
    fsMock->addFolder( newFolder );
    fsMock->addFile( newFolder + "newfile.avi" );

    // This will trigger a reload
    Reload();

    ASSERT_EQ( 4u, ml->files().size() );
    auto f = ml->media( newFolder + "newfile.avi" );
    ASSERT_NE( nullptr, f );
}

// This is expected to fail until we fix the file system modifications detection
TEST_F( Folders, NewFileInSubFolder )
{
    auto f = std::static_pointer_cast<Folder>( ml->folder( mock::FileSystemFactory::Root ) );
    ASSERT_EQ( 3u, ml->files().size() );

    f = std::static_pointer_cast<Folder>( ml->folder( mock::FileSystemFactory::SubFolder ) );
    // Do not watch for live changes
    ml.reset();
    fsMock->addFile( mock::FileSystemFactory::SubFolder + "newfile.avi" );

    Reload();

    ASSERT_EQ( 4u, ml->files().size() );
    auto media = ml->media( mock::FileSystemFactory::SubFolder + "newfile.avi" );
    f = std::static_pointer_cast<Folder>( ml->folder( mock::FileSystemFactory::SubFolder ) );
    ASSERT_EQ( 2u, f->files().size() );
    ASSERT_NE( nullptr, media );
}

TEST_F( Folders, RemoveFileFromDirectory )
{
    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    fsMock->removeFile( mock::FileSystemFactory::SubFolder + "subfile.mp4" );

    Reload();

    ASSERT_EQ( 2u, ml->files().size() );
    auto media = ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    auto f = std::static_pointer_cast<Folder>( ml->folder( mock::FileSystemFactory::SubFolder ) );
    ASSERT_EQ( 0u, f->files().size() );
    ASSERT_EQ( nullptr, media );
}

TEST_F( Folders, RemoveDirectory )
{
    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    fsMock->removeFolder( mock::FileSystemFactory::SubFolder );

    Reload();

    ASSERT_EQ( 2u, ml->files().size() );
    auto media = ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );
    ASSERT_EQ( nullptr, media );
}

TEST_F( Folders, UpdateFile )
{
    auto filePath = mock::FileSystemFactory::SubFolder + "subfile.mp4";
    auto f = ml->media( filePath );
    ASSERT_NE( f, nullptr );
    auto id = f->id();

    ml.reset();
    fsMock->file( filePath )->markAsModified();

    Reload();

    f = ml->media( filePath );
    ASSERT_NE( nullptr, f );
    // File won't be refreshed since unittests don't have parsers (and the file
    // doesn't actually exist) but let's check it's not deleted/re-added anymore
    ASSERT_EQ( id, f->id() );
}

TEST_F( FoldersNoDiscover, Ban )
{
    ml->banFolder( mock::FileSystemFactory::SubFolder );
    cbMock->waitBanFolder();
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );
}

TEST_F( FoldersNoDiscover, DiscoverBanned )
{
    ml->banFolder( mock::FileSystemFactory::Root );
    cbMock->waitBanFolder();
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_EQ( nullptr, f );
}

TEST_F( Folders, BanAfterDiscovery )
{
    auto f = std::static_pointer_cast<Folder>( ml->folder( mock::FileSystemFactory::SubFolder ) );
    ASSERT_NE( nullptr, f );
    auto files = f->files();
    ASSERT_NE( 0u, files.size() );

    ml->banFolder( mock::FileSystemFactory::SubFolder );
    cbMock->waitBanFolder();
    auto f2 = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f2 );
}

TEST_F( FoldersNoDiscover, RemoveFromBannedList )
{
    ml->banFolder( mock::FileSystemFactory::SubFolder );
    cbMock->waitBanFolder();
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
    auto files = ml->files();
    ASSERT_EQ( 2u, files.size() );

    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );

    ml->unbanFolder( mock::FileSystemFactory::SubFolder );
    cbMock->waitBanFolder();
    files = ml->files();
    ASSERT_EQ( 3u, files.size() );
    f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_NE( nullptr, f );
}

TEST_F( FoldersNoDiscover, BanTwice )
{
    ml->banFolder( mock::FileSystemFactory::SubFolder );
    cbMock->waitBanFolder();
    ml->banFolder( mock::FileSystemFactory::SubFolder );
    cbMock->waitBanFolder();
}

TEST_F( FoldersNoDiscover, BanNonExistant )
{
    ml->banFolder( "foo/bar/otters" );
    cbMock->waitBanFolder();
    ml->banFolder( "/foo/bar/otters" );
    cbMock->waitBanFolder();
    // Ban with an existing base
    ml->banFolder( mock::FileSystemFactory::Root + "grouik/" );
    cbMock->waitBanFolder();
}

TEST_F( FoldersNoDiscover, UnbanNonExistant )
{
    ml->unbanFolder( "foo/bar/otters" );
    cbMock->waitUnbanFolder();
    ml->unbanFolder( "/foo/bar/otters" );
    cbMock->waitUnbanFolder();
    // Ban with an existing base
    ml->unbanFolder( mock::FileSystemFactory::Root + "grouik/" );
    cbMock->waitUnbanFolder();
    // Ban existing but unbanned folder
    ml->unbanFolder( mock::FileSystemFactory::Root );
    cbMock->waitUnbanFolder();
}

TEST_F( FoldersNoDiscover, NoMediaBeforeDiscovery )
{
    auto newFolder = mock::FileSystemFactory::Root + "newfolder/";
    fsMock->addFolder( newFolder );
    fsMock->addFile( newFolder + "newfile.avi" );
    fsMock->addFile( newFolder + ".nomedia" );

    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    // We add 3 files before, and the new one shouldn't be account for since there is a .nomedia file
    ASSERT_EQ( 3u, files.size() );
}

TEST_F( Folders, InsertNoMedia )
{
    auto files = ml->files();
    ASSERT_EQ( 3u, files.size() );
    fsMock->addFile( mock::FileSystemFactory::SubFolder + ".nomedia" );

    Reload();

    files = ml->files();
    ASSERT_EQ( 2u, files.size() );
}

TEST_F( Folders, InsertNoMediaInRoot )
{
    fsMock->addFile( mock::FileSystemFactory::Root + ".nomedia" );

    Reload();

    auto files = ml->files();
    ASSERT_EQ( 0u, files.size() );
}

TEST_F( Folders, ReloadSubDir )
{
    auto files = ml->files();
    ASSERT_EQ( 3u, files.size() );
    fsMock->addFile( mock::FileSystemFactory::Root + "newmedia.mkv" );

    ml->reload( mock::FileSystemFactory::SubFolder );
    auto res = cbMock->waitReload();
    ASSERT_TRUE( res );

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    ml->reload();
    res = cbMock->waitReload();
    ASSERT_TRUE( res );

    files = ml->files();
    ASSERT_EQ( 4u, files.size() );
}

TEST_F( Folders, FetchEntryPoints )
{
    auto eps = ml->entryPoints()->all();
    ASSERT_EQ( 1u, eps.size() );
    ASSERT_EQ( mock::FileSystemFactory::Root, eps[0]->mrl() );

    // Check that banned folders don't appear in the results:
    ml->banFolder( mock::FileSystemFactory::SubFolder );
    auto res = cbMock->waitBanFolder();
    ASSERT_TRUE( res );
    eps = ml->entryPoints()->all();
    ASSERT_EQ( 1u, eps.size() );
}

TEST_F( Folders, RemoveRootEntryPoint )
{
    auto media = ml->files();
    ASSERT_NE( 0u, media.size() );

    ml->removeEntryPoint( mock::FileSystemFactory::Root );
    auto res = cbMock->waitEntryPointRemoved();
    ASSERT_TRUE( res );

    media = ml->files();
    ASSERT_EQ( 0u, media.size() );

    auto eps = ml->entryPoints()->all();
    ASSERT_EQ( 0u, eps.size() );
}

TEST_F( Folders, RemoveEntryPoint )
{
    auto media = ml->files();
    ASSERT_NE( 0u, media.size() );

    ml->removeEntryPoint( mock::FileSystemFactory::SubFolder );
    auto res = cbMock->waitEntryPointRemoved();
    ASSERT_TRUE( res );

    media = ml->files();
    ASSERT_NE( 0u, media.size() );

    auto eps = ml->entryPoints()->all();
    ASSERT_EQ( 1u, eps.size() );

    ml->reload();

    // Ensure it wasn't re-discovered, ie. that it was properly banned
    auto media2 = ml->files();
    ASSERT_EQ( media.size(), media2.size() );
}

TEST_F( Folders, RemoveNonExistantEntryPoint )
{
    ml->removeEntryPoint( "/sea/otter" );
    auto res = cbMock->waitEntryPointRemoved();
    ASSERT_TRUE( res );
}

TEST_F( Folders, RemoveRootFolder )
{
    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    fsMock->removeFolder( mock::FileSystemFactory::Root );

    Reload();

    ASSERT_EQ( 0u, ml->files().size() );
}

TEST_F( Folders, NbMedia )
{
    auto root = ml->folder( 1 );
    auto subFolder = ml->folder( 2 );
    ASSERT_EQ( "file:///a/", root->mrl() );
    ASSERT_EQ( "file:///a/folder/", subFolder->mrl() );
    ASSERT_EQ( 2u, root->nbMedia() );
    ASSERT_EQ( 1u, subFolder->nbMedia() );
    // Do not watch for live changes
    ml.reset();
    fsMock->removeFile( mock::FileSystemFactory::SubFolder + "subfile.mp4" );

    Reload();

    root = ml->folder( 1 );
    subFolder = ml->folder( 2 );

    ASSERT_EQ( 2u, root->nbMedia() );
    ASSERT_EQ( 0u, subFolder->nbMedia() );
}
