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
        /// Returns a list of absolute files path
        virtual const std::vector<std::string>& files() const = 0;
        /// Returns a list of absolute path to this folder subdirectories
        virtual const std::vector<std::string>& dirs() const = 0;
        virtual unsigned int lastModificationDate() const = 0;
        virtual bool isRemovable() const = 0;
    };
}
