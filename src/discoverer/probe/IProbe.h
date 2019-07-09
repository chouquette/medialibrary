/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2017-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *          Alexandre Fernandez <nerf@boboop.fr>
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

namespace medialibrary
{

class Playlist;
class Folder;

namespace fs
{
class IDirectory;
class IFile;
}

namespace prober
{

class IProbe
{
public:
    virtual ~IProbe() = default;

    /**
     * @brief proceedOnDirectory Decide whether or not the FsDiscoverer should scan a directory
     */
    virtual bool proceedOnDirectory( const fs::IDirectory& directory ) = 0;

    /**
     * @brief isHidden Inform the FsDiscoverer if directory is considered hidden or not
     */
    virtual bool isHidden( const fs::IDirectory& directory ) = 0;

    /**
     * @brief proceedOnFile Decide if the FsDiscoverer should check a file or ignore it
     */
    virtual bool proceedOnFile( const fs::IFile& file ) = 0;

    /**
     * @brief stopFileDiscovery Tell the FsDiscoverer if whe should stop the scan for optimisation purpose
     */
    virtual bool stopFileDiscovery() = 0;

    /**
     * @brief proceedOnDirectory Decide if the FsDiscoverer should delete folders not found on the file system
     */
    virtual bool deleteUnseenFolders() = 0;

    /**
     * @brief deleteUnseenFiles Decide if the FsDiscoverer should delete files not found on the file system
     */
    virtual bool deleteUnseenFiles() = 0;

    /**
     * @brief forceFileRefresh Decide if we force-add discovered files to filesToAdd, like if it was new ones
     */
    virtual bool forceFileRefresh() = 0;

    virtual std::shared_ptr<Folder> getFolderParent() = 0;

    virtual std::pair<int64_t, int64_t> getPlaylistParent() = 0;
};

}

}
