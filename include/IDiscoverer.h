#ifndef IDISCOVERER_H
# define IDISCOVERER_H

#include <string>
#include "Types.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "IMediaLibrary.h"

class IDiscoverer
{
public:
    virtual ~IDiscoverer() = default;
    // We assume the media library will always outlive the discoverers.
    //FIXME: This is currently false since there is no way of interrupting
    //a discoverer thread
    virtual bool discover( const std::string& entryPoint ) = 0;
    virtual void reload() = 0;
};

#endif // IDISCOVERER_H
