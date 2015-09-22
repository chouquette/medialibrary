#ifndef IDISCOVERER_H
# define IDISCOVERER_H

#include <string>
#include "Types.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"

class IDiscovererCb
{
public:
    virtual ~IDiscovererCb() = default;
    /**
     * @brief onNewFolder Called when the discoverer finds a new directory
     * @param folderPath The new directory's path
     * @param parent The parent folder, or null if this is the root folder
     * @return The newly created folder, or nullptr in case of error (or if the
     *         directory shall not be browsed further)
     */
    virtual FolderPtr onNewFolder( const fs::IDirectory* folder, FolderPtr parent ) = 0;
    /**
     * @brief onNewFile Called when the discoverer finds a new file
     * @param filePath The new file's path
     * @param parent The parent folder
     * @return true if the file was accepted
     */
    virtual FilePtr onNewFile( const fs::IFile* file, FolderPtr parent ) = 0;
};

class IDiscoverer
{
public:
    virtual ~IDiscoverer() = default;
    virtual bool discover( const std::string& entryPoint ) = 0;
};

#endif // IDISCOVERER_H
