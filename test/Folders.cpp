#include "gtest/gtest.h"

#include "IFile.h"
#include "IFolder.h"
#include "IMediaLibrary.h"

#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "Utils.h"

#include <memory>
#include <unordered_map>

namespace mock
{

class File : public fs::IFile
{
public:
    File( const std::string& filePath )
        : m_name( utils::file::fileName( filePath ) )
        , m_path( utils::file::directory( filePath ) )
        , m_fullPath( filePath )
        , m_extension( utils::file::extension( filePath ) )
        , m_lastModification( 0 )
    {
    }

    File( const File& ) = default;

    virtual const std::string& name() const override
    {
        return m_name;
    }

    virtual const std::string& path() const override
    {
        return m_path;
    }

    virtual const std::string& fullPath() const override
    {
        return m_fullPath;
    }

    virtual const std::string& extension() const override
    {
        return m_extension;
    }

    virtual unsigned int lastModificationDate() const override
    {
        return m_lastModification;
    }

    std::string m_name;
    std::string m_path;
    std::string m_fullPath;
    std::string m_extension;
    unsigned int m_lastModification;
};

class Directory : public fs::IDirectory
{
public:
    Directory( std::shared_ptr<mock::Directory> parent, const std::string& path, unsigned int lastModif )
        : m_path( path )
        , m_parent( parent )
        , m_lastModificationDate( lastModif )
    {
    }

    virtual const std::string& path() const
    {
        return m_path;
    }

    virtual const std::vector<std::string>& files() const
    {
        return m_files;
    }

    virtual const std::vector<std::string>& dirs() const
    {
        return m_dirs;
    }

    virtual unsigned int lastModificationDate() const override
    {
        return m_lastModificationDate;
    }

    void addFile( const std::string& fileName )
    {
        m_files.emplace_back( m_path + fileName );
        markAsModified();
    }

    void addFolder( const std::string& folder )
    {
        m_dirs.emplace_back( m_path + folder );
        markAsModified();
    }

    void markAsModified()
    {
        if ( m_parent != nullptr )
            m_parent->markAsModified();
        m_lastModificationDate++;
    }

private:
    std::string m_path;
    std::vector<std::string> m_files;
    std::vector<std::string> m_dirs;
    std::shared_ptr<mock::Directory> m_parent;
    unsigned int m_lastModificationDate;
};


struct FileSystemFactory : public factory::IFileSystem
{
    static constexpr const char* Root = "/a/";
    static constexpr const char* SubFolder = "/a/folder/";

    FileSystemFactory()
    {
        dirs[Root] = std::unique_ptr<mock::Directory>( new Directory{ nullptr, Root, 123 } );
            addFile( Root, "video.avi" );
            addFile( Root, "audio.mp3" );
            addFile( Root, "not_a_media.something" );
            addFile( Root, "some_other_file.seaotter" );
            addFolder( Root, "folder/", 456 );
                addFile( SubFolder, "subfile.mp4" );
    }

    void addFile( const std::string& path, const std::string& fileName )
    {
        dirs[path]->addFile( fileName );
        files[path + fileName] = std::unique_ptr<mock::File>( new mock::File( path + fileName ) );
    }

    void addFolder( const std::string& parentPath, const std::string& path, unsigned int lastModif )
    {
        auto parent = dirs[parentPath];
        parent->addFolder( path );
        dirs[parentPath + path] = std::unique_ptr<mock::Directory>( new Directory( parent, parentPath + path, lastModif ) );
    }

    virtual std::unique_ptr<fs::IDirectory> createDirectory(const std::string& path) override
    {
        mock::Directory* res = nullptr;
        if ( path == "." )
        {
            res = dirs[Root].get();
        }
        else
        {
            auto it = dirs.find( path );
            if ( it != end( dirs ) )
                res = it->second.get();
        }
        if ( res == nullptr )
            throw std::runtime_error("Invalid path");
        return std::unique_ptr<fs::IDirectory>( new Directory( *res ) );
    }

    virtual std::unique_ptr<fs::IFile> createFile( const std::string &filePath ) override
    {
        const auto it = files.find( filePath );
        if ( it == end( files ) )
            files[filePath].reset( new File( filePath ) );
        return std::unique_ptr<fs::IFile>( new File( static_cast<const mock::File&>( * it->second.get() ) ) );
    }

