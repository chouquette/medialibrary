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
#include "Folder.h"
#include "medialibrary/IMediaLibrary.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "mocks/FileSystem.h"
#include "mocks/DiscovererCbMock.h"

#include <memory>

struct FolderTests : public UnitTests<mock::WaitForDiscoveryComplete>
{
public:
    virtual void InstantiateMediaLibrary( const std::string& dbPath,
                                          const std::string& mlFolderDir,
                                          const SetupConfig* cfg ) override
    {
        ml.reset( new MediaLibraryWithDiscoverer( dbPath, mlFolderDir, cfg ) );
    }

    void Reload()
    {
        ml->reload();
        auto res = cbMock->waitReload();
        ASSERT_TRUE( res );
    }
};

static void enforceFakeMediaTypes( MediaLibraryTester* ml )
{
    auto media = std::static_pointer_cast<Media>( ml->media(
                                mock::FileSystemFactory::Root + "video.avi" ) );
    media->setType( IMedia::Type::Video );

    media = std::static_pointer_cast<Media>( ml->media(
                                    mock::FileSystemFactory::Root + "audio.mp3" ) );
    media->setType( IMedia::Type::Audio );

    media = std::static_pointer_cast<Media>( ml->media(
                                    mock::FileSystemFactory::SubFolder + "subfile.mp4" ) );
    media->setType( IMedia::Type::Video );
}

static void Add( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = T->ml->files();

    ASSERT_EQ( files.size(), 3u );
}

static void Load( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    T->Reload();

    auto files = T->ml->files();
    ASSERT_EQ( files.size(), 3u );
}

static void InvalidPath( FolderTests* T )
{
    T->ml->discover( "/invalid/path" );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = T->ml->files();
    ASSERT_EQ( files.size(), 0u );
}

static void List( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto f = std::static_pointer_cast<Folder>( T->ml->folder( mock::FileSystemFactory::Root ) );
    ASSERT_NE( f, nullptr );
    auto files = f->files();
    ASSERT_EQ( files.size(), 2u );

    T->Reload();

    f = std::static_pointer_cast<Folder>( T->ml->folder( f->mrl() ) );
    files = f->files();
    ASSERT_EQ( files.size(), 2u );
}

static void ListFolders( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto f = std::static_pointer_cast<Folder>( T->ml->folder( mock::FileSystemFactory::Root ) );
    ASSERT_NE( f, nullptr );
    auto subFolders = f->folders();
    ASSERT_EQ( 1u, subFolders.size() );

    auto subFolder = subFolders[0];
    auto subFiles = subFolder->files();
    ASSERT_EQ( 1u, subFiles.size() );

    ASSERT_EQ( mock::FileSystemFactory::SubFolder + "subfile.mp4", subFiles[0]->mrl() );

    // Now again, without cache. No need to wait for fs discovery reload here
    T->Reload();

    f = std::static_pointer_cast<Folder>( T->ml->folder( f->mrl() ) );
    subFolders = f->folders();
    ASSERT_EQ( 1u, subFolders.size() );

    subFolder = subFolders[0];
    subFiles = subFolder->files();
    ASSERT_EQ( 1u, subFiles.size() );

    ASSERT_EQ( mock::FileSystemFactory::SubFolder + "subfile.mp4", subFiles[0]->mrl() );
}

static void NewFolderWithFile( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u, T->ml->files().size() );

    auto newFolder = mock::FileSystemFactory::Root + "newfolder/";
    T->fsMock->addFolder( newFolder );
    T->fsMock->addFile( newFolder + "newfile.avi" );

    // This will trigger a reload
    T->Reload();

    ASSERT_EQ( 4u, T->ml->files().size() );
    auto f = T->ml->media( newFolder + "newfile.avi" );
    ASSERT_NE( nullptr, f );
}

static void NewFileInSubFolder( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto f = std::static_pointer_cast<Folder>( T->ml->folder( mock::FileSystemFactory::Root ) );
    ASSERT_EQ( 3u, T->ml->files().size() );

    f = std::static_pointer_cast<Folder>( T->ml->folder( mock::FileSystemFactory::SubFolder ) );

    T->fsMock->addFile( mock::FileSystemFactory::SubFolder + "newfile.avi" );

    T->Reload();

    ASSERT_EQ( 4u, T->ml->files().size() );
    auto media = T->ml->media( mock::FileSystemFactory::SubFolder + "newfile.avi" );
    f = std::static_pointer_cast<Folder>( T->ml->folder( mock::FileSystemFactory::SubFolder ) );
    ASSERT_EQ( 2u, f->files().size() );
    ASSERT_NE( nullptr, media );
}

