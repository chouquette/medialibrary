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

#ifndef TYPES_H
#define TYPES_H

#include <memory>

class IAlbum;
class IAlbumTrack;
class IAudioTrack;
class IDiscoverer;
class IFile;
class IFolder;
class ILabel;
class IMetadataService;
class IMovie;
class IShow;
class IShowEpisode;
class IVideoTrack;
class ILogger;
class IArtist;
class SqliteConnection;

typedef std::shared_ptr<IFile> FilePtr;
typedef std::shared_ptr<IFolder> FolderPtr;
typedef std::shared_ptr<ILabel> LabelPtr;
typedef std::shared_ptr<IAlbum> AlbumPtr;
typedef std::shared_ptr<IAlbumTrack> AlbumTrackPtr;
typedef std::shared_ptr<IShow> ShowPtr;
typedef std::shared_ptr<IShowEpisode> ShowEpisodePtr;
typedef std::shared_ptr<IMovie> MoviePtr;
typedef std::shared_ptr<IAudioTrack> AudioTrackPtr;
typedef std::shared_ptr<IVideoTrack> VideoTrackPtr;
typedef std::shared_ptr<IArtist> ArtistPtr;

typedef SqliteConnection* DBConnection;

enum ServiceStatus
{
    /// All good
    StatusSuccess,
    /// Something failed, but it's not critical (For instance, no internet connection for a
    /// module that uses an online database)
    StatusError,
    /// We can't compute this file for now (for instance the file was on a network drive which
    /// isn't connected anymore)
    StatusTemporaryUnavailable,
    /// Something failed and we won't continue
    StatusFatal
};

#endif // TYPES_H
