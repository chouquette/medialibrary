#include "factory/FileSystem.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"

#if defined(__linux__) || defined(__APPLE__)
# include "filesystem/unix/Directory.h"
# include "filesystem/unix/File.h"
#else
# error No filesystem implementation for this architecture
#endif

namespace factory
{

std::unique_ptr<fs::IDirectory> FileSystemDefaultFactory::createDirectory( const std::string& path )
{
    return std::unique_ptr<fs::IDirectory>( new fs::Directory( path ) );
}

std::unique_ptr<fs::IFile> FileSystemDefaultFactory::createFile(const std::string& fileName)
{
    return std::unique_ptr<fs::IFile>( new fs::File( fileName ) );
}

}
