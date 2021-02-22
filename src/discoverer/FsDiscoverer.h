/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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
#include "medialibrary/filesystem/IFileSystemFactory.h"

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
    FsDiscoverer( MediaLibrary* ml , IMediaLibraryCb* cb, std::unique_ptr<prober::IProbe> probe );
    virtual bool discover( const std::string& entryPoint,
                           const IInterruptProbe& interruptProbe ) override;
    virtual bool reload( const IInterruptProbe& interruptProbe ) override;
    virtual bool reload( const std::string& entryPoint,
                         const IInterruptProbe& interruptProbe ) override;

private:
    void checkFolder( std::shared_ptr<fs::IDirectory> currentFolderFs,
                      std::shared_ptr<Folder> currentFolder,
                      const IInterruptProbe& interruptProbe,
                      fs::IFileSystemFactory& fsFactory, bool newFolder,
                      bool rootFolder ) const;
    void checkFiles( std::shared_ptr<fs::IDirectory> parentFolderFs,
                     std::shared_ptr<Folder> parentFolder,
                     const IInterruptProbe& interruptProbe ) const;
    std::shared_ptr<Folder> addFolder( std::shared_ptr<fs::IDirectory> folder,
                                      Folder* parentFolder,
                                      fs::IFileSystemFactory& fsFactory ) const;
    bool reloadFolder( std::shared_ptr<Folder> folder,
                       const IInterruptProbe& probe, fs::IFileSystemFactory& fsFactory );
    void checkRemovedDevices(fs::IDirectory& fsFolder, std::shared_ptr<Folder> folder,
                              fs::IFileSystemFactory& fsFactory, bool newFolder,
                              bool rootFolder ) const;

private:
    MediaLibrary* m_ml;
    IMediaLibraryCb* m_cb;
    std::unique_ptr<prober::IProbe> m_probe;
};

}
