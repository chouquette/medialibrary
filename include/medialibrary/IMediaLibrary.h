/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs
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
#include <set>
#include <string>

#include "medialibrary/ILogger.h"
#include "Types.h"
#include "IQuery.h"
#include "IMedia.h"

struct libvlc_instance_t;

namespace medialibrary
{

class PriorityAccessImpl;
class PriorityAccess
{
public:
    PriorityAccess( std::unique_ptr<PriorityAccessImpl> p );
    PriorityAccess( PriorityAccess&& );
    ~PriorityAccess();

private:
    std::unique_ptr<PriorityAccessImpl> p;
};

static constexpr auto UnknownArtistID = 1u;
static constexpr auto VariousArtistID = 2u;
static constexpr auto UnknownShowID = 1u;

struct SearchAggregate
{
    Query<IAlbum> albums;
    Query<IArtist> artists;
    Query<IGenre> genres;
    Query<IMedia> media;
    Query<IShow> shows;
    Query<IPlaylist> playlists;
};

enum class SortingCriteria
{
    /*
     * Default depends on the entity type:
     * - By track number (and disc number) for album tracks
     * - Alphabetical order for others
     */
    Default,
    Alpha,
    Duration,
    InsertionDate,
    LastModificationDate,
    ReleaseDate,
    FileSize,
    Artist,
    PlayCount,
    Album,
    Filename,
    // Sort by number of tracks in the containing entity (album, genre, artist, ...)
    TrackNumber,
    // Sort by track ID (Track #1, track #2, ...)
    TrackId,
    // Valid for folders only. Default order is descending
    NbVideo,
    NbAudio,
    // Valid for folders & media groups
    NbMedia,
    // Only valid for artists for now
    NbAlbum,
    LastPlaybackDate,
};


struct QueryParameters
{
    /* This query sorting parameter. Its actual meaning is query dependent */
    SortingCriteria sort = SortingCriteria::Default;
    /* Descending order */
    bool desc = false;
    /* If true, media that are stored on missing devices will still be returned */
    bool includeMissing = false;
};

enum class InitializeResult
{
    ///< Everything worked out fine
    Success,
    ///< Should be considered the same as Success, but is an indication of
    /// unrequired subsequent calls to initialize.
    AlreadyInitialized,
    ///< A fatal error occurred, the IMediaLibrary instance should be destroyed
    Failed,
    ///< The database was reset, the caller needs to re-configure folders to
    /// discover at the bare minimum.
    DbReset,
    ///< Something is wrong with the database. It is up to the application to
    ///< chose what to do, the DB needs to be recovered or dropped in any case.
    DbCorrupted,
};

enum class ThumbnailSizeType : uint8_t
{
    /// A small sized thumbnail. Considered to be the default value before model 17
    Thumbnail,
    /// A banner type thumbnail. The exact size is application dependent.
    Banner,

    /// The number of different size type
    Count,
};

enum class ThumbnailStatus : uint8_t
{
    /// No thumbnail for this entity
    Missing,
    /// This thumbnail was successfully generated or was provided by the user
    /// and is available to use
    Available,
    /// The thumbnail generation failed, without specific reason, usually
    /// because of a timeout.
    /// It is fine to ask for a new generation in this case
    Failure,
    /// The thumbnail generation failed at least 3 times. A new generation might
    /// be required, but is likely to fail again.
    PersistentFailure,
    /// The thumbnail generation failed because of a crash. Asking for a new
    /// generation is not recommended, unless you know the underlying issue was
    /// fixed.
    Crash,
};

enum class HistoryType : uint8_t
{
    /// The history of media analyzed by the media library & external media
    Media,
    /// The network streams history
    Network,
};

enum class ArtistIncluded : uint8_t
{
    /// Include all artists, as long as they have at least one track present
    All,
    /// Do not return the artist that are only doing featurings on some albums.
    /// In other word, this would only return artists which have at least 1 album
    AlbumArtistOnly,
};

enum class PlaylistType : uint8_t
{
    /// Include all kind of playlist, regarding of the media types
    All,
    /// Only include audio playlists
    AudioOnly,
    /// Only include video playlist
    VideoOnly,
};

struct SetupConfig
{
    /**
     * @brief parserServices A vector of external parser services provided by the application
     * @note This currently only supports MetadataExtraction services.
     */
    std::vector<std::shared_ptr<parser::IParserService>> parserServices;
    /**
     * @brief deviceListers Provides some external device listers.
     *
     * The key is the scheme (including the '://') and the value is a IDeviceLister
     * implementation.
     * This is meant for OSes with complicated/impossible to achieve device listing (due to
     * missing APIs, permissions problems...), or for non-local devices, such as
     * network shares.
     */
    std::unordered_map<std::string, DeviceListerPtr> deviceListers;
    /**
     * @brief fsFactories Provides an external filesystem factory implementation
     */
    std::vector<std::shared_ptr<fs::IFileSystemFactory>> fsFactories;

