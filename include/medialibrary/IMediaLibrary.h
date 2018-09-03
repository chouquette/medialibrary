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

#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>

#include "medialibrary/ILogger.h"
#include "Types.h"
#include "IQuery.h"

namespace medialibrary
{

static constexpr auto UnknownArtistID = 1u;
static constexpr auto VariousArtistID = 2u;

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
    TrackNumber,
};


struct QueryParameters
{
    SortingCriteria sort;
    bool desc;
};

enum class InitializeResult
{
    //< Everything worked out fine
    Success,
    //< Should be considered the same as Success, but is an indication of
    // unrequired subsequent calls to initialize.
    AlreadyInitialized,
    //< A fatal error occured, the IMediaLibrary instance should be destroyed
    Failed,
    //< The database was reset, the caller needs to re-configure folders to
    // discover at the bare minimum.
    DbReset,
};

class IMediaLibraryCb
{
public:
    virtual ~IMediaLibraryCb() = default;
    /**
     * @brief onFileAdded Will be called when some media get added.
     * Depending if the media is being restored or was just discovered,
     * the media type might be a best effort guess. If the media was freshly
     * discovered, it is extremely likely that no metadata will be
     * available yet.
     * The number of media is undefined, but is guaranteed to be at least 1.
     */
    virtual void onMediaAdded( std::vector<MediaPtr> media ) = 0;
    /**
     * @brief onFileUpdated Will be called when a file metadata gets updated.
     */
    virtual void onMediaModified( std::vector<MediaPtr> media ) = 0;

    virtual void onMediaDeleted( std::vector<int64_t> mediaIds ) = 0;

    virtual void onArtistsAdded( std::vector<ArtistPtr> artists ) = 0;
    virtual void onArtistsModified( std::vector<ArtistPtr> artists ) = 0;
    virtual void onArtistsDeleted( std::vector<int64_t> artistsIds ) = 0;

    virtual void onAlbumsAdded( std::vector<AlbumPtr> albums ) = 0;
    virtual void onAlbumsModified( std::vector<AlbumPtr> albums ) = 0;
    virtual void onAlbumsDeleted( std::vector<int64_t> albumsIds ) = 0;

    virtual void onPlaylistsAdded( std::vector<PlaylistPtr> playlists ) = 0;
    virtual void onPlaylistsModified( std::vector<PlaylistPtr> playlists ) = 0;
    virtual void onPlaylistsDeleted( std::vector<int64_t> playlistIds ) = 0;

    virtual void onGenresAdded( std::vector<GenrePtr> genres ) = 0;
    virtual void onGenresModified( std::vector<GenrePtr> genres ) = 0;
    virtual void onGenresDeleted( std::vector<int64_t> genreIds ) = 0;

