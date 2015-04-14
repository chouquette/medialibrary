#include "factory/FileSystem.h"
#include "filesystem/IDirectory.h"

#if defined(__linux__) || defined(__APPLE__)
# include "filesystem/unix/Directory.h"
#else
# error No filesystem implementation for this architecture
#endif

namespace factory
{

std::unique_ptr<fs::IDirectory> FileSystemDefaultFactory::createDirectory( const std::string& path )
{
    return std::unique_ptr<fs::IDirectory>( new fs::Directory( path ) );
}

}
