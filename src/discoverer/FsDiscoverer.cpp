#include "FsDiscoverer.h"

#include "factory/FileSystem.h"
#include <queue>
#include <iostream>

FsDiscoverer::FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory, IDiscovererCb* discoveryCb )
    : m_discoveryCb( discoveryCb )
{
    if ( fsFactory != nullptr )
        m_fsFactory = fsFactory;
    else
        m_fsFactory.reset( new factory::FileSystemDefaultFactory );
}

bool FsDiscoverer::discover( const std::string &entryPoint )
{
    std::queue<std::pair<std::string, FolderPtr>> folders;

    folders.emplace( entryPoint, nullptr );
    while ( folders.empty() == false )
    {
        std::unique_ptr<fs::IDirectory> dir;
        auto currentFolder = folders.front();
        auto currentPath = currentFolder.first;
        auto parent = currentFolder.second;
        folders.pop();

        try
        {
            dir = m_fsFactory->createDirectory( currentPath );
        }
        catch ( std::runtime_error& ex )
        {
            std::cerr << ex.what() << std::endl;
            // If the first directory fails to open, stop now.
            // Otherwise, assume something went wrong in a subdirectory.
            if (parent == nullptr)
                return false;
            continue;
        }

        auto folder = m_discoveryCb->onNewFolder( dir.get(), parent );
        if ( folder == nullptr && parent == nullptr )
            return false;
        if ( folder == nullptr )
            continue;

        for ( auto& f : dir->files() )
        {
            auto fsFile = m_fsFactory->createFile( f );
            m_discoveryCb->onNewFile( fsFile.get(), folder );
        }
        for ( auto& f : dir->dirs() )
            folders.emplace( f, folder );
    }
    return true;
}