    std::unordered_map<std::string, std::shared_ptr<mock::File>> files;
    std::unordered_map<std::string, std::shared_ptr<mock::Directory>> dirs;
};

}

class Folders : public testing::Test
{
    public:
        static std::unique_ptr<IMediaLibrary> ml;
        std::shared_ptr<mock::FileSystemFactory> fsMock;

    protected:
        virtual void Reload()
        {
            ml.reset( MediaLibraryFactory::create() );
            bool res = ml->initialize( "test.db", fsMock );
            ASSERT_TRUE( res );
        }

        virtual void SetUp()
        {
            fsMock.reset( new mock::FileSystemFactory );
            Reload();
        }

        virtual void TearDown()
        {
            ml.reset();
            unlink("test.db");
        }
};

std::unique_ptr<IMediaLibrary> Folders::ml;

TEST_F( Folders, Add )
{
    auto f = ml->addFolder( "." );
    ASSERT_NE( f, nullptr );

    auto files = ml->files();

    ASSERT_EQ( files.size(), 3u );
    ASSERT_FALSE( files[0]->isStandAlone() );
}

TEST_F( Folders, Delete )
{
    auto f = ml->addFolder( "." );
    ASSERT_NE( f, nullptr );

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

    SetUp();

    // Recheck folder deletion from DB:
    f = ml->folder( folderPath );
    ASSERT_EQ( nullptr, f );
}

TEST_F( Folders, Load )
{
    auto f = ml->addFolder( "." );
    ASSERT_NE( f, nullptr );

    SetUp();

    auto files = ml->files();
    ASSERT_EQ( files.size(), 3u );
    for ( auto& f : files )
        ASSERT_FALSE( f->isStandAlone() );
}

TEST_F( Folders, InvalidPath )
{
    auto f = ml->addFolder( "/invalid/path" );
    ASSERT_EQ( f, nullptr );

    auto files = ml->files();
    ASSERT_EQ( files.size(), 0u );
}

TEST_F( Folders, List )
{
    auto f = ml->addFolder( "." );
    ASSERT_NE( f, nullptr );

    auto files = f->files();
    ASSERT_EQ( files.size(), 2u );

    SetUp();

    f = ml->folder( f->path() );
    files = f->files();
    ASSERT_EQ( files.size(), 2u );
}

TEST_F( Folders, AbsolutePath )
{
    auto f = ml->addFolder( "." );
    ASSERT_NE( f->path(), "." );
}

TEST_F( Folders, ListFolders )
{
    auto f = ml->addFolder( "." );
    auto subFolders = f->folders();
    ASSERT_EQ( 1u, subFolders.size() );

    auto subFolder = subFolders[0];
    auto subFiles = subFolder->files();
    ASSERT_EQ( 1u, subFiles.size() );

    auto file = subFiles[0];
    ASSERT_EQ( std::string{ mock::FileSystemFactory::SubFolder } + "subfile.mp4", file->mrl() );

    // Now again, without cache
    SetUp();

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
    auto f = ml->addFolder( "." );
    ASSERT_NE( 0u, f->lastModificationDate() );
    auto subFolders = f->folders();
    ASSERT_NE( 0u, subFolders[0]->lastModificationDate() );

    SetUp();

    f = ml->folder( f->path() );
    ASSERT_NE( 0u, f->lastModificationDate() );
    subFolders = f->folders();
    ASSERT_NE( 0u, subFolders[0]->lastModificationDate() );
}

TEST_F( Folders, NewFile )
{
    ml->addFolder( "." );

    ASSERT_EQ( 3u, ml->files().size() );
    // Do not watch for live changes
    ml.reset();
    auto newFolder = std::string(mock::FileSystemFactory::Root) + "newfolder/";
    fsMock->addFolder( mock::FileSystemFactory::Root, "newfolder/", time( nullptr ) );
    fsMock->addFile( newFolder, "newfile.avi" );

    Reload();

    ASSERT_EQ( 4u, ml->files().size() );
    auto file = ml->file( newFolder + "newfile.avi" );
    ASSERT_NE( nullptr, file );
}
