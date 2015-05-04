#include "Tests.h"
#include "filesystem/IFile.h"
#include "filesystem/IDirectory.h"
#include "Utils.h"

class TestEnv : public ::testing::Environment
{
    public:
        virtual void SetUp()
        {
            // Always clean the DB in case a previous test crashed
            unlink("test.db");
        }
};

namespace mock
{
namespace defaults
{

class File : public fs::IFile
{
    std::string m_path;
    std::string m_fileName;
    std::string m_extension;

public:
    File( const std::string& file )
        : m_path( file )
        , m_fileName( utils::file::fileName( file ) )
        , m_extension( utils::file::extension( file ) )
    {
    }

    virtual const std::string& name() const
    {
        return m_fileName;
    }

    virtual const std::string& path() const
    {
        return m_path;
    }

    virtual const std::string& fullPath() const
    {
        return m_path;
    }

    virtual const std::string& extension() const
    {
        return m_extension;
    }

    virtual unsigned int lastModificationDate() const
    {
        // Ensure a non-0 value so tests can easily verify that the value
        // is initialized
        return 123;
    }
};

class FileSystemFactory : public factory::IFileSystem
{
    virtual std::unique_ptr<fs::IDirectory> createDirectory(const std::string&)
    {
        return nullptr;
    }

    virtual std::unique_ptr<fs::IFile> createFile(const std::string& fileName)
    {
        return std::unique_ptr<fs::IFile>( new File( fileName ) );
    }
};
}
}

void Tests::TearDown()
{
    ml.reset();
    unlink("test.db");
}


void Tests::Reload(std::shared_ptr<factory::IFileSystem> fs /* = nullptr */ )
{
    ml.reset( MediaLibraryFactory::create() );
    bool res = ml->initialize( "test.db", fs ? fs : defaultFs );
    ASSERT_TRUE( res );
}


void Tests::SetUp()
{
    defaultFs = std::make_shared<mock::defaults::FileSystemFactory>();
    Reload();
}

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);
