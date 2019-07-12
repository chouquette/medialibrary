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

#include "medialibrary/IMediaLibrary.h"

using namespace medialibrary;

namespace mock
{

class NoopCallback : public IMediaLibraryCb
{
    virtual void onMediaAdded( std::vector<MediaPtr> ) override {}
    virtual void onMediaModified( std::vector<MediaPtr> ) override {}
    virtual void onMediaDeleted( std::vector<int64_t> ) override {}
    virtual void onDiscoveryStarted(const std::string&) override {}
    virtual void onDiscoveryProgress(const std::string&) override {}
    virtual void onDiscoveryCompleted( const std::string&, bool ) override {}
    virtual void onArtistsAdded( std::vector<ArtistPtr> ) override {}
    virtual void onArtistsModified( std::vector<ArtistPtr> ) override {}
    virtual void onArtistsDeleted( std::vector<int64_t> ) override {}
    virtual void onAlbumsAdded( std::vector<AlbumPtr> ) override {}
    virtual void onAlbumsModified( std::vector<AlbumPtr> ) override {}
    virtual void onAlbumsDeleted( std::vector<int64_t> ) override {}
    virtual void onParsingStatsUpdated( uint32_t ) override {}
    virtual void onPlaylistsAdded( std::vector<PlaylistPtr> ) override {}
    virtual void onPlaylistsModified( std::vector<PlaylistPtr> ) override {}
    virtual void onPlaylistsDeleted( std::vector<int64_t> ) override {}
    virtual void onGenresAdded( std::vector<GenrePtr> ) override {}
    virtual void onGenresModified( std::vector<GenrePtr> ) override {}
    virtual void onGenresDeleted( std::vector<int64_t> ) override {}
    virtual void onReloadStarted( const std::string& ) override {}
    virtual void onReloadCompleted( const std::string&, bool ) override {}
    virtual void onEntryPointAdded( const std::string&, bool ) override {}
    virtual void onEntryPointRemoved( const std::string&, bool ) override {}
    virtual void onEntryPointBanned( const std::string&, bool ) override {}
    virtual void onEntryPointUnbanned( const std::string&, bool ) override {}
    virtual void onBackgroundTasksIdleChanged( bool ) override {}
    virtual void onMediaThumbnailReady( MediaPtr, ThumbnailSizeType, bool ) override {}
};

}
