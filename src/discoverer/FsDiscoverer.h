/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef FS_DISCOVERER_H
# define FS_DISCOVERER_H

#include <memory>

#include "discoverer/IDiscoverer.h"
#include "factory/IFileSystem.h"

namespace medialibrary
{

class MediaLibrary;
class Folder;

class FsDiscoverer : public IDiscoverer
{
public:
    FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory, MediaLibrary* ml , IMediaLibraryCb* cb );
    virtual bool discover(const std::string &entryPoint ) override;
    virtual bool reload() override;
    virtual bool reload( const std::string& entryPoint ) override;

private:
    ///
    /// \brief checkSubfolders
    /// \return true if files in this folder needs to be listed, false otherwise
    ///
    void checkFolder( fs::IDirectory& currentFolderFs, Folder& currentFolder, bool newFolder ) const;
    void checkFiles(fs::IDirectory& parentFolderFs, Folder& parentFolder ) const;
    static bool hasDotNoMediaFile( const fs::IDirectory& directory );
    bool addFolder( fs::IDirectory& folder, Folder* parentFolder ) const;
    bool checkDevices();

private:
    MediaLibrary* m_ml;
    std::shared_ptr<factory::IFileSystem> m_fsFactory;
    IMediaLibraryCb* m_cb;
};

}

#endif // FS_DISCOVERER_H
