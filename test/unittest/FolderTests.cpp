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
        unlink("test.db");
        fsMock.reset( new mock::FileSystemFactory );
        cbMock.reset( new mock::WaitForDiscoveryComplete );
        Reload();
    }

    virtual void InstantiateMediaLibrary() override
    {
        ml.reset( new MediaLibraryWithoutParser );
    }

    virtual void Reload()
    {
        Tests::Reload( fsMock, cbMock.get() );
    }
};

class Folders : public FoldersNoDiscover
{
    protected:
        virtual void SetUp() override
        {
            FoldersNoDiscover::SetUp();
            cbMock->prepareForWait();
            ml->discover( mock::FileSystemFactory::Root );
            bool discovered = cbMock->wait();
            ASSERT_TRUE( discovered );
        }
};

TEST_F( Folders, Add )
{
    auto files = ml->files();

    ASSERT_EQ( files.size(), 3u );
}

TEST_F( Folders, Delete )
{
    auto f = ml->folder( mock::FileSystemFactory::Root );
    auto folderPath = f->path();

    auto files = ml->files();
    ASSERT_EQ( files.size(), 3u );

    ml->deleteFolder( *f );

    f = ml->folder( folderPath );
    ASSERT_EQ( nullptr, f );

    files = ml->files();
    ASSERT_EQ( files.size(), 0u );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    // Recheck folder deletion from DB:
    f = ml->folder( folderPath );
    ASSERT_EQ( nullptr, f );
}

TEST_F( Folders, Load )
{
    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 3u );
}

TEST_F( FoldersNoDiscover, InvalidPath )
{
    cbMock->prepareForWait();
    ml->discover( "/invalid/path" );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 0u );
}

TEST_F( Folders, List )
{
    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_NE( f, nullptr );
    auto files = f->files();
    ASSERT_EQ( files.size(), 2u );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    f = ml->folder( f->path() );
    files = f->files();
    ASSERT_EQ( files.size(), 2u );
}

TEST_F( Folders, ListFolders )
{
    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_NE( f, nullptr );
    auto subFolders = f->folders();
    ASSERT_EQ( 1u, subFolders.size() );

    auto subFolder = subFolders[0];
    auto subFiles = subFolder->files();
    ASSERT_EQ( 1u, subFiles.size() );

    ASSERT_EQ( mock::FileSystemFactory::SubFolder + "subfile.mp4", subFiles[0]->mrl() );

    // Now again, without cache. No need to wait for fs discovery reload here
    Reload();

    f = ml->folder( f->path() );
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
    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 4u, ml->files().size() );
    auto f = ml->media( newFolder + "newfile.avi" );
    ASSERT_NE( nullptr, f );
}

// This is expected to fail until we fix the file system modifications detection
TEST_F( Folders, NewFileInSubFolder )
{
    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_EQ( 3u, ml->files().size() );

    f = ml->folder( mock::FileSystemFactory::SubFolder );
    // Do not watch for live changes
    ml.reset();
    fsMock->addFile( mock::FileSystemFactory::SubFolder + "newfile.avi" );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 4u, ml->files().size() );
    auto media = ml->media( mock::FileSystemFactory::SubFolder + "newfile.avi" );
    f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 2u, f->files().size() );
    ASSERT_NE( nullptr, media );
}

TEST_F( Folders, RemoveFileFromDirectory )
{
    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    fsMock->removeFile( mock::FileSystemFactory::SubFolder + "subfile.mp4" );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 2u, ml->files().size() );
    auto media = ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 0u, f->files().size() );
    ASSERT_EQ( nullptr, media );
}

TEST_F( Folders, RemoveDirectory )
{
    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    fsMock->removeFolder( mock::FileSystemFactory::SubFolder );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

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

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    f = ml->media( filePath );
    ASSERT_NE( nullptr, f );
    // The file is expected to be deleted and re-added since it changed, so the
    // id should have changed
    ASSERT_NE( id, f->id() );
}

TEST_F( FoldersNoDiscover, Blacklist )
{
    cbMock->prepareForWait();
    ml->banFolder( mock::FileSystemFactory::SubFolder );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );
}

TEST_F( Folders, BlacklistAfterDiscovery )
{
    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_NE( nullptr, f );
    auto files = f->files();
    ASSERT_NE( 0u, files.size() );

    ml->banFolder( mock::FileSystemFactory::SubFolder );
    auto f2 = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f2 );
}

TEST_F( FoldersNoDiscover, RemoveFromBlacklist )
{
    cbMock->prepareForWait();
    ml->banFolder( mock::FileSystemFactory::SubFolder );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );
    auto files = ml->files();
    ASSERT_EQ( 2u, files.size() );

    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );

    cbMock->prepareForReload();
    auto res = ml->unbanFolder( mock::FileSystemFactory::SubFolder );
    ASSERT_TRUE( res );
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );
    files = ml->files();
    ASSERT_EQ( 3u, files.size() );
    f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_NE( nullptr, f );
}

TEST_F( FoldersNoDiscover, BlacklistTwice )
{
    cbMock->prepareForWait();
    ml->banFolder( mock::FileSystemFactory::SubFolder );
    ml->banFolder( mock::FileSystemFactory::SubFolder );
}

TEST_F( FoldersNoDiscover, BlacklistNonExistant )
{
    cbMock->prepareForWait();
    ml->banFolder( "foo/bar/otters" );
    ml->banFolder( "/foo/bar/otters" );
    // Ban with an existing base
    ml->banFolder( mock::FileSystemFactory::Root + "grouik/" );
}

TEST_F( FoldersNoDiscover, NoMediaBeforeDiscovery )
{
    auto newFolder = mock::FileSystemFactory::Root + "newfolder/";
    fsMock->addFolder( newFolder );
    fsMock->addFile( newFolder + "newfile.avi" );
    fsMock->addFile( newFolder + ".nomedia" );

    cbMock->prepareForWait();
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
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

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 2u, files.size() );
}

TEST_F( Folders, InsertNoMediaInRoot )
{
    fsMock->addFile( mock::FileSystemFactory::Root + ".nomedia" );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    auto files = ml->files();
    ASSERT_EQ( 0u, files.size() );
}

TEST_F( Folders, ReloadSubDir )
{
    auto files = ml->files();
    ASSERT_EQ( 3u, files.size() );
    fsMock->addFile( mock::FileSystemFactory::Root + "newmedia.mkv" );

    cbMock->prepareForReload();
    ml->reload( mock::FileSystemFactory::SubFolder );
    bool reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 3u, files.size() );

    cbMock->prepareForReload();
    ml->reload();
    reloaded = cbMock->wait();
    ASSERT_TRUE( reloaded );

    files = ml->files();
    ASSERT_EQ( 4u, files.size() );
}
