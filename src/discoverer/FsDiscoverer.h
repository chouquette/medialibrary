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
#include <atomic>

#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "compat/ConditionVariable.h"
#include "compat/Mutex.h"

namespace medialibrary
{

class MediaLibrary;
class IMediaLibraryCb;
class Folder;

class FsDiscoverer
{
public:
    FsDiscoverer(MediaLibrary* ml , IMediaLibraryCb* cb);
    bool reload();
    bool reload( const std::string& entryPoint );
    bool addEntryPoint(const std::string& entryPoint );
    /**
     * @brief interrupt Interrupts the current operation and return ASAP
     */
    void interrupt();
    void pause();
    void resume();

private:
    void checkFolder( std::shared_ptr<fs::IDirectory> currentFolderFs,
                      std::shared_ptr<Folder> currentFolder,
                      fs::IFileSystemFactory& fsFactory) const;
    void checkFiles( std::shared_ptr<fs::IDirectory> parentFolderFs,
                     std::shared_ptr<Folder> parentFolder ) const;
    std::shared_ptr<Folder> addFolder(std::shared_ptr<fs::IDirectory> folder,
                                      Folder* parentFolder) const;
    bool reloadFolder( std::shared_ptr<Folder> folder,
                       fs::IFileSystemFactory& fsFactory );
    void checkRemovedDevices(fs::IDirectory& fsFolder, std::shared_ptr<Folder> folder,
                              fs::IFileSystemFactory& fsFactory, bool newFolder,
                              bool rootFolder ) const;
    bool isInterrupted() const;
    void waitIfPaused() const;

private:
    MediaLibrary* m_ml;
    IMediaLibraryCb* m_cb;
    std::atomic_bool m_isInterrupted;
    mutable compat::Mutex m_mutex;
    mutable compat::ConditionVariable m_cond;
    bool m_paused;
};

}
