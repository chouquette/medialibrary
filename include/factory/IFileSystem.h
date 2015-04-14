#pragma once

#include <memory>

namespace fs
{
    class IDirectory;
}

namespace factory
{
    class IFileSystem
    {
    public:
        virtual ~IFileSystem() = default;
        virtual std::unique_ptr<fs::IDirectory> createDirectory( const std::string& path ) = 0;
    };
}
