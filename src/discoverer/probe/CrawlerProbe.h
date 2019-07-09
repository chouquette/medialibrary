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

#include "discoverer/probe/IProbe.h"

namespace medialibrary
{

namespace prober
{

class CrawlerProbe : public IProbe
{
public:

    CrawlerProbe() = default;

    void setDiscoverNoMedia( bool discoverNoMedia )
    {
        m_discoverNoMedia = discoverNoMedia;
    }

    virtual bool proceedOnDirectory( const fs::IDirectory& ) override
    {
        return true;
    }

    virtual bool isHidden( const fs::IDirectory& directory ) override
    {
        bool hidden = m_discoverNoMedia == false && hasDotNoMediaFile( directory ) == true;
        if ( hidden == true )
            LOG_INFO( "Ignoring folder ", directory.mrl(), " with a .nomedia file" );
        return hidden;
    }

    virtual bool proceedOnFile( const fs::IFile& ) override
    {
        return true;
    }

    virtual bool stopFileDiscovery() override
    {
        return false;
    }

    virtual bool deleteUnseenFolders() override
    {
        return true;
    }

    virtual bool deleteUnseenFiles() override
    {
        return true;
    }

    virtual bool forceFileRefresh() override
    {
        return false;
    }

    virtual std::shared_ptr<Folder> getFolderParent() override
    {
        return nullptr;
    }

    virtual std::pair<int64_t,int64_t> getPlaylistParent() override
    {
        return { 0, 0 };
    }

private:
    static bool hasDotNoMediaFile( const fs::IDirectory& directory )
    {
        const auto& files = directory.files();
        return std::find_if( begin( files ), end( files ), []( const std::shared_ptr<fs::IFile>& file ){
            return strcasecmp( file->name().c_str(), ".nomedia" ) == 0;
        }) != end( files );
    }

private:
    bool m_discoverNoMedia; // Initialized to false

};

}

}
