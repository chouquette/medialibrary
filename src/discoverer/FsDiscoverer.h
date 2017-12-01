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

#pragma once

#include <memory>

#include "discoverer/IDiscoverer.h"
#include "factory/IFileSystem.h"

namespace medialibrary
{

class MediaLibrary;
class Folder;

namespace prober
{
class IProbe;
}

class FsDiscoverer : public IDiscoverer
{
public:
    FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory, MediaLibrary* ml , IMediaLibraryCb* cb, std::unique_ptr<prober::IProbe> probe );
    virtual bool discover(const std::string& entryPoint ) override;
    virtual bool reload() override;
    virtual bool reload( const std::string& entryPoint ) override;

private:
    ///
    /// \brief checkSubfolders
    /// \return true if files in this folder needs to be listed, false otherwise
    ///
    void checkFolder( std::shared_ptr<fs::IDirectory> currentFolderFs,
                      std::shared_ptr<Folder> currentFolder, bool newFolder ) const;
    void checkFiles( std::shared_ptr<fs::IDirectory> parentFolderFs,
                     std::shared_ptr<Folder> parentFolder ) const;
    bool addFolder( std::shared_ptr<fs::IDirectory> folder,
                    Folder* parentFolder ) const;
    void reloadFolder( std::shared_ptr<Folder> folder );

private:
    MediaLibrary* m_ml;
    std::shared_ptr<factory::IFileSystem> m_fsFactory;
    IMediaLibraryCb* m_cb;
    std::unique_ptr<prober::IProbe> m_probe;
};

}
