#pragma once

#include <memory>
#include <vector>

namespace fs
{
    class IFile;

    class IDirectory
    {
    public:
        virtual ~IDirectory() = default;
        virtual std::vector<std::unique_ptr<IFile>> files() const = 0;
    };

    std::unique_ptr<IDirectory> createDirectory( const std::string& path );
}
