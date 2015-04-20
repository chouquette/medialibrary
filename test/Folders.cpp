#include "gtest/gtest.h"

#include "IFile.h"
#include "IFolder.h"
#include "IMediaLibrary.h"

#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"

namespace mock
{

class Directory : public fs::IDirectory
{
public:
    Directory( const std::string& path, const std::vector<std::string>& files, const std::vector<std::string>& dirs, unsigned int lastModif )
        : m_path( path )
        , m_lastModificationDate( lastModif )
    {
        for ( auto &f : files )
        {
            m_files.push_back( path + f );
        }
        for ( auto& d : dirs )
        {
            m_dirs.push_back( path + d );
        }
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

private:
    std::string m_path;
    std::vector<std::string> m_files;
    std::vector<std::string> m_dirs;
    unsigned int m_lastModificationDate;
};


struct FileSystemFactory : public factory::IFileSystem
{
    static constexpr const char* Root = "/a/";
    static constexpr const char* SubFolder = "/a/folder/";
    virtual std::unique_ptr<fs::IDirectory> createDirectory(const std::string& path) override
    {
        if ( path == Root || path == "." )
        {
            return std::unique_ptr<fs::IDirectory>( new Directory
            {
                Root,
                std::vector<std::string>
                {
                    "video.avi",
                    "audio.mp3",
                    "not_a_media.something",
                    "some_other_file.seaotter",
                },
                std::vector<std::string>
                {
                    "folder/"
                },
                123
            });
        }
        else if ( path == SubFolder )
        {
            return std::unique_ptr<fs::IDirectory>( new Directory
            {
                SubFolder,
                std::vector<std::string>
                {
                    "subfile.mp4"
                },
                std::vector<std::string>{},
                456
            });
        }
        throw std::runtime_error("Invalid path");
    }
};

}

class Folders : public testing::Test
{
    public:
        static std::unique_ptr<IMediaLibrary> ml;

    protected:
        virtual void SetUp()
        {
            ml.reset( MediaLibraryFactory::create() );
            bool res = ml->initialize( "test.db",
                std::unique_ptr<factory::IFileSystem>( new mock::FileSystemFactory ) );
            ASSERT_TRUE( res );
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
    ASSERT_EQ( 123u, f->lastModificationDate() );
    auto subFolders = f->folders();
    ASSERT_EQ( 456u, subFolders[0]->lastModificationDate() );

    SetUp();

    f = ml->folder( f->path() );
    ASSERT_EQ( 123u, f->lastModificationDate() );
    subFolders = f->folders();
    ASSERT_EQ( 456u, subFolders[0]->lastModificationDate() );
}
