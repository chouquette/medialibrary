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

#include <string>
#include <stack>
#include <memory>

#include "discoverer/probe/IProbe.h"

namespace medialibrary
{

namespace prober
{

class PathProbe : public IProbe
{
public:
    explicit PathProbe( std::string path,                         // The path we target
                        bool isDirectory,                         // The PathProbe behave differently between files & folders
                        std::shared_ptr<Folder> parentFolder,     // known parent folder to start from, if any
                        const std::string& parentFolderPath,      // known parent folder path
                        int64_t parentPlaylistId,
                        int64_t parentPlaylistIndex,
                        bool reload );

    virtual bool proceedOnDirectory( const fs::IDirectory& directory ) override;

    virtual bool isHidden( const fs::IDirectory& ) override
    {
        // We want to add the provided path without checking for .nomedia files
        return false;
    }

    virtual bool proceedOnFile( const fs::IFile& file ) override;

    virtual bool stopFileDiscovery() override
    {
        return m_isDiscoveryEnded;
    }

    virtual bool deleteUnseenFolders() override
    {
        return false;
    }

    virtual bool deleteUnseenFiles() override
    {
        return false;
    }

    virtual bool forceFileRefresh() override
    {
        return true;
    }

    virtual std::shared_ptr<Folder> getFolderParent() override
    {
        return m_parentFolder;
    }

    virtual std::pair<int64_t,int64_t> getPlaylistParent() override
    {
        return { m_parentPlaylistId, m_playlistIndex };
    }

private:
    bool proceedOnEntryPoint( const fs::IDirectory& directory );

    bool m_isDirectory;
    std::stack<std::string> m_splitPath;
    bool m_isDiscoveryEnded;
    bool m_entryPointHandled;
    std::shared_ptr<Folder> m_parentFolder;
    std::string m_path;
    int64_t m_parentPlaylistId;
    int64_t m_playlistIndex;
};

}

}
