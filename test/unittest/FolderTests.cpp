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
#include "Folder.h"
#include "medialibrary/IMediaLibrary.h"
#include "utils/Filename.h"
#include "utils/Url.h"
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
    auto fsFile = std::static_pointer_cast<mock::File>( fsMock->file( filePath ) );
    fsFile->markAsModified();

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
    cbMock->waitUnbanFolder();
    cbMock->waitReload();
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
    // Unhandled scheme
    ml->banFolder( "foo://bar/otters" );
    cbMock->waitBanFolder();
    // valid scheme, unknown root folder
    ml->banFolder( "file:///foo/bar/otters" );
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

static void enforceFakeMediaTypes( MediaLibraryTester* ml )
{
    auto media = std::static_pointer_cast<Media>( ml->media(
                                mock::FileSystemFactory::Root + "video.avi" ) );
    media->setType( IMedia::Type::Video );
    media->save();

    media = std::static_pointer_cast<Media>( ml->media(
                                    mock::FileSystemFactory::Root + "audio.mp3" ) );
    media->setType( IMedia::Type::Audio );
    media->save();

    media = std::static_pointer_cast<Media>( ml->media(
                                    mock::FileSystemFactory::SubFolder + "subfile.mp4" ) );
    media->setType( IMedia::Type::Video );
    media->save();
}

TEST_F( Folders, NbMedia )
{
    enforceFakeMediaTypes( ml.get() );

    auto root = ml->folder( 1 );
    auto subFolder = ml->folder( 2 );
    ASSERT_EQ( "file:///a/", root->mrl() );
    ASSERT_EQ( "file:///a/folder/", subFolder->mrl() );
    ASSERT_EQ( 2u, root->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( 1u, subFolder->media( IMedia::Type::Unknown, nullptr )->count() );

    // Do not watch for live changes
    ml.reset();
    fsMock->removeFile( mock::FileSystemFactory::SubFolder + "subfile.mp4" );

    Reload();

    root = ml->folder( 1 );
    subFolder = ml->folder( 2 );

    ASSERT_EQ( 2u, root->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( 1u, root->media( IMedia::Type::Video, nullptr )->count() );
    ASSERT_EQ( 1u, root->media( IMedia::Type::Audio, nullptr )->count() );
    ASSERT_EQ( 0u, subFolder->media( IMedia::Type::Unknown, nullptr )->count() );

    auto videoMedia = root->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, videoMedia.size() );
    auto media = std::static_pointer_cast<Media>( videoMedia[0] );
    media->setType( IMedia::Type::Audio );
    media->save();

    videoMedia = root->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 0u, videoMedia.size() );
    ASSERT_EQ( 0u, root->media( IMedia::Type::Video, nullptr )->count() );

    auto audioMedia = root->media( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 2u, audioMedia.size() );
    ASSERT_EQ( 2u, root->media( IMedia::Type::Audio, nullptr )->count() );

    media->setType( IMedia::Type::Video );
    media->save();

    videoMedia = root->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, videoMedia.size() );
    ASSERT_EQ( 1u, root->media( IMedia::Type::Video, nullptr )->count() );

    audioMedia = root->media( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 1u, audioMedia.size() );
    ASSERT_EQ( 1u, root->media( IMedia::Type::Audio, nullptr )->count() );
}

