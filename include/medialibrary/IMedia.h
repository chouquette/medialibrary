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

#include <vector>
#include <memory>

#include "IFile.h"
#include "Types.h"
#include "IQuery.h"

namespace medialibrary
{

class IAlbumTrack;
class IShowEpisode;
class ITrackInformation;
class IMetadata;
struct QueryParameters;
enum class ThumbnailSizeType : uint8_t;

class IMedia
{
    public:
        enum class Type : uint8_t
        {
            /**
             * Unknown media type. Type is used to avoid 0 being a valid value
             * Media that are discovered by the medialibrary will not be
             * added to the collection when their type can't be determined
             */
            Unknown,
            /**
             * Video media
             */
            Video,
            /**
             * Audio media
             */
            Audio,
            /**
             * External media, ie. media that were not discovered by the media
             * library, but that were added manually by the user.
             * These media are not analyzed, so their subtype, tracks, or other
             * details are not known.
             * They can, however, be used to store meta or be included in the
             * playback history.
             */
            External,
            /**
             * Represent a stream, which is a specific kind of External media.
             * This type of media is also intended to be inserted manually by
             * the user.
             */
            Stream,
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
        virtual uint32_t playCount() const = 0;
        /**
         * @brief increasePlayCount Increment this media play count by 1.
         *
         * This also bumps the last played date to "now", causing the media
         * to be the recent when fetching the history.
         */
        virtual bool increasePlayCount() = 0;
        /**
         * @brief setPlayCount Set a specific value to this media's play count
         *
         * This is mostly intended for migrations where single step increment
         * would not be the most efficient way.
         * This method will not bump the media in the history
         */
        virtual bool setPlayCount( uint32_t playCount ) = 0;
        virtual time_t lastPlayedDate() const = 0;
        virtual ShowEpisodePtr showEpisode() const = 0;
        virtual const std::vector<FilePtr>& files() const = 0;
        /**
         * @brief addFile Add a file to this media
         * @param mrl The new file mrl
         * @param fileType The new file type
         * @return
         */
        virtual FilePtr addFile( const std::string& mrl, IFile::Type fileType ) = 0;
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
        virtual Query<IVideoTrack> videoTracks() const = 0;
        virtual Query<IAudioTrack> audioTracks() const = 0;
        virtual Query<ISubtitleTrack> subtitleTracks() const = 0;
        ///
        /// \brief chapters Returns the chapters for this media, if any
        ///
        /// For this query, the default sorting parameter is by chapter offset.
        /// Supported criteria are: Alpha, Duration, Default.
        /// Any other criteria will fallback to default.
        /// Default order for duration is from longer to shorter.
        /// Passing desc = true will invert this default.
        ///
        virtual Query<IChapter> chapters( const QueryParameters* params ) const = 0;
        ///
        /// \brief thumbnail Returns the mrl of a thumbnail of the given size for this media
        /// \param sizeType The targeted thumbnail size
        /// \return An mrl, representing the absolute path to the media thumbnail
        ///         or an empty string, if the thumbnail generation failed
        ///
        /// \sa{isThumbnailGenerated}
        ///
        virtual const std::string& thumbnailMrl( ThumbnailSizeType sizeType ) const = 0;
        ///
        /// \brief isThumbnailGenerated Returns true if a thumbnail generation was
        ///                             attempted, or if a thumbnail was assigned to thie media
        /// \param sizeType The targeted thumbnail size type
        /// In case the thumbnail generation failed, this will still be true, but
        /// the mrl returned by \sa{thumbnailMrl} will be empty.
        /// This is intended as a helper for the client application, so it doesn't
        /// attempt ask for a new thumbmail generation.
        /// \return
        ///
        virtual bool isThumbnailGenerated( ThumbnailSizeType sizeType ) const = 0;
        ///
        /// \brief setThumbnail Sets a thumbnail for the current media
        /// \param mrl A mrl pointing the the thumbnail file.
        /// \param sizeType The targeted thumbnail size type
        /// \return true in case the thumbnail was successfully stored to database
        ///         false otherwise
        /// This is intended to be used by applications that have their own way
        /// of computing thumbnails.
        ///
        virtual bool setThumbnail( const std::string& mrl, ThumbnailSizeType sizeType ) = 0;
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
        virtual bool unsetMetadata( MetadataType type ) = 0;

        ///
        /// \brief removeFromHistory Removes a media from the history.
        /// \return true in case of success, false otherwise
        ///
        /// This can be used for all type of media, including streams & network.
        /// If this call succeeds, the media will have a play count of 0, and
        /// won't appear in the history anymore. Any potential progress will
        /// also be lost.
        /// After calling this method, the observable state is as if the media
        /// was never played.
        ///
        virtual void removeFromHistory() = 0;

        ///
        /// \brief requestThumbnail Queues a thumbnail generation request for
        /// this media, to be run asynchronously.
        /// Upon completion (successful or not) IMediaLibraryCb::onMediaThumbnailReady
        /// will be called.
        /// In case a thumbnail was already generated for the media, a new thumbnail
        /// will be generated, and the previous one will be overriden.
        /// In case a previous thumbnailing attempt failed, false will be returned
        /// and no new generation will occur.
        /// If you want to force a new generation, you need to
        /// call \sa{IMediaLibrary::enableFailedThumbnailRegeneration} beforehand.
        /// \param sizeType The size type of the thumbnail to generate
        /// \param desiredWidth The desired thumbnail width
        /// \param desiredHeight The desired thumbnail height
        /// \param position The position at which to generate the thumbnail, in [0;1] range
        ///
        /// The generated thumbnail will try to oblige by the requested size, while
        /// respecting the source aspect ratio. If the aspect ratios differ, the
        /// source image will be cropped.
        /// If one of the dimension is 0, the other one will be deduced from the
        /// source aspect ratio. If both are 0, the source dimensions will be used.
        ///
        /// This function is thread-safe
        ///
        virtual bool requestThumbnail( ThumbnailSizeType sizeType, uint32_t desiredWidth,
                                       uint32_t desiredHeight, float position ) = 0;

        ///
        /// \brief bookmarks Returns a query representing this media bookmarks
        /// \param params Some query parameters, or nullptr for the default
        ///
        /// The sorting criteria supported for this requests are Alpha & Default
        /// (default being by ascending time)
        /// Any other criteria will fallback to default.
        ///
        virtual Query<IBookmark> bookmarks( const QueryParameters* params ) const = 0;
        ///
        /// \brief bookmark Returns the bookmark at the provided time
        /// \return A bookmark if found, nullptr otherwise
        ///
        virtual BookmarkPtr bookmark( int64_t time ) const = 0;
        ///
        ///
        /// \brief addBookmark Add a bookmark to this media
        /// \param time The bookmark time
        /// \return A pointer to the new bookmark in case of successful
        ///         addition, or nullptr otherwise
        ///
        virtual BookmarkPtr addBookmark( int64_t time ) = 0;
        ///
        /// \brief removeBookmark Removes a bookmark by its time
        /// \param time The time at which the bookmark must be removed.
        /// \return
        ///
        virtual bool removeBookmark( int64_t time ) = 0;
        ///
        /// \brief removeAllBookmarks Remove all bookmarks attached to this media
        ///
        virtual bool removeAllBookmarks() = 0;
};

}