static void RemoveFileFromDirectory( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u, T->ml->files().size() );

    T->fsMock->removeFile( mock::FileSystemFactory::SubFolder + "subfile.mp4" );

    T->Reload();

    ASSERT_EQ( 2u, T->ml->files().size() );
    auto media = T->ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    auto f = std::static_pointer_cast<Folder>( T->ml->folder( mock::FileSystemFactory::SubFolder ) );
    ASSERT_EQ( 0u, f->files().size() );
    ASSERT_EQ( nullptr, media );
}

static void RemoveDirectory( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u, T->ml->files().size() );

    T->fsMock->removeFolder( mock::FileSystemFactory::SubFolder );

    T->Reload();

    ASSERT_EQ( 2u, T->ml->files().size() );
    auto media = T->ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    auto f = T->ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );
    ASSERT_EQ( nullptr, media );
}

static void UpdateFile( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto filePath = mock::FileSystemFactory::SubFolder + "subfile.mp4";
    auto f = T->ml->media( filePath );
    ASSERT_NE( f, nullptr );
    auto id = f->id();

    auto fsFile = std::static_pointer_cast<mock::File>( T->fsMock->file( filePath ) );
    fsFile->markAsModified();

    T->Reload();

    f = T->ml->media( filePath );
    ASSERT_NE( nullptr, f );
    // File won't be refreshed since unittests don't have parsers (and the file
    // doesn't actually exist) but let's check it's not deleted/re-added anymore
    ASSERT_EQ( id, f->id() );
}

static void Ban( FolderTests* T )
{
    /* Attempt to ban a folder after its discovery and check that it stays banned */
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    T->ml->banFolder( mock::FileSystemFactory::SubFolder );
    T->cbMock->waitBanFolder();

    auto f = T->ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );

    T->ml->discover( mock::FileSystemFactory::Root );
    discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    f = T->ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );
}

static void DiscoverBanned( FolderTests* T )
{
    /* Attempt to ban a folder prior to its discovery */
    T->ml->banFolder( mock::FileSystemFactory::Root );
    T->cbMock->waitBanFolder();
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto f = T->ml->folder( mock::FileSystemFactory::Root );
    ASSERT_EQ( nullptr, f );
}

static void BanAfterDiscovery( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto f = std::static_pointer_cast<Folder>( T->ml->folder( mock::FileSystemFactory::SubFolder ) );
    ASSERT_NE( nullptr, f );
    auto files = f->files();
    ASSERT_NE( 0u, files.size() );

    T->ml->banFolder( mock::FileSystemFactory::SubFolder );
    T->cbMock->waitBanFolder();
    auto f2 = T->ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f2 );
}

static void RemoveFromBannedList( FolderTests* T )
{
    T->ml->banFolder( mock::FileSystemFactory::SubFolder );
    T->cbMock->waitBanFolder();
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
    auto files = T->ml->files();
    ASSERT_EQ( 2u, files.size() );

    auto f = T->ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );

    T->ml->unbanFolder( mock::FileSystemFactory::SubFolder );
    T->cbMock->waitUnbanFolder();
    T->cbMock->waitReload();
    files = T->ml->files();
    ASSERT_EQ( 3u, files.size() );
    f = T->ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_NE( nullptr, f );
}

static void BanTwice( FolderTests* T )
{
    T->ml->banFolder( mock::FileSystemFactory::SubFolder );
    T->cbMock->waitBanFolder();
    T->ml->banFolder( mock::FileSystemFactory::SubFolder );
    T->cbMock->waitBanFolder();
}

static void BanNonExistant( FolderTests* T )
{
    // Unhandled scheme
    T->ml->banFolder( "foo://bar/otters" );
    T->cbMock->waitBanFolder();
    // valid scheme, unknown root folder
    T->ml->banFolder( "file:///foo/bar/otters" );
    T->cbMock->waitBanFolder();
    // Ban with an existing base
    T->ml->banFolder( mock::FileSystemFactory::Root + "grouik/" );
    T->cbMock->waitBanFolder();
}

static void UnbanNonExistant( FolderTests* T )
{
    T->ml->unbanFolder( "foo/bar/otters" );
    T->cbMock->waitUnbanFolder();
    T->ml->unbanFolder( "/foo/bar/otters" );
    T->cbMock->waitUnbanFolder();
    // Ban with an existing base
    T->ml->unbanFolder( mock::FileSystemFactory::Root + "grouik/" );
    T->cbMock->waitUnbanFolder();
    // Ban existing but unbanned folder
    T->ml->unbanFolder( mock::FileSystemFactory::Root );
    T->cbMock->waitUnbanFolder();
}

static void NoMediaBeforeDiscovery( FolderTests* T )
{
    auto newFolder = mock::FileSystemFactory::Root + "newfolder/";
    T->fsMock->addFolder( newFolder );
    T->fsMock->addFile( newFolder + "newfile.avi" );
    T->fsMock->addFile( newFolder + ".nomedia" );

    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = T->ml->files();
    // We add 3 files before, and the new one shouldn't be account for since there is a .nomedia file
    ASSERT_EQ( 3u, files.size() );
}