TEST_F( Folders, NbMediaDeletionTrigger )
{
    enforceFakeMediaTypes( ml.get() );

    auto root = ml->folder( 1 );
    ASSERT_EQ( "file:///a/", root->mrl() );
    ASSERT_EQ( 2u, root->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( 1u, ml->folders( IMedia::Type::Audio, nullptr )->count() );
    auto folders = ml->folders( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 1u, folders.size() );

    auto media = root->media( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 1u, media.size() );
    ml->deleteMedia( media[0]->id() );
    media = root->media( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 0u, media.size() );

    ASSERT_EQ( 0u, ml->folders( IMedia::Type::Audio, nullptr )->count() );
    folders = ml->folders( IMedia::Type::Audio, nullptr )->all();
    ASSERT_EQ( 0u, folders.size() );
}

TEST_F( Folders, IsIndexed )
{
    // Check with a couple of indexed folders
    auto res = ml->isIndexed( mock::FileSystemFactory::Root );
    ASSERT_TRUE( res );
    res = ml->isIndexed( mock::FileSystemFactory::SubFolder );
    ASSERT_TRUE( res );
    // Check with a random non-indexed folder
    res = ml->isIndexed( "file:///path/to/another/folder" );
    ASSERT_FALSE( res );
    // Check with a file
    res = ml->isIndexed( mock::FileSystemFactory::Root + "video.avi" );
    ASSERT_TRUE( res );
}

TEST_F( FoldersNoDiscover, IsIndexed )
{
    // The previous test checks for a non-existing folder. This time, try with
    // an existing folder that wasn't indexed
    ml->discover( mock::FileSystemFactory::SubFolder );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
    auto res = ml->isIndexed( mock::FileSystemFactory::Root );
    ASSERT_FALSE( res );
    res = ml->isIndexed( mock::FileSystemFactory::SubFolder );
    ASSERT_TRUE( res );
}

TEST_F( FoldersNoDiscover, IsBannedFolderIndexed )
{
    ml->banFolder( mock::FileSystemFactory::SubFolder );
    cbMock->waitBanFolder();
    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );
    auto res = ml->isIndexed( mock::FileSystemFactory::Root );
    ASSERT_TRUE( res );
    res = ml->isIndexed( mock::FileSystemFactory::SubFolder );
    ASSERT_FALSE( res );

    ml->unbanFolder( mock::FileSystemFactory::SubFolder );
    cbMock->waitUnbanFolder();
    cbMock->waitReload();
    res = ml->isIndexed( mock::FileSystemFactory::SubFolder );
    ASSERT_TRUE( res );
}

