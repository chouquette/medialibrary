#include "Tests.h"

#include "IFile.h"
#include "IFolder.h"
#include "IMediaLibrary.h"
#include "Utils.h"
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

        virtual void Reload()
        {
            Tests::Reload( fsMock, cbMock.get() );
        }
};

TEST_F( Folders, Add )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto files = ml->files();

    ASSERT_EQ( files.size(), 3u );
    ASSERT_FALSE( files[0]->isStandAlone() );
}

TEST_F( Folders, Delete )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::Root );
    auto folderPath = f->path();

    auto files = ml->files();
    ASSERT_EQ( files.size(), 3u );

    auto filePath = files[0]->mrl();

    ml->deleteFolder( f );

    f = ml->folder( folderPath );
    ASSERT_EQ( nullptr, f );

    files = ml->files();
    ASSERT_EQ( files.size(), 0u );

    // Check the file isn't cached anymore:
    auto file = ml->file( filePath );
    ASSERT_EQ( nullptr, file );

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
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 3u );
    for ( auto& f : files )
        ASSERT_FALSE( f->isStandAlone() );
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
    ml->discover( "." );
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

TEST_F( Folders, AbsolutePath )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( "." );
    ASSERT_EQ( f, nullptr );
}

TEST_F( Folders, ListFolders )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
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
    ASSERT_EQ( std::string{ mock::FileSystemFactory::SubFolder } + "subfile.mp4", file->mrl() );

    // Now again, without cache. No need to wait for fs discovery reload here
    Reload();

    f = ml->folder( f->path() );
    subFolders = f->folders();
    ASSERT_EQ( 1u, subFolders.size() );

    subFolder = subFolders[0];
    subFiles = subFolder->files();
    ASSERT_EQ( 1u, subFiles.size() );

    file = subFiles[0];
    ASSERT_EQ( std::string{ mock::FileSystemFactory::SubFolder } + "subfile.mp4", file->mrl() );
}

TEST_F( Folders, LastModificationDate )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_NE( 0u, f->lastModificationDate() );
    auto subFolders = f->folders();
    ASSERT_NE( 0u, subFolders[0]->lastModificationDate() );

    Reload();

    f = ml->folder( f->path() );
    ASSERT_NE( 0u, f->lastModificationDate() );
    subFolders = f->folders();
    ASSERT_NE( 0u, subFolders[0]->lastModificationDate() );
}

TEST_F( Folders, NewFolderWithFile )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    auto newFolder = std::string(mock::FileSystemFactory::Root) + "newfolder/";
    fsMock->addFolder( mock::FileSystemFactory::Root, "newfolder/", time( nullptr ) );
    fsMock->addFile( newFolder, "newfile.avi" );

    // This will trigger a reload
    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 4u, ml->files().size() );
    auto file = ml->file( newFolder + "newfile.avi" );
    ASSERT_NE( nullptr, file );
}

// This is expected to fail until we fix the file system modifications detection
TEST_F( Folders, NewFileInSubFolder )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_EQ( 3u, ml->files().size() );

    f = ml->folder( mock::FileSystemFactory::SubFolder );
    auto lmd = f->lastModificationDate();
    // Do not watch for live changes
    ml.reset();
    fsMock->addFile( mock::FileSystemFactory::SubFolder, "newfile.avi" );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 4u, ml->files().size() );
    auto file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "newfile.avi" );
    f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 2u, f->files().size() );
    ASSERT_NE( nullptr, file );
    ASSERT_FALSE( file->isStandAlone() );
    ASSERT_NE( lmd, f->lastModificationDate() );
}

TEST_F( Folders, RemoveFileFromDirectory )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    fsMock->removeFile( mock::FileSystemFactory::SubFolder, "subfile.mp4" );

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    ASSERT_EQ( 2u, ml->files().size() );
    auto file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( 0u, f->files().size() );
    ASSERT_EQ( nullptr, file );
}

TEST_F( Folders, RemoveDirectory )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
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
    auto file = ml->file( std::string( mock::FileSystemFactory::SubFolder ) + "subfile.mp4" );
    auto f = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_EQ( nullptr, f );
    ASSERT_EQ( nullptr, file );
}

TEST_F( Folders, UpdateFile )
{
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto filePath = std::string{ mock::FileSystemFactory::SubFolder } + "subfile.mp4";
    auto f = ml->file( filePath );
    ASSERT_NE( f, nullptr );
    auto id = f->id();

    ml.reset();
    fsMock->files[filePath]->markAsModified();
    fsMock->dirs[mock::FileSystemFactory::SubFolder]->markAsModified();

    cbMock->prepareForReload();
    Reload();
    bool reloaded = cbMock->waitForReload();
    ASSERT_TRUE( reloaded );

    f = ml->file( filePath );
    ASSERT_NE( nullptr, f );
    // The file is expected to be deleted and re-added since it changed, so the
    // id should have changed
    ASSERT_NE( id, f->id() );
}

// This simply tests that the flag is properly stored in db
TEST_F( Folders, CheckRemovable )
{
    fsMock->dirs[mock::FileSystemFactory::SubFolder]->markRemovable();
    cbMock->prepareForWait( 1 );
    ml->discover( "." );
    bool discovered = cbMock->wait();
    ASSERT_TRUE( discovered );

    auto f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_NE( f, nullptr );
    ASSERT_FALSE( f->isRemovable() );
    auto subfolder = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_NE( subfolder, nullptr );
    ASSERT_TRUE( subfolder->isRemovable() );

    // No actual FS change, no need to wait for the actual FS reload
    Reload();

    f = ml->folder( mock::FileSystemFactory::Root );
    ASSERT_FALSE( f->isRemovable() );
    subfolder = ml->folder( mock::FileSystemFactory::SubFolder );
    ASSERT_TRUE( subfolder->isRemovable() );
}
