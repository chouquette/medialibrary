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

#include "Media.h"
#include "Folder.h"
#include "IMediaLibrary.h"
#include "utils/Filename.h"
#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"

#include <memory>

class Folders : public Tests
{
    protected:
        std::shared_ptr<mock::FileSystemFactory> fsMock;
        std::unique_ptr<mock::WaitForDiscoveryComplete> cbMock;

    protected:
        virtual void SetUp() override
        {
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

TEST_F( Folders, Add )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();

    ASSERT_EQ( files.size(), 3u );
}

TEST_F( Folders, Delete )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::Root );
    auto folderPath = f->path();

    auto files = ml->files();
    ASSERT_EQ( files.size(), 3u );

    auto filePath = files[0]->mrl();

    ml->deleteFolder( f.get() );

    f = ml->folder( folderPath );
    ASSERT_EQ( nullptr, f );

    files = ml->files();
    ASSERT_EQ( files.size(), 0u );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    // Recheck folder deletion from DB:
    f = ml->folder( folderPath );
    ASSERT_EQ( nullptr, f );
}

TEST_F( Folders, Load )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 3u );
}

TEST_F( Folders, InvalidPath )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "/invalid/path" );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 0u );
}

TEST_F( Folders, List )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_NE( f, nullptr );
    auto files = f->files();
    ASSERT_EQ( files.size(), 2u );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    f = ml->folder( f->path() );
    files = f->files();
    ASSERT_EQ( files.size(), 2u );
}

TEST_F( Folders, ListFolders )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_NE( f, nullptr );
    auto subFolders = f->folders();
    ASSERT_EQ( 1u, subFolders.size() );

    auto subFolder = subFolders[0];
    auto subFiles = subFolder->files();
    ASSERT_EQ( 1u, subFiles.size() );

    auto file = subFiles[0];
    ASSERT_EQ( mock::FileSystemFactory::SubFolder + "subfile.mp4", file->mrl() );

    // Now again, without cache. No need to wait for fs discovery reload here
    Reload();

    f = ml->folder( f->path() );
    subFolders = f->folders();
    ASSERT_EQ( 1u, subFolders.size() );

    subFolder = subFolders[0];
    subFiles = subFolder->files();
    ASSERT_EQ( 1u, subFiles.size() );

    file = subFiles[0];
    ASSERT_EQ( mock::FileSystemFactory::SubFolder + "subfile.mp4", file->mrl() );
}

TEST_F( Folders, NewFolderWithFile )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    auto newFolder = mock::FileSystemFactory::Root + "newfolder/";
    fsMock->addFolder( newFolder );
    fsMock->addFile( newFolder + "newfile.avi" );

    // This will trigger a reload
    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 4u, ml->files().size() );
    auto f = ml->media( newFolder + "newfile.avi" );
    ASSERT_NE( nullptr, f );
}

// This is expected to fail until we fix the file system modifications detection
TEST_F( Folders, NewFileInSubFolder )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_EQ( 3u, ml->files().size() );

    f = ml->folder( mock::FileSystemFactory::SubFolder );
    // Do not watch for live changes
    ml.reset();
    fsMock->addFile( mock::FileSystemFactory::SubFolder + "newfile.avi" );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 4u, ml->files().size() );
    auto file = ml->media( mock::FileSystemFactory::SubFolder + "newfile.avi" );
    f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 2u, f->files().size() );
    ASSERT_NE( nullptr, file );
}

TEST_F( Folders, RemoveFileFromDirectory )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    fsMock->removeFile( mock::FileSystemFactory::SubFolder + "subfile.mp4" );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 2u, ml->files().size() );
    auto file = ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 0u, f->files().size() );
    ASSERT_EQ( nullptr, file );
}

TEST_F( Folders, RemoveDirectory )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    fsMock->removeFolder( mock::FileSystemFactory::SubFolder );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 2u, ml->files().size() );
    auto file = ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );
    ASSERT_EQ( nullptr, file );
}

TEST_F( Folders, UpdateFile )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto filePath = mock::FileSystemFactory::SubFolder + "subfile.mp4";
    auto f = ml->media( filePath );
    ASSERT_NE( f, nullptr );
    auto id = f->id();

    ml.reset();
    fsMock->file( filePath )->markAsModified();

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    f = ml->media( filePath );
    ASSERT_NE( nullptr, f );
    // The file is expected to be deleted and re-added since it changed, so the
    // id should have changed
    ASSERT_NE( id, f->id() );
}

TEST_F( Folders, Blacklist )
{
    cbMock->prepareForWait( 1 );
    ml->ignoreFolder( mock::FileSystemFactory::SubFolder );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );
}

TEST_F( Folders, BlacklistAfterDiscovery )
{
    cbMock->prepareForWait( 1 );
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );
    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_NE( nullptr, f );
    auto files = f->files();
    ASSERT_NE( 0u, files.size() );

    ml->ignoreFolder( mock::FileSystemFactory::SubFolder );
    auto f2 = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f2 );
    for ( auto& file : files )
    {
        auto m = ml->media( file->mrl() );
        ASSERT_EQ( nullptr, m );
    }

}