static void InsertNoMedia( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = T->ml->files();
    ASSERT_EQ( 3u, files.size() );
    T->fsMock->addFile( mock::FileSystemFactory::SubFolder + ".nomedia" );

    T->Reload();

    files = T->ml->files();
    ASSERT_EQ( 2u, files.size() );
}

static void InsertNoMediaInRoot( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    T->fsMock->addFile( mock::FileSystemFactory::Root + ".nomedia" );

    T->Reload();

    auto files = T->ml->files();
    ASSERT_EQ( 0u, files.size() );
}

static void ReloadSubDir( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto files = T->ml->files();
    ASSERT_EQ( 3u, files.size() );
    T->fsMock->addFile( mock::FileSystemFactory::Root + "newmedia.mkv" );

    T->ml->reload( mock::FileSystemFactory::SubFolder );
    auto res = T->cbMock->waitReload();
    ASSERT_TRUE( res );

    files = T->ml->files();
    ASSERT_EQ( 3u, files.size() );

    T->ml->reload();
    res = T->cbMock->waitReload();
    ASSERT_TRUE( res );

    files = T->ml->files();
    ASSERT_EQ( 4u, files.size() );
}

static void FetchRoots( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto eps = T->ml->roots( nullptr )->all();
    ASSERT_EQ( 1u, eps.size() );
    ASSERT_EQ( mock::FileSystemFactory::Root, eps[0]->mrl() );

    // Check that banned folders don't appear in the results:
    T->ml->banFolder( mock::FileSystemFactory::SubFolder );
    auto res = T->cbMock->waitBanFolder();
    ASSERT_TRUE( res );
    eps = T->ml->roots( nullptr )->all();
    ASSERT_EQ( 1u, eps.size() );
}

static void RemoveRootRoot( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( T->ml.get() );
    auto media = T->ml->files();
    ASSERT_EQ( 3u, media.size() );
    auto video = T->ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 2u, video.size() );
    auto audio = T->ml->audioFiles( nullptr )->all();
    ASSERT_EQ( 1u, audio.size() );

    auto m = T->ml->media( mock::FileSystemFactory::Root + "video.avi" );
    ASSERT_NE( nullptr, m );
    ASSERT_FALSE( m->isExternalMedia() );
    m = T->ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    ASSERT_NE( nullptr, m );
    ASSERT_FALSE( m->isExternalMedia() );

    T->ml->removeRoot( mock::FileSystemFactory::Root );
    auto res = T->cbMock->waitRootRemoved();
    ASSERT_TRUE( res );

    media = T->ml->files();
    ASSERT_EQ( 3u, media.size() );

    video = T->ml->videoFiles( nullptr )->all();
    ASSERT_EQ( 0u, video.size() );
    audio = T->ml->audioFiles( nullptr )->all();
    ASSERT_EQ( 0u, audio.size() );

    /* The media should now be converted to an external media */
    m = T->ml->media( mock::FileSystemFactory::Root + "video.avi" );
    ASSERT_NE( nullptr, m );
    ASSERT_TRUE( m->isExternalMedia() );

    m = T->ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    ASSERT_NE( nullptr, m );
    ASSERT_TRUE( m->isExternalMedia() );

    auto eps = T->ml->roots( nullptr )->all();
    ASSERT_EQ( 0u, eps.size() );
}

static void RemoveRoot( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto media = T->ml->files();
    ASSERT_NE( 0u, media.size() );

    auto m = T->ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    ASSERT_NE( nullptr, m );
    ASSERT_FALSE( m->isExternalMedia() );

    T->ml->removeRoot( mock::FileSystemFactory::SubFolder );
    auto res = T->cbMock->waitRootRemoved();
    ASSERT_TRUE( res );

    media = T->ml->files();
    ASSERT_NE( 0u, media.size() );

    auto eps = T->ml->roots( nullptr )->all();
    ASSERT_EQ( 1u, eps.size() );

    m = T->ml->media( mock::FileSystemFactory::SubFolder + "subfile.mp4" );
    ASSERT_NE( nullptr, m );
    ASSERT_TRUE( m->isExternalMedia() );

    T->ml->reload();

    // Ensure it wasn't re-discovered, ie. that it was properly banned
    auto media2 = T->ml->files();
    ASSERT_EQ( media.size(), media2.size() );
}

static void RemoveNonExistantRoot( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    T->ml->removeRoot( "/sea/otter" );
    auto res = T->cbMock->waitRootRemoved();
    ASSERT_TRUE( res );
}

static void RemoveRootFolder( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u, T->ml->files().size() );

    T->fsMock->removeFolder( mock::FileSystemFactory::Root );

    T->Reload();

    ASSERT_EQ( 0u, T->ml->files().size() );
}

