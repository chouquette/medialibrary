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
        ///
        /// \brief createDirectory creates a representation of a directory
        /// \param path An absolute path to a directory
        ///
        virtual std::unique_ptr<fs::IDirectory> createDirectory( const std::string& path ) = 0;
        ///
        /// \brief createFile creates a representation of a file
        /// \param fileName an absolute path to a file
        ///
        virtual std::unique_ptr<fs::IFile> createFile( const std::string& fileName ) = 0;
    };
}