    /**
     * @brief logLevel The default log level to initialize the medialibrary with.
     * This can be overwriten at a later point using IMediaLibrary::setVerbosity
     */
    LogLevel logLevel = LogLevel::Error;

    /**
     * @brief logger An ILogger instance if the application wishes to use a
     * custom one.
     * If nullptr is provided, the default IOstream logger will be used.
     */
    std::shared_ptr<ILogger> logger;
};

class IMediaLibraryCb
{
public:
    virtual ~IMediaLibraryCb() = default;
    /**
     * @brief onMediaAdded Will be called when some media get added.
     * Depending if the media is being restored or was just discovered,
     * the media type might be a best effort guess. If the media was freshly
     * discovered, it is extremely likely that no metadata will be
     * available yet.
     * The number of media is undefined, but is guaranteed to be at least 1.
     */
    virtual void onMediaAdded( std::vector<MediaPtr> media ) = 0;
    /**
     * @brief onMediaUpdated Will be called when a file metadata gets updated.
     */
    virtual void onMediaModified( std::set<int64_t> mediaIds ) = 0;

    virtual void onMediaDeleted( std::set<int64_t> mediaIds ) = 0;

    virtual void onArtistsAdded( std::vector<ArtistPtr> artists ) = 0;
    virtual void onArtistsModified( std::set<int64_t> artistsIds ) = 0;
    virtual void onArtistsDeleted( std::set<int64_t> artistsIds ) = 0;

    virtual void onAlbumsAdded( std::vector<AlbumPtr> albums ) = 0;
    virtual void onAlbumsModified( std::set<int64_t> albumsIds ) = 0;
    virtual void onAlbumsDeleted( std::set<int64_t> albumsIds ) = 0;

    virtual void onPlaylistsAdded( std::vector<PlaylistPtr> playlists ) = 0;
    virtual void onPlaylistsModified( std::set<int64_t> playlistsIds ) = 0;
    virtual void onPlaylistsDeleted( std::set<int64_t> playlistIds ) = 0;

    virtual void onGenresAdded( std::vector<GenrePtr> genres ) = 0;
    virtual void onGenresModified( std::set<int64_t> genresIds ) = 0;
    virtual void onGenresDeleted( std::set<int64_t> genreIds ) = 0;

    virtual void onMediaGroupsAdded( std::vector<MediaGroupPtr> mediaGroups ) = 0;
    virtual void onMediaGroupsModified( std::set<int64_t> mediaGroupsIds ) = 0;
    virtual void onMediaGroupsDeleted( std::set<int64_t> mediaGroupsIds ) = 0;

    virtual void onBookmarksAdded( std::vector<BookmarkPtr> bookmarks ) = 0;
    virtual void onBookmarksModified( std::set<int64_t> bookmarkIds ) = 0;
    virtual void onBookmarksDeleted( std::set<int64_t> bookmarkIds ) = 0;

    virtual void onFoldersAdded( std::vector<FolderPtr> folders ) = 0;
    virtual void onFoldersModified( std::set<int64_t> folderIds ) = 0;
    virtual void onFoldersDeleted( std::set<int64_t> folderIds ) = 0;

    /**
     * @brief onDiscoveryStarted This callback will be invoked when the discoverer
     * starts to crawl an entrypoint that was scheduled for discovery or reload.
     *
     * This callback will be invoked when the discoverer thread gets woken up
     * regardless of how many entry points need to be discovered.
     */
    virtual void onDiscoveryStarted() = 0;
    /**
     * @brief onDiscoveryProgress This callback will be invoked each time the
     * discoverer enters a new folder.
     * @param currentFolder The folder being discovered
     *
     * This callback can be invoked multiple times even though a single entry point was asked to be
     * discovered. ie. In the case of a file system discovery, discovering a folder would make this
     * callback being invoked for all subfolders
     */
    virtual void onDiscoveryProgress( const std::string& currentFolder ) = 0;
    /**
     * @brief onDiscoveryCompleted Will be invoked when the discoverer finishes
     * all its queued operations and goes back to idle.
     *
     * This callback will be invoked once for each invocation fo onDiscoveryStarted
     */
    virtual void onDiscoveryCompleted() = 0;

