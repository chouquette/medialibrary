#pragma once

#include "factory/IFileSystem.h"

namespace factory
{
    class FileSystemDefaultFactory : public IFileSystem
    {
    public:
        virtual std::unique_ptr<fs::IDirectory> createDirectory( const std::string& path ) override;
        virtual std::unique_ptr<fs::IFile> createFile( const std::string& path, const std::string& fileName ) override;
    };
}