    /**
     * @brief onDiscoveryStarted This callback will be invoked when a folder queued for discovery
     * (by calling IMediaLibrary::discover()) gets processed.
     * @param entryPoint The entrypoint being discovered
     * This callback will be invoked once per endpoint.
     */
    virtual void onDiscoveryStarted( const std::string& entryPoint ) = 0;
    /**
     * @brief onDiscoveryProgress This callback will be invoked each time the discoverer enters a new
     * entrypoint. Typically, everytime it enters a new folder.
     * @param entryPoint The entrypoint being discovered
     * This callback can be invoked multiple times even though a single entry point was asked to be
     * discovered. ie. In the case of a file system discovery, discovering a folder would make this
     * callback being invoked for all subfolders
     */
    virtual void onDiscoveryProgress( const std::string& entryPoint ) = 0;
    /**
     * @brief onDiscoveryCompleted Will be invoked when the discovery of a specified entrypoint has
     * completed.
     * ie. in the case of a filesystem discovery, once the folder, and all its files and subfolders
     * have been discovered.
     * This will also be invoked with an empty entryPoint when the initial reload of the medialibrary
     * has completed.
     */
    virtual void onDiscoveryCompleted( const std::string& entryPoint, bool sucess ) = 0;
    /**
     * @brief onReloadStarted will be invoked when a reload operation begins.
     * @param entryPoint Will be an empty string is the reload is a global reload, or the specific
     * entry point that gets reloaded
     */
    virtual void onReloadStarted( const std::string& entryPoint ) = 0;
    /**
     * @brief onReloadCompleted will be invoked when a reload operation gets completed.
     * @param entryPoint Will be an empty string is the reload was a global reload, or the specific
     * entry point that has been reloaded
     */
    virtual void onReloadCompleted( const std::string& entryPoint, bool success ) = 0;
    /**
     * @brief onEntryPointRemoved will be invoked when an entrypoint removal request gets processsed
     * by the appropriate worker thread.
     * @param entryPoint The entry point which removal was required
     * @param success A boolean representing the operation's success
     */
    virtual void onEntryPointRemoved( const std::string& entryPoint, bool success ) = 0;
    /**
     * @brief onEntryPointBanned will be called when an entrypoint ban request is done being processed.
     * @param entryPoint The banned entrypoint
     * @param success A boolean representing the operation's success
     */
    virtual void onEntryPointBanned( const std::string& entryPoint, bool success ) = 0;
    /**
     * @brief onEntryPointUnbanned will be called when an entrypoint unban request is done being processed.
     * @param entryPoint The unbanned entrypoint
     * @param success A boolean representing the operation's success
     */
    virtual void onEntryPointUnbanned( const std::string& entryPoint, bool success ) = 0;
    /**
     * @brief onParsingStatsUpdated Called when the parser statistics are updated
     *
     * There is no waranty about how often this will be called.
     * @param percent The progress percentage [0,100]
     *
     */
    virtual void onParsingStatsUpdated( uint32_t percent) = 0;
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
     * @param success true if the thumbnail was generated, false if the generation failed
     */
    virtual void onMediaThumbnailReady( MediaPtr media, bool success ) = 0;
};

class IMediaLibrary
{
    public:
        virtual ~IMediaLibrary() = default;
        /**
         * \brief  initialize Initializes the media library.
         *
         * In case the application uses a specific IDeviceLister, the device lister must be properly
         * initialized and ready to return a list of all known devices before calling this method.
         *
         * \param dbPath        Path to the database file
         * \param thumbnailPath Path to a folder that will contain the thumbnails. It will be
         *                      created if required.
         * \param mlCallback    A pointer to an IMediaLibraryCb that will be invoked with various
         *                      events during the medialibrary lifetime.
         * \return true in case of success, false otherwise
         * If initialize returns Fail, this medialibrary must not be used
         * anymore, and should be disposed off.
         * If it returns Ok the first time, calling this method again is a no-op
         * In case DbReset is returned, it is up to application to decide what
         * to do to repopulate the database.
         */
        virtual InitializeResult initialize( const std::string& dbPath, const std::string& thumbnailPath, IMediaLibraryCb* mlCallback ) = 0;

        /**
         * @brief start Starts the background thread and reload the medialibrary content
         * This *MUST* be called after initialize.
         *
         * The user is expected to populate its device lister between a call to initialize() and a
         * call to start().
         * Once this method has been called, the medialibrary will know all the device known to the
         * device lister, and it become impossible to know wether a removable storage device has been
         * inserted for the first time or not.
         *
         * @return true in case of success, false otherwise.
         * * If start returns false, this medialibrary must not be used anymore, and should be
         * disposed off.
         * If it returns true the first time, calling this method again is a no-op
         */
        virtual bool start() = 0;
        virtual void setVerbosity( LogLevel v ) = 0;

