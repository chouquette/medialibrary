#pragma once

#include <memory>

namespace fs
{
    class IDirectory;
    class IFile;
}

namespace factory
{
    class IFileSystem
    {
    public:
        virtual ~IFileSystem() = default;
        virtual std::unique_ptr<fs::IDirectory> createDirectory( const std::string& path ) = 0;
        virtual std::unique_ptr<fs::IFile> createFile( const std::string& path, const std::string& fileName ) = 0;
    };
}