TEST_F( FoldersNoDiscover, ListWithMedia )
{
    auto newFolder = mock::FileSystemFactory::Root + "empty/";
    fsMock->addFolder( newFolder );

    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( ml.get() );

    QueryParameters params{};
    params.sort = SortingCriteria::NbMedia;
    auto folders = ml->folders( IMedia::Type::Video, &params )->all();
    ASSERT_EQ( 2u, folders.size() );
    ASSERT_EQ( folders[0]->mrl(), mock::FileSystemFactory::Root );
    ASSERT_EQ( 2u, folders[0]->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( folders[1]->mrl(), mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 1u, folders[1]->media( IMedia::Type::Unknown, nullptr )->count() );

    params.desc = true;
    folders = ml->folders( IMedia::Type::Video, &params )->all();
    ASSERT_EQ( 2u, folders.size() );
    ASSERT_EQ( folders[1]->mrl(), mock::FileSystemFactory::Root );
    ASSERT_EQ( 2u, folders[1]->media( IMedia::Type::Unknown, nullptr )->count() );
    ASSERT_EQ( folders[0]->mrl(), mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 1u, folders[0]->media( IMedia::Type::Unknown, nullptr )->count() );

    // List folders with audio media only
    auto query = ml->folders( IMedia::Type::Audio, &params );
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

TEST_F( FoldersNoDiscover, SearchMedia )
{
    auto newFolder = mock::FileSystemFactory::Root + "empty/";
    fsMock->addFolder( newFolder );

    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( ml.get() );

    auto folder = ml->folder( mock::FileSystemFactory::Root );
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

TEST_F( FoldersNoDiscover, ListSubFolders )
{
    auto newFolder = mock::FileSystemFactory::Root + "empty/";
    fsMock->addFolder( newFolder );

    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( ml.get() );

    auto entryPoints = ml->entryPoints()->all();
    ASSERT_EQ( 1u, entryPoints.size() );

    auto root = entryPoints[0];
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
    media->save();

    // Check fetching by type now
    ASSERT_EQ( 0u, rootSubFolders[0]->media( IMedia::Type::Audio, nullptr )->count() );
    ASSERT_EQ( 1u, rootSubFolders[0]->media( IMedia::Type::Video, nullptr )->count() );
    // Double check with a fetch all instead of counting
    auto allMedia = rootSubFolders[0]->media( IMedia::Type::Video, nullptr )->all();
    ASSERT_EQ( 1u, allMedia.size() );
    ASSERT_EQ( media->id(), allMedia[0]->id() );
}

TEST_F( FoldersNoDiscover, SearchFolders )
{
    // Add an empty folder matching the search pattern
    auto newFolder = mock::FileSystemFactory::Root + "empty/folder/";
    fsMock->addFolder( newFolder );
    // Add a non empty sub folder also matching the pattern
    auto newSubFolder = mock::FileSystemFactory::Root + "empty/folder/fold/";
    fsMock->addFolder( newSubFolder );
    fsMock->addFile( newSubFolder + "some file.avi" );
    fsMock->addFile( newSubFolder + "some other file.avi" );

    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    enforceFakeMediaTypes( ml.get() );
    auto media = std::static_pointer_cast<Media>(
                                ml->media( newSubFolder + "some file.avi" ) );
    media->setType( IMedia::Type::Video );
    media->save();

    media = std::static_pointer_cast<Media>(
                        ml->media( newSubFolder + "some other file.avi" ) );
    media->setType( IMedia::Type::Video );
    media->save();

    QueryParameters params{};
    params.sort = SortingCriteria::NbMedia;
    auto folders = ml->searchFolders( "fold", IMedia::Type::Unknown, &params )->all();
    ASSERT_EQ( 2u, folders.size() );
    ASSERT_EQ( newSubFolder, folders[0]->mrl() );
    ASSERT_EQ( mock::FileSystemFactory::SubFolder, folders[1]->mrl() );
}

TEST_F( FoldersNoDiscover, Name )
{
    auto newFolder = mock::FileSystemFactory::SubFolder + "folder%20with%20spaces/";
    fsMock->addFolder( newFolder );

    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    auto root = ml->folder( 1 );
    auto subFolder = ml->folder( 2 );
    auto spacesFolder = ml->folder( 3 );
    ASSERT_EQ( "a", root->name() );
    ASSERT_EQ( "folder", subFolder->name() );
    ASSERT_EQ( "folder with spaces", spacesFolder->name() );
    ASSERT_EQ( newFolder, spacesFolder->mrl() );
}

TEST_F( FoldersNoDiscover, IsBanned )
{
    auto res = ml->isBanned( mock::FileSystemFactory::Root );
    ASSERT_FALSE( res );
    ml->banFolder( mock::FileSystemFactory::Root );
    cbMock->waitBanFolder();
    res = ml->isBanned( mock::FileSystemFactory::Root );
    ASSERT_TRUE( res );

    res = ml->isBanned( "not even an mrl" );
    ASSERT_FALSE( res );
}

TEST_F( FoldersNoDiscover, BannedEntryPoints )
{
    auto res = ml->bannedEntryPoints();
    ASSERT_NE( nullptr, res );
    ASSERT_EQ( 0u, res->all().size() );
    ASSERT_EQ( 0u, res->count() );

    ml->banFolder( mock::FileSystemFactory::SubFolder );
    cbMock->waitBanFolder();

    res = ml->bannedEntryPoints();
    ASSERT_NE( nullptr, res );
    ASSERT_EQ( 1u, res->all().size() );
    ASSERT_EQ( 1u, res->count() );
    ASSERT_EQ( mock::FileSystemFactory::SubFolder, res->all()[0]->mrl() );

    ml->discover( mock::FileSystemFactory::Root );
    bool discovered = cbMock->waitDiscovery();
    ASSERT_TRUE( discovered );

    res = ml->bannedEntryPoints();
    ASSERT_NE( nullptr, res );
    ASSERT_EQ( 1u, res->all().size() );
    ASSERT_EQ( 1u, res->count() );
    ASSERT_EQ( mock::FileSystemFactory::SubFolder, res->all()[0]->mrl() );

    res = ml->entryPoints();
    ASSERT_NE( nullptr, res );
    ASSERT_EQ( 1u, res->all().size() );
    ASSERT_EQ( 1u, res->count() );
    ASSERT_EQ( mock::FileSystemFactory::Root, res->all()[0]->mrl() );
}