static void NbMedia( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( T->ml.get() );

    auto root = T->ml->folder( 1 );
    auto subFolder = T->ml->folder( 2 );
    ASSERT_EQ( "file:///a/", root->mrl() );
    ASSERT_EQ( "file:///a/folder/", subFolder->mrl() );
    ASSERT_EQ( 2u, root->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( 1u, subFolder->media( IMedia::Type::Unknown, nullptr )->count() );

    T->fsMock->removeFile( mock::FileSystemFactory::SubFolder + "subfile.mp4" );

    T->Reload();

    root = T->ml->folder( 1 );
    subFolder = T->ml->folder( 2 );

    ASSERT_EQ( 2u, root->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( 1u, root->media( IMedia::Type::Video, nullptr )->count() );
    ASSERT_EQ( 1u, root->media( IMedia::Type::Audio, nullptr )->count() );
    ASSERT_EQ( 0u, subFolder->media( IMedia::Type::Unknown, nullptr )->count() );

    auto videoMedia = root->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, videoMedia.size() );
    auto media = std::static_pointer_cast<Media>( videoMedia[0] );
    media->setType( IMedia::Type::Audio );

    videoMedia = root->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 0u, videoMedia.size() );
    ASSERT_EQ( 0u, root->media( IMedia::Type::Video, nullptr )->count() );

    auto audioMedia = root->media( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 2u, audioMedia.size() );
    ASSERT_EQ( 2u, root->media( IMedia::Type::Audio, nullptr )->count() );

    media->setType( IMedia::Type::Video );

    videoMedia = root->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, videoMedia.size() );
    ASSERT_EQ( 1u, root->media( IMedia::Type::Video, nullptr )->count() );

    audioMedia = root->media( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 1u, audioMedia.size() );
    ASSERT_EQ( 1u, root->media( IMedia::Type::Audio, nullptr )->count() );
}

