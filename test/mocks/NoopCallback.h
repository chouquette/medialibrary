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

#include "IMediaLibrary.h"

namespace mock
{

class NoopCallback : public IMediaLibraryCb
{
    virtual void onMediaAdded( std::vector<MediaPtr> ) override {}
    virtual void onMediaUpdated( std::vector<MediaPtr> ) override {}
    virtual void onMediaDeleted( std::vector<int64_t> ) override {}
    virtual void onDiscoveryStarted(const std::string&) override {}
    virtual void onDiscoveryCompleted(const std::string&) override {}
    virtual void onReloadStarted( const std::string& ) override {}
    virtual void onReloadCompleted( const std::string& ) override {}
    virtual void onArtistsAdded( std::vector<ArtistPtr> ) override {}
    virtual void onArtistsModified( std::vector<ArtistPtr> ) override {}
    virtual void onArtistsDeleted( std::vector<int64_t> ) override {}
    virtual void onAlbumAdded( AlbumPtr ) override {}
    virtual void onTrackAdded( MediaPtr, AlbumTrackPtr ) override {}
    virtual void onParsingStatsUpdated( uint32_t ) override {}
};

}
