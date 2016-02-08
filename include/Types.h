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

class IAlbum;
class IAlbumTrack;
class IAudioTrack;
class IDiscoverer;
class IFile;
class IGenre;
class IHistoryEntry;
class IMedia;
class ILabel;
class IMetadataService;
class IMovie;
class IShow;
class IShowEpisode;
class IVideoTrack;
class ILogger;
class IArtist;
class IPlaylist;
class SqliteConnection;
class MediaLibrary;

using AlbumPtr = std::shared_ptr<IAlbum>;
using AlbumTrackPtr = std::shared_ptr<IAlbumTrack>;
using ArtistPtr = std::shared_ptr<IArtist>;
using AudioTrackPtr = std::shared_ptr<IAudioTrack>;
using FilePtr = std::shared_ptr<IFile>;
using GenrePtr = std::shared_ptr<IGenre>;
using HistoryPtr = std::shared_ptr<IHistoryEntry>;
using LabelPtr = std::shared_ptr<ILabel>;
using MediaPtr = std::shared_ptr<IMedia>;
using MoviePtr = std::shared_ptr<IMovie>;
using PlaylistPtr = std::shared_ptr<IPlaylist>;
using ShowEpisodePtr = std::shared_ptr<IShowEpisode>;
using ShowPtr = std::shared_ptr<IShow>;
using VideoTrackPtr = std::shared_ptr<IVideoTrack>;

using MediaLibraryPtr = const MediaLibrary*;

typedef SqliteConnection* DBConnection;

enum class LogLevel
{
    /// Verbose: Extra logs (currently used by to enable third parties logs
    /// such as VLC)
    Verbose,
    Debug,
    Info,
    Warning,
    Error,
};
