#include "Tests.h"

#include <memory>

#include "filesystem/IFile.h"
#include "filesystem/IDirectory.h"
#include "Utils.h"
#include "discoverer/FsDiscoverer.h"
#include "mocks/FileSystem.h"

class TestEnv : public ::testing::Environment
{
    public:
        virtual void SetUp()
        {
            // Always clean the DB in case a previous test crashed
            unlink("test.db");
        }
};

void Tests::TearDown()
{
    ml.reset();
    unlink("test.db");
}


void Tests::Reload(std::shared_ptr<factory::IFileSystem> fs /*= nullptr*/, IMediaLibraryCb* metadataCb /*= nullptr*/ )
{
    ml.reset( MediaLibraryFactory::create() );
    if ( fs == nullptr )
    {
        fs = std::shared_ptr<factory::IFileSystem>( new mock::NoopFsFactory );
    }
    ml->setFsFactory( fs );
    bool res = ml->initialize( "test.db", "/tmp", metadataCb );
    ASSERT_TRUE( res );
}


void Tests::SetUp()
{
    defaultFs = std::make_shared<mock::NoopFsFactory>();
    Reload();
}

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);
