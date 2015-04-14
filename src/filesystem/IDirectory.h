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
        // Returns the absolute path to this directory
        virtual const std::string& path() const = 0;
        virtual const std::vector<std::string>& files() const = 0;
    };
}