static void NbMediaDeletionTrigger( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( T->ml.get() );

    auto root = T->ml->folder( 1 );
    ASSERT_EQ( "file:///a/", root->mrl() );
    ASSERT_EQ( 2u, root->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( 1u, T->ml->folders( IMedia::Type::Audio, nullptr )->count() );
    auto folders = T->ml->folders( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 1u, folders.size() );

    auto media = root->media( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    T->ml->deleteMedia( media[0]->id() );
    media = root->media( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    ASSERT_EQ( 0u, T->ml->folders( IMedia::Type::Audio, nullptr )->count() );
    folders = T->ml->folders( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 0u, folders.size() );
}

static void IsIndexedDiscovered( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    // Check with a couple of indexed folders
    auto res = T->ml->isIndexed( mock::FileSystemFactory::Root );
    ASSERT_TRUE( res );
    res = T->ml->isIndexed( mock::FileSystemFactory::SubFolder );
    ASSERT_TRUE( res );
    // Check with a random non-indexed folder
    res = T->ml->isIndexed( "file:///path/to/another/folder" );
    ASSERT_FALSE( res );
    // Check with a file
    res = T->ml->isIndexed( mock::FileSystemFactory::Root + "video.avi" );
    ASSERT_TRUE( res );
}

static void IsIndexedNonDiscovered( FolderTests* T )
{
    // The previous test checks for a non-existing folder. This time, try with
    // an existing folder that wasn't indexed
    T->ml->discover( mock::FileSystemFactory::SubFolder );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
    auto res = T->ml->isIndexed( mock::FileSystemFactory::Root );
    ASSERT_FALSE( res );
    res = T->ml->isIndexed( mock::FileSystemFactory::SubFolder );
    ASSERT_TRUE( res );
}

static void IsIndexedMultipleMountpoint( FolderTests* T )
{
    auto device = T->fsMock->device( mock::FileSystemFactory::Root );
    ASSERT_NE( nullptr, device );
    device->setRemovable( true );
    auto mp1 = std::string{ "file:///grouik/test/" };
    device->addMountpoint( mp1 );
    auto mp2 = std::string{ "file:///sea/otter/" };
    device->addMountpoint( mp2 );

    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto res = T->ml->isIndexed( mock::FileSystemFactory::SubFolder );
    ASSERT_TRUE( res );

    // Ensure we have the correct assumption for path manipulations
    ASSERT_EQ( mock::FileSystemFactory::SubFolder, mock::FileSystemFactory::Root + "folder/" );

    res = T->ml->isIndexed( mp1 + "folder/" );
    ASSERT_TRUE( res );

    res = T->ml->isIndexed( mp2 + "folder/" );
    ASSERT_TRUE( res );

    res = T->ml->isIndexed( "file:///this/path/is/not/valid/folder/" );
    ASSERT_FALSE( res );
}

static void IsBannedFolderIndexed( FolderTests* T )
{
    T->ml->banFolder( mock::FileSystemFactory::SubFolder );
    T->cbMock->waitBanFolder();
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
    auto res = T->ml->isIndexed( mock::FileSystemFactory::Root );
    ASSERT_TRUE( res );
    res = T->ml->isIndexed( mock::FileSystemFactory::SubFolder );
    ASSERT_FALSE( res );

    T->ml->unbanFolder( mock::FileSystemFactory::SubFolder );
    T->cbMock->waitUnbanFolder();
    T->cbMock->waitReload();
    res = T->ml->isIndexed( mock::FileSystemFactory::SubFolder );
    ASSERT_TRUE( res );
}

static void ListWithMedia( FolderTests* T )
{
    auto newFolder = mock::FileSystemFactory::Root + "empty/";
    T->fsMock->addFolder( newFolder );

    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( T->ml.get() );

    QueryParameters params{};
    params.sort = SortingCriteria::NbMedia;
    auto folders = T->ml->folders( IMedia::Type::Video, &params )->all();
    ASSERT_EQ( 2u, folders.size() );
    ASSERT_EQ( folders[0]->mrl(), mock::FileSystemFactory::Root );
    ASSERT_EQ( 2u, folders[0]->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( 1u, folders[0]->nbVideo() );
    ASSERT_EQ( 1u, folders[0]->nbAudio() );
    ASSERT_EQ( 2u, folders[0]->nbMedia() );
    ASSERT_EQ( folders[1]->mrl(), mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 1u, folders[1]->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( 1u, folders[1]->nbVideo() );
    ASSERT_EQ( 0u, folders[1]->nbAudio() );
    ASSERT_EQ( 1u, folders[1]->nbMedia() );

    // Keep in mind that this handles "desc" as "not the expected order"
    // ie. you'd expect the folder with the most media/video/audio first, so
    // desc = true will invert this.
    params.desc = true;
    folders = T->ml->folders( IMedia::Type::Video, &params )->all();
    ASSERT_EQ( 2u, folders.size() );
    ASSERT_EQ( folders[1]->mrl(), mock::FileSystemFactory::Root );
    ASSERT_EQ( 2u, folders[1]->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( folders[0]->mrl(), mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 1u, folders[0]->media( IMedia::Type::Unknown, nullptr )->count() );

    params.sort = SortingCriteria::NbAudio;
    folders = T->ml->folders( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, folders.size() );
    ASSERT_EQ( folders[0]->mrl(), mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( folders[1]->mrl(), mock::FileSystemFactory::Root );

    params.desc = false;
    folders = T->ml->folders( IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, folders.size() );
    ASSERT_EQ( folders[0]->mrl(), mock::FileSystemFactory::Root );
    ASSERT_EQ( folders[1]->mrl(), mock::FileSystemFactory::SubFolder );

    // List folders with audio media only
    auto query = T->ml->folders( IMedia::Type::Audio, &params );
    folders = query->all();
    ASSERT_EQ( 1u, query->count() );
    ASSERT_EQ( 1u, folders.size() );
    ASSERT_EQ( folders[0]->mrl(), mock::FileSystemFactory::Root );

    // Check the fetching of those media
    // For each query, test with count() and all()
    auto mediaQuery = folders[0]->media( IMedia::Type::Audio, &params );
    ASSERT_EQ( 1u, mediaQuery->count() );
    ASSERT_EQ( 1u, mediaQuery->all().size() );

    // Try again with a different sort, which triggers a more complex request
    params.sort = SortingCriteria::Artist;
    mediaQuery = folders[0]->media( IMedia::Type::Audio, &params );
    ASSERT_EQ( 1u, mediaQuery->count() );
    ASSERT_EQ( 1u, mediaQuery->all().size() );

    // But check that we still have all the media when we filter with 'Unknown'
    mediaQuery = folders[0]->media( IMedia::Type::Unknown, nullptr );
    ASSERT_EQ( 2u, mediaQuery->count() );
    ASSERT_EQ( 2u, mediaQuery->all().size() );

    // Now try sorting by last modified date, which was causing a crash
    params.sort = SortingCriteria::LastModificationDate;
    mediaQuery = folders[0]->media( IMedia::Type::Unknown, &params );
    ASSERT_EQ( 2u, mediaQuery->count() );
    ASSERT_EQ( 2u, mediaQuery->all().size() );

    mediaQuery = folders[0]->media( IMedia::Type::Audio, &params );
    ASSERT_EQ( 1u, mediaQuery->count() );
    ASSERT_EQ( 1u, mediaQuery->all().size() );
}

static void SearchMedia( FolderTests* T )
{
    auto newFolder = mock::FileSystemFactory::Root + "empty/";
    T->fsMock->addFolder( newFolder );

    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( T->ml.get() );

    auto folder = T->ml->folder( mock::FileSystemFactory::Root );
    ASSERT_NE( nullptr, folder );

    auto videosQuery = folder->searchMedia( "video", IMedia::Type::Video, nullptr );
    ASSERT_EQ( 1u, videosQuery->count() );
    auto videos = videosQuery->all();
    ASSERT_EQ( 1u, videos.size() );

    auto audioQuery = folder->searchMedia( "audio", IMedia::Type::Audio, nullptr );
    ASSERT_EQ( 1u, audioQuery->count() );
    auto audios = audioQuery->all();
    ASSERT_EQ( 1u, audios.size() );

    QueryParameters params;
    params.sort = SortingCriteria::Alpha;
    params.desc = true;
    auto allTypeQuery = folder->searchMedia( "video", IMedia::Type::Unknown, &params );
    ASSERT_EQ( 1u, allTypeQuery->count() );
    auto all = allTypeQuery->all();
    ASSERT_EQ( 1u, all.size() );
}

static void ListSubFolders( FolderTests* T )
{
    auto newFolder = mock::FileSystemFactory::Root + "empty/";
    T->fsMock->addFolder( newFolder );

    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( T->ml.get() );

    auto roots = T->ml->roots( nullptr )->all();
    ASSERT_EQ( 1u, roots.size() );

    auto root = roots[0];
    QueryParameters params{};
    params.sort = SortingCriteria::NbMedia;
    auto rootSubFolders = root->subfolders( &params )->all();
    ASSERT_EQ( 2u, rootSubFolders.size() );
    ASSERT_EQ( mock::FileSystemFactory::SubFolder, rootSubFolders[0]->mrl() );
    auto sfMedia = rootSubFolders[0]->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_EQ( 1u, sfMedia.size() );
    ASSERT_EQ( newFolder, rootSubFolders[1]->mrl() );
    ASSERT_EQ( 0u, rootSubFolders[1]->media( IMedia::Type::Unknown, nullptr )->count() );

    auto media = std::static_pointer_cast<Media>( sfMedia[0] );
    media->setType( IMedia::Type::Video );

    // Check fetching by type now
    ASSERT_EQ( 0u, rootSubFolders[0]->media( IMedia::Type::Audio, nullptr )->count() );
    ASSERT_EQ( 1u, rootSubFolders[0]->media( IMedia::Type::Video, nullptr )->count() );
    // Double check with a fetch all instead of counting
    auto allMedia = rootSubFolders[0]->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, allMedia.size() );
    ASSERT_EQ( media->id(), allMedia[0]->id() );
}

static void SearchFolders( FolderTests* T )
{
    // Add an empty folder matching the search pattern
    auto newFolder = mock::FileSystemFactory::Root + "empty/folder/";
    T->fsMock->addFolder( newFolder );
    // Add a non empty sub folder also matching the pattern
    auto newSubFolder = mock::FileSystemFactory::Root + "empty/folder/fold/";
    T->fsMock->addFolder( newSubFolder );
    T->fsMock->addFile( newSubFolder + "some file.avi" );
    T->fsMock->addFile( newSubFolder + "some other file.avi" );

    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( T->ml.get() );
    auto media = std::static_pointer_cast<Media>(
                                T->ml->media( newSubFolder + "some file.avi" ) );
    media->setType( IMedia::Type::Video );

    media = std::static_pointer_cast<Media>(
                        T->ml->media( newSubFolder + "some other file.avi" ) );
    media->setType( IMedia::Type::Video );

    QueryParameters params{};
    params.sort = SortingCriteria::NbMedia;
    auto folders = T->ml->searchFolders( "fold", IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, folders.size() );
    ASSERT_EQ( newSubFolder, folders[0]->mrl() );
    ASSERT_EQ( mock::FileSystemFactory::SubFolder, folders[1]->mrl() );
}

static void Name( FolderTests* T )
{
    auto newFolder = mock::FileSystemFactory::SubFolder + "folder%20with%20spaces/";
    T->fsMock->addFolder( newFolder );

    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto root = T->ml->folder( 1 );
    auto subFolder = T->ml->folder( 2 );
    auto spacesFolder = T->ml->folder( 3 );
    ASSERT_EQ( "a", root->name() );
    ASSERT_EQ( "folder", subFolder->name() );
    ASSERT_EQ( "folder with spaces", spacesFolder->name() );
    ASSERT_EQ( newFolder, spacesFolder->mrl() );
}

static void IsBanned( FolderTests* T )
{
    auto res = T->ml->isBanned( mock::FileSystemFactory::Root );
    ASSERT_FALSE( res );
    T->ml->banFolder( mock::FileSystemFactory::Root );
    T->cbMock->waitBanFolder();
    res = T->ml->isBanned( mock::FileSystemFactory::Root );
    ASSERT_TRUE( res );

    res = T->ml->isBanned( "not even an mrl" );
    ASSERT_FALSE( res );
}

static void BannedRoots( FolderTests* T )
{
    auto res = T->ml->bannedRoots();
    ASSERT_NE( nullptr, res );
    ASSERT_EQ( 0u, res->all().size() );
    ASSERT_EQ( 0u, res->count() );

    T->ml->banFolder( mock::FileSystemFactory::SubFolder );
    T->cbMock->waitBanFolder();

    res = T->ml->bannedRoots();
    ASSERT_NE( nullptr, res );
    ASSERT_EQ( 1u, res->all().size() );
    ASSERT_EQ( 1u, res->count() );
    ASSERT_EQ( mock::FileSystemFactory::SubFolder, res->all()[0]->mrl() );

    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    res = T->ml->bannedRoots();
    ASSERT_NE( nullptr, res );
    ASSERT_EQ( 1u, res->all().size() );
    ASSERT_EQ( 1u, res->count() );
    ASSERT_EQ( mock::FileSystemFactory::SubFolder, res->all()[0]->mrl() );

    res = T->ml->roots( nullptr );
    ASSERT_NE( nullptr, res );
    ASSERT_EQ( 1u, res->all().size() );
    ASSERT_EQ( 1u, res->count() );
    ASSERT_EQ( mock::FileSystemFactory::Root, res->all()[0]->mrl() );
}

static void CheckDbModel( FolderTests* T )
{
    auto res = Folder::checkDbModel( T->ml.get() );
    ASSERT_TRUE( res );
}

static void NbMediaAfterExternalInternalConversion( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( T->ml.get() );

    auto subFolder = T->ml->folder( 2 );
    ASSERT_EQ( "file:///a/folder/", subFolder->mrl() );
    auto media = subFolder->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ASSERT_EQ( 1u, subFolder->nbVideo() );

    auto m = std::static_pointer_cast<Media>( media[0] );
    auto res = m->convertToExternal();
    ASSERT_TRUE( res );

    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_EQ( 0u, subFolder->nbVideo() );

    m->markAsInternal( IMedia::Type::Video, 0,
                       static_cast<Folder*>( subFolder.get() )->deviceId(), subFolder->id() );
    res = T->ml->setMediaFolderId( m->id(), subFolder->id() );
    ASSERT_TRUE( res );

    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_EQ( 1u, subFolder->nbVideo() );
}

static void Duration( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto root = T->ml->folder( 1 );
    ASSERT_NON_NULL( root );
    ASSERT_EQ( 0u, root->duration() );
    auto subFolder = T->ml->folder( 2 );
    ASSERT_NON_NULL( subFolder );
    ASSERT_EQ( 0u, subFolder->duration() );

    auto media = subFolder->media( IMedia::Type::Unknown, nullptr )->all();
    ASSERT_TRUE( media.size() >= 1 );
    auto m = std::static_pointer_cast<Media>( media[0] );
    m->setDuration( 1234 );

    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_EQ( 1234, subFolder->duration() );

    m->setDuration( -1 );
    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_EQ( 0, subFolder->duration() );

    m->setDuration( 4321 );
    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_EQ( 4321, subFolder->duration() );

    T->ml->deleteMedia( m->id() );
    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_EQ( 0, subFolder->duration() );

}

static void SetPublic( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto checkMediaPublicness = []( const IFolder& f, bool expectedPublicness ) {
        auto media = f.media( IMedia::Type::Unknown, nullptr )->all();
        ASSERT_FALSE( media.empty() );
        for ( const auto& m : media )
            ASSERT_EQ( m->isPublic(), expectedPublicness );
    };

    auto root = T->ml->folder( 1 );
    ASSERT_NON_NULL( root );
    ASSERT_FALSE( root->isPublic() );
    auto subFolder = T->ml->folder( 2 );
    ASSERT_NON_NULL( subFolder );
    ASSERT_FALSE( subFolder->isPublic() );
    checkMediaPublicness( *subFolder, false );

    auto res = subFolder->setPublic( true );
    ASSERT_TRUE( res );
    ASSERT_TRUE( subFolder->isPublic() );
    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_TRUE( subFolder->isPublic() );
    checkMediaPublicness( *subFolder, true );

    /* Ensure publicness didn't propagate to parent folders */
    root = T->ml->folder( root->id() );
    ASSERT_FALSE( root->isPublic() );
    checkMediaPublicness( *root, false );

    /* Ensure we can list public "root" folders without fetching the actual entry point */
    QueryParameters params{};
    params.publicOnly = true;
    auto rootsQuery = T->ml->roots( &params );
    ASSERT_EQ( 1u, rootsQuery->count() );
    auto roots = rootsQuery->all();
    ASSERT_EQ( 1u, roots.size() );
    ASSERT_EQ( roots[0]->id(), subFolder->id() );

    /*
     * Set the subfolder back to private and check for propagations through the
     * parent folder
     */
    res = subFolder->setPublic( false );
    ASSERT_TRUE( res );
    ASSERT_FALSE( subFolder->isPublic() );
    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_FALSE ( subFolder->isPublic() );
    checkMediaPublicness( *subFolder, false );

    root = T->ml->folder( root->id() );
    ASSERT_FALSE( root->isPublic() );
    checkMediaPublicness( *root, false );

    rootsQuery = T->ml->roots( &params );
    ASSERT_EQ( 0u, rootsQuery->count() );
    roots = rootsQuery->all();
    ASSERT_EQ( 0u, roots.size() );

    res = root->setPublic( true );
    ASSERT_TRUE( res );
    ASSERT_TRUE( root->isPublic() );
    root = T->ml->folder( root->id() );
    ASSERT_TRUE( root->isPublic() );
    checkMediaPublicness( *root, true );

    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_TRUE( subFolder->isPublic() );
    checkMediaPublicness( *subFolder, true );

    /* Check that roots() will return the actual entrypoint if public */
    rootsQuery = T->ml->roots( &params );
    ASSERT_EQ( 1u, rootsQuery->count() );
    roots = rootsQuery->all();
    ASSERT_EQ( 1u, roots.size() );
    ASSERT_EQ( roots[0]->id(), root->id() );

    res = root->setPublic( false );
    ASSERT_TRUE( res );
    ASSERT_FALSE( root->isPublic() );
    root = T->ml->folder( root->id() );
    ASSERT_FALSE( root->isPublic() );
    checkMediaPublicness( *root, false );

    subFolder = T->ml->folder( subFolder->id() );
    ASSERT_FALSE( subFolder->isPublic() );
    checkMediaPublicness( *subFolder, false );
}

static void Favorite( FolderTests* T )
{
    T->ml->discover( mock::FileSystemFactory::Root );
    bool discovered = T->cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto root = T->ml->folder( 1 );
    ASSERT_NON_NULL( root );

    ASSERT_FALSE( root->isFavorite() );

    QueryParameters params;
    params.favoriteOnly = true;
    auto query = T->ml->roots( &params );
    ASSERT_EQ( query->count(), 0u );

    root->setFavorite( true );
    ASSERT_TRUE( root->isFavorite() );

    root = T->ml->folder( 1 );
    ASSERT_TRUE( root->isFavorite() );

    auto res = T->ml->roots( &params )->all();
    ASSERT_EQ( res.size(), 1u );
    ASSERT_EQ( res[0]->id(), root->id() );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( FolderTests );

    ADD_TEST( Add );
    ADD_TEST( Load );
    ADD_TEST( InvalidPath );
    ADD_TEST( List );
    ADD_TEST( ListFolders );
    ADD_TEST( NewFolderWithFile );
    ADD_TEST( NewFileInSubFolder );
    ADD_TEST( RemoveFileFromDirectory );
    ADD_TEST( RemoveDirectory );
    ADD_TEST( UpdateFile );
    ADD_TEST( Ban );
    ADD_TEST( DiscoverBanned );
    ADD_TEST( BanAfterDiscovery );
    ADD_TEST( RemoveFromBannedList );
    ADD_TEST( BanTwice );
    ADD_TEST( BanNonExistant );
    ADD_TEST( UnbanNonExistant );
    ADD_TEST( NoMediaBeforeDiscovery );
    ADD_TEST( InsertNoMedia );
    ADD_TEST( InsertNoMediaInRoot );
    ADD_TEST( ReloadSubDir );
    ADD_TEST( FetchRoots );
    ADD_TEST( RemoveRootRoot );
    ADD_TEST( RemoveRoot );
    ADD_TEST( RemoveNonExistantRoot );
    ADD_TEST( RemoveRootFolder );
    ADD_TEST( NbMedia );
    ADD_TEST( NbMediaDeletionTrigger );
    ADD_TEST( IsIndexedDiscovered );
    ADD_TEST( IsIndexedNonDiscovered );
    ADD_TEST( IsIndexedMultipleMountpoint );
    ADD_TEST( IsBannedFolderIndexed );
    ADD_TEST( ListWithMedia );
    ADD_TEST( SearchMedia );
    ADD_TEST( ListSubFolders );
    ADD_TEST( SearchFolders );
    ADD_TEST( Name );
    ADD_TEST( IsBanned );
    ADD_TEST( BannedRoots );
    ADD_TEST( CheckDbModel );
    ADD_TEST( NbMediaAfterExternalInternalConversion );
    ADD_TEST( Duration );
    ADD_TEST( SetPublic );
    ADD_TEST( Favorite );

    END_TESTS
}
