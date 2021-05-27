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
    virtual void onMediaModified( std::set<int64_t> ) override {}
    virtual void onMediaDeleted( std::set<int64_t> ) override {}
    virtual void onMediaConvertedToExternal( std::set<int64_t> ) override {}
    virtual void onDiscoveryStarted() override {}
    virtual void onDiscoveryProgress(const std::string&) override {}
    virtual void onDiscoveryCompleted() override {}
    virtual void onDiscoveryFailed( const std::string& ) override {}
    virtual void onArtistsAdded( std::vector<ArtistPtr> ) override {}
    virtual void onArtistsModified( std::set<int64_t> ) override {}
    virtual void onArtistsDeleted( std::set<int64_t> ) override {}
    virtual void onAlbumsAdded( std::vector<AlbumPtr> ) override {}
    virtual void onAlbumsModified( std::set<int64_t> ) override {}
    virtual void onAlbumsDeleted( std::set<int64_t> ) override {}
    virtual void onParsingStatsUpdated( uint32_t ) override {}
    virtual void onPlaylistsAdded( std::vector<PlaylistPtr> ) override {}
    virtual void onPlaylistsModified( std::set<int64_t> ) override {}
    virtual void onPlaylistsDeleted( std::set<int64_t> ) override {}
    virtual void onGenresAdded( std::vector<GenrePtr> ) override {}
    virtual void onGenresModified( std::set<int64_t> ) override {}
    virtual void onGenresDeleted( std::set<int64_t> ) override {}
    virtual void onMediaGroupsAdded( std::vector<MediaGroupPtr> ) override {}
    virtual void onMediaGroupsModified( std::set<int64_t> ) override {}
    virtual void onMediaGroupsDeleted( std::set<int64_t> ) override {}
    virtual void onBookmarksAdded( std::vector<BookmarkPtr> ) override {}
    virtual void onBookmarksModified( std::set<int64_t> ) override {}
    virtual void onBookmarksDeleted( std::set<int64_t> ) override {}
    virtual void onEntryPointAdded( const std::string&, bool ) override {}
    virtual void onEntryPointRemoved( const std::string&, bool ) override {}
    virtual void onEntryPointBanned( const std::string&, bool ) override {}
    virtual void onEntryPointUnbanned( const std::string&, bool ) override {}
    virtual void onBackgroundTasksIdleChanged( bool ) override {}
    virtual void onMediaThumbnailReady( MediaPtr, ThumbnailSizeType, bool ) override {}
    virtual void onHistoryChanged( HistoryType ) override {}
    virtual void onRescanStarted() override {}
};

}
