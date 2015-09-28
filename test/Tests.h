#include "gtest/gtest.h"

#include "factory/IFileSystem.h"
#include "IMediaLibrary.h"

class Tests : public testing::Test
{
protected:
    std::unique_ptr<IMediaLibrary> ml;
    std::shared_ptr<factory::IFileSystem> defaultFs;

    virtual void SetUp() override;
    void Reload( std::shared_ptr<factory::IFileSystem> fs = nullptr, IMediaLibraryCb* metadataCb = nullptr );
    virtual void TearDown() override;
};