    /**
     * @brief onDiscoveryFailed Will be invoked when a discovery operation fails
     * @param entryPoint The entry point for which the discovery failed.
     */
    virtual void onDiscoveryFailed( const std::string& entryPoint ) = 0;

    /**
     * @brief onEntryPointAdded will be invoked when an entrypoint gets added
     * @param entryPoint The entry point which was scheduled for discovery
     * @param success A boolean to represent the operation's success
     *
     * This callback will only be emitted the first time the entry point gets
     * processed, after it has been inserted to the database.
     * In case of failure, it might be emited every time the request is sent, since
     * the provided entry point would most likely be invalid, and couldn't be inserted.
     * Later processing of that entry point will still cause \sa{onDiscoveryStarted}
     * \sa{onDiscoveryProgress} and \sa{onDiscoveryCompleted} events to be fired
     * \warning This event will be fired after \sa{onDiscoveryStarted} since we
     * don't know if an entry point is known before starting its processing
     */
    virtual void onEntryPointAdded( const std::string& entryPoint, bool success ) = 0;

    /**
     * @brief onEntryPointRemoved will be invoked when an entrypoint removal
     *                            request gets processsed
     * by the appropriate worker thread.
     * @param entryPoint The entry point which removal was required
     * @param success A boolean representing the operation's success
     */
    virtual void onEntryPointRemoved( const std::string& entryPoint, bool success ) = 0;
    /**
     * @brief onEntryPointBanned will be called when an entrypoint ban request
     *                           is done being processed.
     * @param entryPoint The banned entrypoint
     * @param success A boolean representing the operation's success
     */
    virtual void onEntryPointBanned( const std::string& entryPoint, bool success ) = 0;
    /**
     * @brief onEntryPointUnbanned will be called when an entrypoint unban request
     *                             is done being processed.
     * @param entryPoint The unbanned entrypoint
     * @param success A boolean representing the operation's success
     */
    virtual void onEntryPointUnbanned( const std::string& entryPoint, bool success ) = 0;
    /**
     * @brief onParsingStatsUpdated Called when the parser statistics are updated
     *
     * There is no waranty about how often this will be called.
     * @param opsDone The number of operation the parser completed
     * @param opsScheduled The number of operations currently scheduled by the parser
     *
     */
    virtual void onParsingStatsUpdated( uint32_t opsDone, uint32_t opsScheduled ) = 0;
    /**
     * @brief onBackgroundTasksIdleChanged Called when background tasks idle state change
     *
     * When all parser tasks are idle, it is guaranteed that no entity modification
     * callbacks will be invoked.
     * @param isIdle true when all background tasks are idle, false otherwise
     */
    virtual void onBackgroundTasksIdleChanged( bool isIdle ) = 0;

    /**
     * @brief onMediaThumbnailReady Called when a thumbnail generation completed.
     * @param media The media for which a thumbnail was generated
     * @param sizeType The size type that was requerested
     * @param success true if the thumbnail was generated, false if the generation failed
     */
    virtual void onMediaThumbnailReady( MediaPtr media, ThumbnailSizeType sizeType,
                                        bool success ) = 0;
    /**
     * @brief onHistoryChanged Called when a media history gets modified (including when cleared)
     * @param type The history type
     */
    virtual void onHistoryChanged( HistoryType type ) = 0;