        virtual LabelPtr createLabel( const std::string& label ) = 0;
        virtual bool deleteLabel( LabelPtr label ) = 0;
        virtual MediaPtr media( int64_t mediaId ) const = 0;
        virtual MediaPtr media( const std::string& mrl ) const = 0;
        /**
         * @brief addExternalMedia Adds an external media to the list of known media
         * @param mrl This media MRL
         *
         * Once created, this media can be used just like any other media, except
         * it won't have a subType, and won't be analyzed to extract tracks and
         * won't be inserted to any collection (ie. album/show/...)
         * If the mrl is already known to the medialibrary, this function will
         * return nullptr.
         *
         * The media can be fetched using media( std::string ) afterward.
         */
        virtual MediaPtr addExternalMedia( const std::string& mrl ) = 0;
        /**
         * @brief addStream Create an external media of type IMedia::Type::Stream
         *
         * This is equivalent to addExternalMedia, except for the resulting
         * new media's type
         */
        virtual MediaPtr addStream( const std::string& mrl ) = 0;
        virtual Query<IMedia> audioFiles( const QueryParameters* params = nullptr ) const = 0;
        virtual Query<IMedia> videoFiles( const QueryParameters* params = nullptr ) const = 0;
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
         * @param includeAll If true, all artists including those without album
         *                   will be returned. If false, only artists which have
         *                   an album will be returned.
         * @param sort A sorting criteria. So far, this is ignored, and artists are sorted by lexial order
         * @param desc If true, the provided sorting criteria will be reversed.
         */
        virtual Query<IArtist> artists( bool includeAll,
                                        const QueryParameters* params = nullptr ) const = 0;
        /**
         * @brief genres Return the list of music genres
         * @param sort A sorting criteria. So far, this is ignored, and artists are sorted by lexial order
         * @param desc If true, the provided sorting criteria will be reversed.
         */
        virtual Query<IGenre> genres( const QueryParameters* params = nullptr ) const = 0;
        virtual GenrePtr genre( int64_t id ) const = 0;
        /***
         *  Playlists
         */
        virtual PlaylistPtr createPlaylist( const std::string& name ) = 0;
        virtual Query<IPlaylist> playlists( const QueryParameters* params = nullptr ) = 0;
        virtual PlaylistPtr playlist( int64_t id ) const = 0;
        virtual bool deletePlaylist( int64_t playlistId ) = 0;

        /**
         * History
         */
        virtual Query<IMedia> history() const = 0;
        virtual Query<IMedia> streamHistory() const = 0;
        /**
         * @brief clearHistory will clear both streams history & media history.
         * @return true in case of success, false otherwise. The database will stay untouched in case
         *              of failure.
         *
         * This will flush the entity cache, but will not edit any existing instance of a media entity,
         * meaning any instance of media you're holding will outdated fields.
         */
        virtual bool clearHistory() = 0;

        /**
         * Search
         */

        /**
         * @brief searchMedia, searchAudio, and searchVideo search for some media, based on a pattern.
         * @param pattern A 3 character or more pattern that will be matched against the media's title
         *                or filename if no title was set for this media.
         * @param params Some query parameters. Valid sorting criteria are:
         *               - Duration
         *               - InsertionDate
         *               - ReleaseDate
         *               - PlayCount
         *               - Filename
         *               - LastModificationDate
         *               - FileSize
         *              Default sorting parameter uses the media's title.
         *              Passing nullptr will default to default ascending sort
         *
         * Only media that were discovered by the medialibrary will be included.
         * For instance, media that are added explicitely, playlist items that
         * point to remote content, will *not* be included
         */
        virtual Query<IMedia> searchMedia( const std::string& pattern,
                                                   const QueryParameters* params = nullptr ) const = 0;
        virtual Query<IMedia> searchAudio( const std::string& pattern, const QueryParameters* params = nullptr ) const = 0;
        virtual Query<IMedia> searchVideo( const std::string& pattern, const QueryParameters* params = nullptr ) const = 0;

