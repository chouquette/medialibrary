#ifndef FS_DISCOVERER_H
# define FS_DISCOVERER_H

#include <memory>

#include "IDiscoverer.h"
#include "factory/IFileSystem.h"

class FsDiscoverer : public IDiscoverer
{
public:
    FsDiscoverer(std::shared_ptr<factory::IFileSystem> fsFactory , IDiscovererCb *discoveryCb);
    virtual bool discover( const std::string &entryPoint ) override;

private:
    std::shared_ptr<factory::IFileSystem> m_fsFactory;
    IDiscovererCb* m_discoveryCb;
};

#endif // FS_DISCOVERER_H