    /**
     * @brief onUnhandledException will be invoked in case of an unhandled exception
     *
     * @param context A minimal context hint
     * @param errMsg  The exception string, as returned by std::exception::what()
     * @param clearSuggested A boolean to inform the application that a database
     *                       clearing is suggested.
     *
     * If the application chooses to handle the error to present it to the user
     * or report it somehow, it should return true.
     * If the implementation returns false, then the exception will be rethrown
     * If clearSuggested is true, the application is advised to call
     * IMediaLibrary::clearDatabase. After doing so, the medialibrary can still
     * be used without any further calls (but will need to rescan the entire user
     * collection). If clearDatabase isn't called, the database should be
     * considered as corrupted, and therefor the medialibrary considered unusable.
     *
     * If clearSuggested is false, there are no certain way of knowing if the
     * database is still usable or not.
     */
    virtual bool onUnhandledException( const char* /* context */,
                                       const char* /* errMsg */,
                                       bool /* clearSuggested */ ) { return false; }
    /**
     * @brief onRescanStarted will be invoked when a rescan is started.
     *
     * This won't be emited when the media library issues a rescan itself, due
     * to a migration.
     */
    virtual void onRescanStarted() = 0;
};

class IMediaLibrary
{
public:
    virtual ~IMediaLibrary() = default;
    /**
     * \brief  initialize Initializes the media library.
     *
     * \param mlCallback    A pointer to an IMediaLibraryCb that will be invoked
     *                      with various events during the medialibrary lifetime.
     * \return An \see{InitializeResult} code.
     *
     * If initialize returns Failed, this medialibrary must not be used
     * anymore, and should be disposed off.
     * If it returns Ok the first time, calling this method again is a no-op and
     * AlreadyInitialized will be returned
     * In case DbReset is returned, it is up to application to decide what
     * to do to repopulate the database.
     *
     * This method is thread safe. If multiple initialization start simultaneously
     * only the first one will return Success, the later ones will return
     * AlreadyInitialized
     */
    virtual InitializeResult initialize( IMediaLibraryCb* mlCallback ) = 0;

    virtual void setVerbosity( LogLevel v ) = 0;

    virtual LabelPtr createLabel( const std::string& label ) = 0;
    virtual bool deleteLabel( LabelPtr label ) = 0;
    virtual MediaPtr media( int64_t mediaId ) const = 0;
    virtual MediaPtr media( const std::string& mrl ) const = 0;
    /**
     * @brief addExternalMedia Adds an external media to the list of known media
     * @param mrl This media MRL
     * @param duration A duration for this media (values <= 0 are ignored)
     *
     * Once created, this media can be used just like any other media, except
     * it won't have a subType, and won't be analyzed to extract tracks and
     * won't be inserted to any collection (ie. album/show/...)
     * If the mrl is already known to the medialibrary, this function will
     * return nullptr.
     *
     * The media can be fetched using media( std::string ) afterward.
     */
    virtual MediaPtr addExternalMedia( const std::string& mrl, int64_t duration ) = 0;
    /**
     * @brief addStream Create an external media of type IMedia::Type::Stream
     *
     * This is equivalent to addExternalMedia, except for the resulting
     * new media's type
     */
    virtual MediaPtr addStream( const std::string& mrl ) = 0;

    /**
     * @brief removeExternalMedia Remove an external media or a stream
     * @return false if the media was not external/stream, or in case of DB failure
     */
    virtual bool removeExternalMedia( MediaPtr media ) = 0;