        virtual Query<IPlaylist> searchPlaylists( const std::string& name,
                                                  const QueryParameters* params = nullptr ) const = 0;
        virtual Query<IAlbum> searchAlbums( const std::string& pattern,
                                            const QueryParameters* params = nullptr ) const = 0;
        virtual Query<IGenre> searchGenre( const std::string& genre,
                                           const QueryParameters* params = nullptr ) const = 0;
        virtual Query<IArtist> searchArtists( const std::string& name, bool includeAll,
                                              const QueryParameters* params = nullptr  ) const = 0;
        virtual SearchAggregate search( const std::string& pattern,
                                        const QueryParameters* params = nullptr ) const = 0;

        /**
         * @brief discover Launch a discovery on the provided entry point.
         * The actuall discovery will run asynchronously, meaning this method will immediatly return.
         * Depending on which discoverer modules where provided, this might or might not work
         * \note This must be called after start()
         * @param entryPoint The MRL of the entrypoint to discover.
         */
        virtual void discover( const std::string& entryPoint ) = 0;
        virtual bool setDiscoverNetworkEnabled( bool enable ) = 0;
        virtual Query<IFolder> entryPoints() const = 0;
        virtual FolderPtr folder( const std::string& mrl ) const = 0;
        virtual void removeEntryPoint( const std::string& entryPoint ) = 0;
        /**
         * @brief banFolder will prevent an entry point folder from being discovered.
         * If the folder was already discovered, it will be removed prior to the ban, and all
         * associated media will be discarded.
         * * @note This method is asynchronous and will run after all currently stacked
         * discovery/ban/unban operations have completed.
         */
        virtual void banFolder( const std::string& path ) = 0;
        /**
         * @brief unbanFolder Unban an entrypoint.
         * In case this entry point was indeed previously banned, this will issue a reload of
         * that entry point
         * @param entryPoint The entry point to unban
         * @note This method is asynchronous and will run after all currently stacked
         * discovery/ban/unban operations have completed.
         */
        virtual void unbanFolder( const std::string& entryPoint ) = 0;
        virtual const std::string& thumbnailPath() const = 0;
        virtual void setLogger( ILogger* logger ) = 0;
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
        virtual void reload() = 0;
        virtual void reload( const std::string& entryPoint ) = 0;
        /**
         * @brief forceParserRetry Forces a re-run of all metadata parsers and resets any
         * unterminated file retry count to 0, granting them 3 new tries at being parsed
         */
        virtual bool forceParserRetry() = 0;

        /**
         * @brief setDeviceLister Sets a device lister.
         * This is meant for OSes with complicated/impossible to achieve device listing (due to
         * missing APIs, permissions problems...)
         * @param lister A device lister
         * @return In case of successfull registration, this will return a IDeviceListerCb, which
         * can be used to signal changes related to the devices available. This callback is owned by
         * the medialibrary, and must *NOT* be released by the application.
         * In case of failure, nullptr will be returned.
         *
         * This must be called *before* initialize()
         */
        virtual IDeviceListerCb* setDeviceLister( DeviceListerPtr lister ) = 0;

        /**
         * @brief forceRescan Deletes all entities except Media and Playlist, and
         *                    forces all media to be rescanned.
         *
         * This can be called anytime after the medialibrary has been initialized. * It will make all held instances outdated. Those should be considered
         * as invalid the moment this method returns.
         */
        virtual void forceRescan() = 0;

        /**
         * \brief requestThumbnail Queues a thumbnail generation request for
         * this media, to be run asynchronously.
         * Upon completion (successful or not) IMediaLibraryCb::onMediaThumbnailReady
         * will be called.
         */
        virtual bool requestThumbnail( MediaPtr media ) = 0;

        virtual void addParserService( std::shared_ptr<parser::IParserService> service ) = 0;

        /**
         * @brief addNetworkFileSystemFactory Provides a network filesystem factory implementation
         *
         * This must be called between initialize() and start().
         * A network file system factory must be inserted before enabling network
         * file systems discovery.
         */
        virtual void addNetworkFileSystemFactory( std::shared_ptr<fs::IFileSystemFactory> fsFactory ) = 0;
};

}

extern "C"
{
    medialibrary::IMediaLibrary* NewMediaLibrary();
}

#endif // IMEDIALIBRARY_H
