#include "IDirectory.h"

#if defined(__linux__) || defined(__APPLE__)
# include "unix/Directory.h"
#else
# error No filesystem implementation for this architecture
#endif

std::unique_ptr<fs::IDirectory> fs::createDirectory( const std::string& path )
{
    return std::unique_ptr<fs::IDirectory>( new fs::Directory( path ) );
}