    /**
     * @brief audioFiles Returns the media classified as Audio
     * @param params Some query parameters.
     * @return A query representing the results set
     *
     * All media accessors throughout the media library suppor the same sorting
     * criteria, which are:
     *   - Duration
     *   - InsertionDate
     *   - ReleaseDate
     *   - PlayCount
     *   - Filename
     *   - LastModificationDate
     *   - FileSize
     * Default sorting parameter uses the media's title, in ascending order
     */
    virtual Query<IMedia> audioFiles( const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief videoFiles Returns the media classified as Video
     * @param params Some query parameters.
     * @return A query representing the results set
     *
     * \see{IMediaLibrary::audioFile} for the supported sorting criteria
     */
    virtual Query<IMedia> videoFiles( const QueryParameters* params = nullptr ) const = 0;

    /**
     * @brief movies Returns the media classified as Movie
     * @param params Some query parameters.
     * @return A query representing the results set
     *
     * \see{IMediaLibrary::videoFiles} for the supported sorting criteria
     */
    virtual Query<IMedia> movies( const QueryParameters* params = nullptr ) const = 0;

    /**
     * @brief inProgressMedia Returns media for which playback wasn't completed
     * @param type The type of media to fetch, or 'Unknown' for all
     * @param params Some query parameters
     * @return A query representing the results set
     *
     * @see{IMedia::setProgress}
     */
    virtual Query<IMedia> inProgressMedia( IMedia::Type type,
                                           const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief createMediaGroup Creates a media group
     * @param name The group name
     * @return The new group instance, or nullptr in case of error
     */
    virtual MediaGroupPtr createMediaGroup( std::string name ) = 0;
    /**
     * @brief createMediaGroup Creates a media group with the provided media
     * @param mediaIds A list of media to be included in the group
     * @return The new group instance, or nullptr in case of error
     *
     * If the provided media are already part of a group, they will be moved to
     * the newly created one.
     * The group will have no name and will return an empty string.
     */
    virtual MediaGroupPtr createMediaGroup( const std::vector<int64_t>& mediaIds ) = 0;
    /**
     * @brief deleteMediaGroup Deletes a media group
     * @param id The group ID
     * @return true in case of success, false otherwise
     *
     * This will ungroup all media that were part of the group.
     */
    virtual bool deleteMediaGroup( int64_t id ) = 0;
    /**
     * @brief mediaGroup Returns a media group with the given id
     * @return A media group, or nullptr if the group doesn't exist, or in case
     *         of sporadic failure.
     */
    virtual MediaGroupPtr mediaGroup( int64_t id ) const = 0;
    /**
     * @brief mediaGroups Returns a query representing the root media groups
     * @param mediaType The type of media contained in this group, or Unknown if all
     *                  types should be returned
     * @param params A query parameter
     *
     * The supported sorting criteria are:
     * - Alpha (default)
     * - NbVideo
     * - NbAudio
     * - NbMedia
     */
    virtual Query<IMediaGroup> mediaGroups( IMedia::Type mediaType,
                                            const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IMediaGroup> searchMediaGroups( const std::string& pattern,
                                                  const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief regroupAll Attemps to regroup all media that belong to a forced singleton group
     *
     * This will try to regroup all media that were manually removed from their
     * group, and now belong to a forced singleton group.
     * Media that belong to a group of only 1 element will not be affected by this.
     * Usual regrouping rules apply, meaning that a minimum of 6 characters match
     * is required for 2 media to be grouped together, and if applicable, the longest
     * match will be used to name the created group
     * In case of error, false will be returned, but some media might have been
     * regrouped already.
     *
     * @warning This might be a relatively long operation as it must fetch the
     *          first media being part of a singleton group and regroup it with
     *          its matching media, in a loop, until all media are regrouped
     */
    virtual bool regroupAll() = 0;
    virtual AlbumPtr album( int64_t id ) const = 0;
    virtual Query<IAlbum> albums( const QueryParameters* params = nullptr ) const = 0;
    virtual ShowPtr show( int64_t id ) const = 0;
    virtual MoviePtr movie( int64_t id ) const = 0;
    virtual ArtistPtr artist( int64_t id ) const = 0;
    virtual Query<IShow> shows( const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IShow> searchShows( const std::string& pattern,
                                      const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief artists List all artists that have at least an album.
     * Artists that only appear on albums as guests won't be listed from here, but will be
     * returned when querying an album for all its appearing artists
     * @param included If true, all artists including those without album
     *                   will be returned. If false, only artists which have
     *                   an album will be returned.
     * @param params Some query parameters
     *
     * This function only handles lexical sort
     */
    virtual Query<IArtist> artists( ArtistIncluded included,
                                    const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief genres Return the list of music genres
     * @param params Some query parameters
     */
    virtual Query<IGenre> genres( const QueryParameters* params = nullptr ) const = 0;
    virtual GenrePtr genre( int64_t id ) const = 0;
    /***
     *  Playlists
     */
    virtual PlaylistPtr createPlaylist( std::string name ) = 0;
    /**
     * @brief playlists List all playlists known to the media library
     * @param type The type of playlist to return
     * @param params Some query parameters
     * @return A Query object
     *
     * The provided playlist type allows the application to fetch the playlist containing
     * only Video/Audio media. Depending on QueryParameters::includeMissing
     * missing media will or will not be included.
     * This means that at a given time, a playlist might be considered an audio
     * playlist if all the video it contains are on a remove device. When the
     * device comes back, the playlist will turn back to a non-audio playlist.
     *
     * If a playlist contains a media of unknown type, it is assumed to be a video.
     *
     * If a playlist is empty, it will only be returned for PlaylistType::All
     */
    virtual Query<IPlaylist> playlists( PlaylistType type,
                                        const QueryParameters* params = nullptr ) = 0;
    virtual PlaylistPtr playlist( int64_t id ) const = 0;
    virtual bool deletePlaylist( int64_t playlistId ) = 0;

    /**
     * History
     */
    virtual Query<IMedia> history() const = 0;
    /**
     * @brief history Returns the history for media of the provided type
     * @param type Can be either Audio or Video. Other types are not supported
     */
    virtual Query<IMedia> history( IMedia::Type type ) const = 0;
    virtual Query<IMedia> streamHistory() const = 0;
    /**
     * @brief clearHistory will clear both streams history & media history.
     * @return true in case of success, false otherwise. The database will stay untouched in case
     *              of failure.
     *
     * This will clear all history, and also reset any potential playback progress
     * for all media
     */
    virtual bool clearHistory() = 0;

    /**
     * Search
     */

    /**
     * @brief searchMedia, searchAudio, and searchVideo search for some media, based on a pattern.
     * @param pattern A 3 character or more pattern that will be matched against the media's title
     *                or filename if no title was set for this media.
     * @param params Some query parameters.
     *
     * Only media that were discovered by the medialibrary will be included.
     * For instance, media that are added explicitely, playlist items that
     * point to remote content, will *not* be included
     *
     * \see{IMediaLibrary::audioFile} for the supported sorting criteria
     */
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IMedia> searchAudio( const std::string& pattern,
                                       const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IMedia> searchVideo( const std::string& pattern,
                                       const QueryParameters* params = nullptr ) const = 0;

    virtual Query<IPlaylist> searchPlaylists( const std::string& name,
                                              const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IAlbum> searchAlbums( const std::string& pattern,
                                        const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IGenre> searchGenre( const std::string& genre,
                                       const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IArtist> searchArtists( const std::string& name, ArtistIncluded included,
                                          const QueryParameters* params = nullptr  ) const = 0;
    virtual SearchAggregate search( const std::string& pattern,
                                    const QueryParameters* params = nullptr ) const = 0;

    /**
     * @brief discover Launch a discovery on the provided entry point.
     * This will start the discoverer thread, device listers, and file system
     * factories if needed
     * The actuall discovery will run asynchronously, meaning this method will immediatly return.
     * Depending on which discoverer modules where provided, this might or might not work
     * @note This must be called after initialize()
     * @param entryPoint The MRL of the entrypoint to discover.
     */
    virtual void discover( const std::string& entryPoint ) = 0;
    /**
     * @brief setDiscoverNetworkEnabled Enable discovery of network shares
     * @return true In case of success, false otherwise.
     *
     * This can be called at any time.
     * If enabling before the discoverer thread gets started, the intent will
     * be stored, but the device listers nor the file system factories won't be
     * started.
     * When disabling network discovery, all content that was discovered on
     * the network will be marked as non-present, meaning they won't be
     * returned until network discovery gets enabled again.
     * As far as the user is concerned, this is equivalent to (un)plugging
     * a USB drive, in the sense that the medialibrary will still store
     * information about network content and won't have to discover/parse it
     * again.
     * When enabling, this method will consider the lack of network enabled
     * IFsFactory implementation a failure.
     * Setting the same state twice is considered as a success, regardless of
     * the previous call status.
     */
    virtual bool setDiscoverNetworkEnabled( bool enabled ) = 0;
    virtual bool isDiscoverNetworkEnabled() const = 0;
    /**
     * @brief entryPoints List the entrypoints that are managed by the medialibrary
     *
     * The resulting list includes entry points on device that are currently unmounted
     */
    virtual Query<IFolder> entryPoints() const = 0;
    /**
     * @brief isIndexed Returns true if the mrl point to a file of folder in an
     *                  indexed entrypoint
     * @param mrl The MRL to probe
     * @return true if the mrl is indexed, false otherwise
     */
    virtual bool isIndexed( const std::string& mrl ) const = 0;
    /**
     * @brief isBanned Returns true if the folder represented by the MRL is banned
     * @param mrl The folder MRL to probe
     * @return true is banned, false otherwise (including if the MRL is not matching
     *         any known folder)
     */
    virtual bool isBanned( const std::string& mrl ) const = 0;
    /**
     * @brief folders Returns a flattened list of all folders containing at
     *                least a media of a given type
     * @param type A required type of media, or IMedia::Type::Unknown if any
     *             media type is fine.
     * @param params A query parameters object
     * @return A query object to be used to fetch the results
     *
     * This is flattened, ie.
     * ├── a
     * │   └── w
     * │       └── x
     * │           └── y
     * │               └── z
     * │                   └── DogMeme.avi
     * ├── c
     * │   └── NakedMoleRat.asf
     *
     * would return a query containing 'z' and 'c' as the other folders are
     * not containing any media.
     * In case a non flattened list is desired, the
     * entryPoints() & IFolder::subFolders() functions should be used.
     */
    virtual Query<IFolder> folders( IMedia::Type type,
                                    const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IFolder> searchFolders( const std::string& pattern,
                                          IMedia::Type type,
                                          const QueryParameters* params = nullptr ) const = 0;
    virtual FolderPtr folder( int64_t folderId ) const = 0;
    virtual FolderPtr folder( const std::string& mrl ) const = 0;
    /**
     * @brief removeEntryPoint Removes an entry point
     * @param entryPoint The MRL of the entry point to remove
     *
     * This will remove the provided entry point from the list of know locations
     * to manage by the media library.
     * The location will be ignored afterward, even if it is a sub folder of
     * another managed location.
     * This can be reverted by calling unbanFolder
     * @note This method is asynchronous, but will interrupt any ongoing
     *       discovery, process the request, and resume the previously running
     *       task
     * @note This must be called after initialize()
     */
    virtual void removeEntryPoint( const std::string& entryPoint ) = 0;
    /**
     * @brief banFolder will prevent an entry point folder from being discovered.
     * If the folder was already discovered, it will be removed prior to the ban, and all
     * associated media will be discarded.
     * @param mrl The MRL to ban
     * @note This method is asynchronous, but will interrupt any ongoing
     *       discovery, process the request, and resume the previously running
     *       task
     * @note This must be called after initialize()
     */
    virtual void banFolder( const std::string& mrl ) = 0;
    /**
     * @brief unbanFolder Unban an entrypoint.
     * In case this entry point was indeed previously banned, this will issue a reload of
     * that entry point
     * @param mrl The MRL to unban
     * @note This method is asynchronous, but will interrupt any ongoing
     *       discovery, process the request, and resume the previously running
     *       task
     * @note This must be called after initialize()
     */
    virtual void unbanFolder( const std::string& mrl ) = 0;
    /**
     * @brief bannedEntryPoints Returns a query representing the banned entry points
     *
     * The result set will include entry points on missing devices as well. Folder
     * hierarchy isn't preserved, and the results are flattened.
     */
    virtual Query<IFolder> bannedEntryPoints() const = 0;
    /**
     * @brief pauseBackgroundOperations Will stop potentially CPU intensive background
     * operations, until resumeBackgroundOperations() is called.
     * If an operation is currently running, it will finish before pausing.
     */
    virtual void pauseBackgroundOperations() = 0;
    /**
     * @brief resumeBackgroundOperations Resumes background tasks, previously
     * interrupted by pauseBackgroundOperations().
     */
    virtual void resumeBackgroundOperations() = 0;
    /**
     * @brief reload Reload all known entry points
     * @note This must be called after initialize()
     *
     * This will start the discoverer thread, appropriate device listers and
     * file system factories if needed.
     */
    virtual void reload() = 0;
    /**
     * @brief reload Reload a specific entry point
     * @param entryPoint The entrypoint to reload
     * @note This must be called after initialize()
     *
     * This will start the discoverer thread, appropriate device listers and
     * file system factories if needed.
     */
    virtual void reload( const std::string& entryPoint ) = 0;
    /**
     * @brief forceParserRetry Forces a re-run of all metadata parsers and resets any
     * unterminated file retry count to 0, granting them 3 new tries at being parsed
     */
    virtual bool forceParserRetry() = 0;

    /**
     * @brief deviceLister Get a device lister for the provided scheme
     * @param scheme The scheme for which a device lister is required
     * @return A device lister instance, if any.
     *
     * This will return the device lister provided to the constructor through
     * SetupConfig, or a media library provided one.
     * If no device lister is available for this scheme, nullptr will be returned
     */
    virtual DeviceListerPtr deviceLister( const std::string& scheme ) const = 0;

    /**
     * @brief forceRescan Deletes all entities except Media and Playlist, and
     *                    forces all media to be rescanned.
     *
     * This can be called anytime after the medialibrary has been initialized.
     * It will make all held instances outdated. Those should be considered
     * as invalid the moment this method returns.
     *
     * This will return false in case of a database error. If this happens,
     * nothing will be updated.
     */
    virtual bool forceRescan() = 0;

    /**
     * @brief enableFailedThumbnailRegeneration Allow failed thumbnail attempt to be retried
     *
     * This will not attempt to regenerate the thumbnail immediatly, requestThumbnail
     * still has to be called afterward.
     */
    virtual void enableFailedThumbnailRegeneration() = 0;

    virtual void addThumbnailer( std::shared_ptr<IThumbnailer> thumbnailer ) = 0;

    /**
     * @brief clearDatabase Will drop & recreate the database
     * @param restorePlaylists If true, the media library will attempt to keep
     *                         the user created playlists
     */
    virtual bool clearDatabase( bool restorePlaylists ) = 0;

    /**
     * @brief supportedMediaExtensions Returns the supported media extensions
     *
     * The list is guaranteed to be ordered alphabetically
     */
    virtual const std::vector<const char*>& supportedMediaExtensions() const = 0;
    /**
     * @brief isMediaExtensionSupported Checks if the provided media extension
     *                                  is supported.
     */
    virtual bool isMediaExtensionSupported( const char* ext ) const = 0;
    /**
     * @brief supportedPlaylistExtensions Returns the supported playlist extensions
     *
     * The list is guaranteed to be ordered alphabetically
     */
    virtual const std::vector<const char*>& supportedPlaylistExtensions() const = 0;
    /**
     * @brief isPlaylistExtensionSupported Checks if the provided playlist extension
     *                                     is supported.
     */
    virtual bool isPlaylistExtensionSupported( const char* ext ) const = 0;

    /**
     * @brief isDeviceKnown Probes a device to know if it has been seen by the medialibrary
     * @param uuid The device UUID
     * @param mountpoint The device mountpoint, as an MRL
     * @param isRemovable The device's removable state
     * @return true if the device has been seen by the media library
     *
     * If this function returns false, a representation for this device will be
     * inserted in database, and any later call will return true.
     * This must be called *after* calling initialize()
     */
    virtual bool isDeviceKnown( const std::string& uuid,
                                const std::string& mountpoint,
                                bool isRemovable ) = 0;
    /**
     * @brief deleteRemovableDevices Deletes all removable devices
     * @return true if devices were deleted, false otherwise
     *
     * This will delete *ALL* removable devices from the database, causing *ALL*
     * files & media stored on that device to be deleted as well.
     * This is intended for applications with an external device lister to
     * recover in case of an issue causing multiple devices or invalide entries
     * to be inserted in the database
     */
    virtual bool deleteRemovableDevices() = 0;

    /**
     * @brief supportedSubtitlesExtensions Returns the supported subtitle extensions
     *
     * The list is guaranteed to be ordered alphabetically
     */
    virtual const std::vector<const char*>& supportedSubtitleExtensions() const = 0;
    /**
     * @brief isSubtitleExtensionSupported Checks if the provided subtitle extension
     *                                     is supported.
     */
    virtual bool isSubtitleExtensionSupported( const char* ext ) const = 0;

    virtual bool requestThumbnail( int64_t mediaId, ThumbnailSizeType sizeType,
                                   uint32_t desiredWidth, uint32_t desiredHeight,
                                   float position ) = 0;

    virtual BookmarkPtr bookmark( int64_t bookmarkId ) const = 0;

    /**
     * @brief setExternalLibvlcInstance Provides an existing libvlc instance
     * @param inst A libvlc instance which can be released as soon as this function returns
     * @return true if the instance was correctly set
     *
     * If the media library is build with libvlc support, this can't fail.
     * This must be called before any discovery or parsing is started, so ideally
     * the user should call this right after creating an IMediaLibrary instance
     * through NewMediaLibrary
     */
    virtual bool setExternalLibvlcInstance( libvlc_instance_t* inst ) = 0;

    virtual PriorityAccess acquirePriorityAccess() = 0;

    /**
     * @brief flushUserProvidedThumbnails Removes all user provided thumbnail from the database
     * @return true in case of success, false otherwise.
     *
     * The media library will *not* attempt to remove the files, as it doesn't
     * own them.
     * This will only make sure that any media that the application provided a
     * thumbnail for will not have a thumbnail associated with it anymore.
     */
    virtual bool flushUserProvidedThumbnails() = 0;
};

}

extern "C"
{
    /**
     * @brief NewMediaLibrary Creates a media library instance
     * @param dbPath The path to the database.
     * @param mlFolderPath The path to the dedicated media library folder
     * @param lockFile A boolean that represents the need for locking the media library
     * @param cfg A pointer to additional configurations. Can be nullptr
     * @return A media library instance
     *
     * If lockFile is true, nullptr will be returned if another instance holds
     * the same media library directory.
     * \see{SetupConfig}
     */
    medialibrary::IMediaLibrary* NewMediaLibrary( const char* dbPath,
                                                  const char* mlFolderPath,
                                                  bool lockFile,
                                                  const medialibrary::SetupConfig* cfg );
}
