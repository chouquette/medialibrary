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
#include <unordered_map>

#include "IFile.h"
#include "Types.h"
#include "IQuery.h"

namespace medialibrary
{

class IShowEpisode;
class ITrackInformation;
class IMetadata;
struct QueryParameters;
enum class ThumbnailSizeType : uint8_t;
enum class ThumbnailStatus : uint8_t;

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
        /*
         * Removed starting from model 27, this is now a full field in the
         * media table
         * Progress = 50,
         */
        Speed = 51,
        Title,
        Chapter,
        Program,
        // Seen, // Replaced by the media playcount

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
    static constexpr size_t NbMeta = 17;

    /**
     * @brief The ProgressResult enum describes the result for setLastPosition
     * and setLastTime operations
     */
    enum class ProgressResult : uint8_t
    {
        /// An error occured and the progress wasn't changed
        Error,
        /// The provided position/time was interpreted as the beginning of the
        /// media and has been reset to -1. This nedia playback is now not
        /// considered started.
        Begin,
        /// The provided position/time was not interpreted as a special position
        /// and was updated as provided in the database. The playback will be
        /// considered in progress
        AsIs,
        /// The provided position/time was interpreted as the end of the media.
        /// The playback will not be considered in progress anymore and the
        /// play count has been incremented.
        End,
    };

    virtual ~IMedia() = default;

