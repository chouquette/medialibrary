#ifndef FS_DISCOVERER_H
# define FS_DISCOVERER_H

#include <memory>

#include "IDiscoverer.h"
#include "factory/IFileSystem.h"

class FsDiscoverer : public IDiscoverer
{
public:
    FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory );
    virtual bool discover(IMediaLibrary *ml, DBConnection dbConn, const std::string &entryPoint ) override;
    virtual void reload( IMediaLibrary* ml, DBConnection dbConn ) override;

private:
    bool checkSubfolders(IMediaLibrary *ml, DBConnection dbConn, fs::IDirectory *folder, FolderPtr parentFolder );
    void checkFiles(IMediaLibrary *ml, DBConnection dbConn, fs::IDirectory *folder, FolderPtr parentFolder );

private:
    std::shared_ptr<factory::IFileSystem> m_fsFactory;
};

#endif // FS_DISCOVERER_H
