#ifndef FS_DISCOVERER_H
# define FS_DISCOVERER_H

#include <memory>

#include "IDiscoverer.h"
#include "factory/IFileSystem.h"

class FsDiscoverer : public IDiscoverer
{
public:
    FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory, IMediaLibrary *ml, DBConnection dbConn );
    virtual bool discover(const std::string &entryPoint ) override;
    virtual void reload() override;

private:
    bool checkSubfolders( fs::IDirectory *folder, FolderPtr parentFolder );
    void checkFiles( fs::IDirectory *folder, FolderPtr parentFolder );

private:
    IMediaLibrary* m_ml;
    DBConnection m_dbConn;
    std::shared_ptr<factory::IFileSystem> m_fsFactory;
};

#endif // FS_DISCOVERER_H