    virtual int64_t id() const = 0;
    virtual Type type() const = 0;
    /**
     * @brief setType Updates this media's type
     * @param type The new type
     * @return true in case of success, false otherwise.
     *
     * If the media type was Unknown before, this will trigger a refresh for
     * this media.
     * If the refresh task fails to be created, false will be returned, and
     * the media will stay unmodified.
     */
    virtual bool setType( Type type ) = 0;
    virtual SubType subType() const = 0;
    virtual const std::string& title() const = 0;
    /**
     * @brief setTitle Enforces a title for this media
     * @param title The new title
     * @return true if the title was successfully modified, false otherwise
     */
    virtual bool setTitle( const std::string& title ) = 0;
    /**
     * @brief duration Returns the media duration in ms
     */
    virtual int64_t duration() const = 0;
    virtual uint32_t playCount() const = 0;
    /**
     * @brief lastPosition Returns the last saved progress
     *
     * This is the same unit as VLC's playback position, ie. a float between
     * 0 and 1.
     * If the value is negative, it means the playback has either never been
     * played, or it was played to completion
     * If the duration is unknown, the media library will just return what the
     * application provided during its last call to setLastPosition()
     */
    virtual float lastPosition() const = 0;
    /**
     * @brief setLastPosition updates the last playback position
     *
     * @param lastPosition The current playback position expressed by a number in the range [0;1]
     * @return a ProgressResult value indicating how the value was intepreted and
     *         if the operation succeeded
     *
     * The media library will interpret the value to determine if the playback
     * is completed and the media should be marked as watched (therefor increasing
     * the playcount). If the progress isn't large enough, the media library will
     * ignore the new progress.
     * The base value for the beginning/end of a media is 5%, meaning that the
     * first 5% will not increase the progress, and the last 5% will mark the
     * media as watched and reset the progress value (so that next playback
     * restarts from the beginning)
     * These 5% are decreased by 1% for every playback hour, so for instance, a
     * 3h movie will use 5% - (3h * 1%), so the first 2% will be ignored, the last
     * 2% will trigger the completion.
     * If the media duration is unknown, the progress will be stored as-is in
     * database but the playcount will not be updated, nor will the position be
     * clamped in the [0;1] range.
     *
     * Calling lastPosition() or playCount() afterward will fetch the curated values.
     * This will also bump the media last played date, causing it to appear at
     * the top of the history.
     * If the duration is known, this will also update lastTime().
     * If the duration is unknown, lastTime will be set to -1 when this function
     * is called.
     */
    virtual IMedia::ProgressResult setLastPosition( float lastPosition ) = 0;
    /**
     * @brief lastTime Returns the last playback time as provided by the application
     *
     * This is expected to be a time in millisecond, but is ultimately what the
     * application provided to the media library.
     * This defaults to -1 is the playback isn't in progress
     */
    virtual int64_t lastTime() const = 0;
    /**
     * @brief setLastTime Sets the last playback time.
     * @param lastTime A time in millisecond
     * @return a ProgressResult value indicating how the value was intepreted and
     *         if the operation succeeded
     *
     * This is similar to setLastPosition but works with a time in
     * milliseconds rather than a percentage.
     * If the duration is unknown, calling this function will reset the lastProgress
     * to -1.
     */
    virtual IMedia::ProgressResult setLastTime( int64_t lastTime ) = 0;
    /**
     * @brief setPlayCount Set a specific value to this media's play count
     *
     * This is mostly intended for migrations where single step increment
     * would not be the most efficient way.
     * This method will not bump the media in the history
     */
    virtual bool setPlayCount( uint32_t playCount ) = 0;
    virtual time_t lastPlayedDate() const = 0;
    /**
     * @brief markAsPlayed Will mark the media as played and will bump it in the
     *                     history
     * @return true in case of success, false otherwise
     *
     * This is intended as an alternative to setLastPosition/setLastTime in case
     * where the user isn't interested in saving the progression in database, but
     * still cares about the media appearing in the history and using its play
     * count.
     */
    virtual bool markAsPlayed() = 0;
    virtual ShowEpisodePtr showEpisode() const = 0;
    virtual const std::vector<FilePtr>& files() const = 0;
    /**
     * @brief addFile Add a file to this media
     * @param mrl The new file mrl
     * @param fileType The new file type
     * @return The file pointer
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
    virtual Query<IChapter> chapters( const QueryParameters* params = nullptr ) const = 0;
    ///
    /// \brief thumbnail Returns the mrl of a thumbnail of the given size for this media
    /// \param sizeType The targeted thumbnail size
    /// \return An mrl, representing the absolute path to the media thumbnail
    ///         or an empty string, if the thumbnail generation failed or
    ///         was never requested
    ///
    /// \sa{thumbnailStatus}
    ///
    virtual const std::string& thumbnailMrl( ThumbnailSizeType sizeType ) const = 0;

    ///
    /// \brief thumbnailStatus Returns this media thumbnail status
    /// \param sizeType The targeted thumbnail size
    ///
    /// This will return Missing if no thumbnail generation has been requested
    /// for this media, or Success/Failure/Crash, depending on the generation
    /// results.
    ///
    virtual ThumbnailStatus thumbnailStatus( ThumbnailSizeType sizeType ) const = 0;

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

    ///
    /// \brief requestThumbnail Queues a thumbnail generation request for
    /// this media, to be run asynchronously.
    /// Upon completion (successful or not) IMediaLibraryCb::onMediaThumbnailReady
    /// will be called.
    /// In case a thumbnail was already generated for the media, a new thumbnail
    /// will be generated, and the previous one will be overriden.
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
    /// \brief removeThumbnail Clear this media's thumbnail
    /// \param sizeType The thumbnail size type
    /// \return true if the thumbnail was successfully cleared, false otherwise
    ///
    /// If this is successful, later calls to thumbnailStatus will return Missing
    ///
    virtual bool removeThumbnail( ThumbnailSizeType sizeType ) = 0;

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
    /// \brief metadata Returns all the meta set for this device.
    ///
    virtual std::unordered_map<MetadataType, std::string> metadata() const = 0;
    ///
    /// \brief setMetadata Immediatly saves a metadata in database
    ///
    virtual bool setMetadata( MetadataType type, const std::string& value ) = 0;
    virtual bool setMetadata( MetadataType type, int64_t value ) = 0;
    virtual bool unsetMetadata( MetadataType type ) = 0;
    ///
    /// \brief setMetadata Sets multiple metadata at once
    /// \param meta An unordered_map, indexed by the provided meta, containing a string as value
    /// \return true if all meta were successfully set, false otherwise.
    ///
    /// If this function returns false, no meta will have been updated.
    ///
    virtual bool setMetadata( const std::unordered_map<MetadataType, std::string>& meta ) = 0;

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
    /// This will return false in case of a database failure
    ///
    virtual bool removeFromHistory() = 0;

    ///
    /// \brief bookmarks Returns a query representing this media bookmarks
    /// \param params Some query parameters, or nullptr for the default
    ///
    /// The sorting criteria supported for this requests are Alpha & Default
    /// (default being by ascending time)
    /// Any other criteria will fallback to default.
    ///
    virtual Query<IBookmark> bookmarks( const QueryParameters* params = nullptr ) const = 0;
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
    /// \return false in case of a database error
    ///
    virtual bool removeBookmark( int64_t time ) = 0;
    ///
    /// \brief removeAllBookmarks Remove all bookmarks attached to this media
    ///
    virtual bool removeAllBookmarks() = 0;
    ///
    /// \brief isDiscoveredMedia Returns true if this media was discovered
    ///                          during a scan.
    /// false means that the media has been explicitely added by the user
    /// as a stream, or an external media
    ///
    virtual bool isDiscoveredMedia() const = 0;
    ///
    /// \brief isExternalMedia Returns true if the media was explicitely added
    ///                        by the application.
    /// This is the opposite counterpart of isDiscoveredMedia
    ///
    virtual bool isExternalMedia() const = 0;
    ///
    /// \brief isStream Returns true if this media is an external media, and
    ///                  of type stream.
    ///
    virtual bool isStream() const = 0;
    ///
    /// \brief addToGroup Adds this media to the given group
    /// \param group The target media group
    /// \return true if the media was successfully added, false otherwise
    ///
    virtual bool addToGroup( IMediaGroup& group ) = 0;
    ///
    /// \brief addToGroup Adds this media to the given group
    /// \param groupId The target group ID
    /// \return true if the media was successfully added, false otherwise
    ///
    virtual bool addToGroup( int64_t groupId ) = 0;
    ///
    /// \brief removeFromGroup Remove this media from its group
    /// \return true if the media was successfully removed, false otherwise.
    ///
    virtual bool removeFromGroup() = 0;
    ///
    /// \brief groupId Returns this media's group ID
    ///
    virtual int64_t groupId() const = 0;
    ///
    /// \brief group Return this media's group
    ///
    virtual MediaGroupPtr group() const = 0;
    ///
    /// \brief regroup Attempts to group this media with other ungrouped media
    /// \return true in case of success, false otherwise
    ///
    /// This will attempt to find other ungrouped media which start with the
    /// same prefix (currently, 6 characters) as the current media.
    /// This can only be used on ungroupped media, as we don't want to tinkle
    /// with groups that may have been organized manually by the user.
    ///
    virtual bool regroup() = 0;
    ///
    /// \brief isPresent Returns true if the media is present
    ///
    /// The media is considered present if the device containing its main file
    /// is present (ie. if a removable drive is mounted, or a network drive
    /// connected)
    /// This is only relevent when the media is not external
    ///
    virtual bool isPresent() const = 0;

    virtual ArtistPtr artist() const = 0;
    virtual int64_t artistId() const = 0;
    virtual GenrePtr genre() const = 0;
    virtual int64_t genreId() const = 0;
    virtual unsigned int trackNumber() const = 0;
    virtual AlbumPtr album() const = 0;
    virtual int64_t albumId() const = 0;
    /**
     * @return Which disc this tracks appears on (or 0 if unspecified)
     */
    virtual uint32_t discNumber() const = 0;
    /**
     * @brief lyrics returns the lyrics associated with this media, if any
     */
    virtual const std::string& lyrics() const = 0;
};

}
