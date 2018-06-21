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

#include <vector>
#include <memory>

#include "IMediaLibrary.h"
#include "IFile.h"

namespace medialibrary
{

class IAlbumTrack;
class IShowEpisode;
class ITrackInformation;
class IMetadata;

class IMedia
{
    public:
        enum class Type : uint8_t
        {
            Unknown,
            Video,
            Audio,
            External,
        };
        enum class SubType : uint8_t
        {
            Unknown,
            ShowEpisode,
            Movie,
            AlbumTrack,
        };
        enum class MetadataType : uint32_t
        {
            Rating = 1,

            // Playback
            Progress = 50,
            Speed,
            Title,
            Chapter,
            Program,
            Seen,

            // Video:
            VideoTrack = 100,
            AspectRatio,
            Zoom,
            Crop,
            Deinterlace,
            VideoFilter,

            // Audio
            AudioTrack = 150,
            Gain,
            AudioDelay,

            // Spu
            SubtitleTrack = 200,
            SubtitleDelay,

            // Various
            ApplicationSpecific = 250,
        };
        static constexpr size_t NbMeta = 19;

        virtual ~IMedia() = default;

        virtual int64_t id() const = 0;
        virtual Type type() const = 0;
        virtual SubType subType() const = 0;
        virtual const std::string& title() const = 0;
        virtual bool setTitle( const std::string& title ) = 0;
        virtual AlbumTrackPtr albumTrack() const = 0;
        /**
         * @brief duration Returns the media duration in ms
         */
        virtual int64_t duration() const = 0;
        virtual int playCount() const = 0;
        virtual bool increasePlayCount() = 0;
        virtual ShowEpisodePtr showEpisode() const = 0;
        virtual const std::vector<FilePtr>& files() const = 0;
        /**
         * @return The main file's filename
         */
        virtual const std::string& fileName() const = 0;
        virtual FilePtr addExternalMrl( const std::string& mrl, IFile::Type type ) = 0;
        virtual bool isFavorite() const = 0;
        virtual bool setFavorite( bool favorite ) = 0;
        virtual bool addLabel( LabelPtr label ) = 0;
        virtual bool removeLabel( LabelPtr label ) = 0;
        virtual MoviePtr movie() const = 0;
        virtual Query<ILabel> labels() const = 0;
        virtual Query<IVideoTrack> videoTracks() = 0;
        virtual Query<IAudioTrack> audioTracks() = 0;
        ///
        /// \brief thumbnail Returns the path of a thumbnail for this media
        /// \return A path, relative to the thumbnailPath configured when initializing
        ///  The media library
        ///
        virtual const std::string& thumbnail() const = 0;
        ///
        /// \brief isThumbnailGenerated Returns true if a thumbnail generation was
        ///                             attempted.
        /// In case the thumbnail generation failed, this will still be true, but
        /// the thumbnail returned by \sa{thumbnail} will be empty.
        /// This is intended as a helper for the client application, so it doesn't
        /// attempt ask for a new thumbmail generation.
        /// \return
        ///
        virtual bool isThumbnailGenerated() const = 0;
        ///
        /// \brief setThumbnail Sets a thumbnail for the current media
        /// \param mrl A mrl pointing the the thumbnail file.
        /// \return true in case the thumbnail was successfully stored to database
        ///         false otherwise
        /// This is intended to be used by applications that have their own way
        /// of computing thumbnails.
        ///
        virtual bool setThumbnail( const std::string& mrl ) = 0;
        virtual unsigned int insertionDate() const = 0;
        virtual unsigned int releaseDate() const = 0;

        /// Metadata
        ///
        /// \brief metadata Fetch (or return a cached) metadata value for this media
        /// \param type The metadata type
        /// \return A reference to a wrapper object representing the metadata.
        ///
        virtual const IMetadata& metadata( MetadataType type ) const = 0;
        ///
        /// \brief setMetadata Immediatly saves a metadata in database
        ///
        virtual bool setMetadata( MetadataType type, const std::string& value ) = 0;
        virtual bool setMetadata( MetadataType type, int64_t value ) = 0;
};

}
