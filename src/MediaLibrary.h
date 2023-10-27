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

#ifndef MEDIALIBRARY_H
#define MEDIALIBRARY_H

#include "medialibrary/IMediaLibrary.h"
#include "Settings.h"

#include "medialibrary/IDeviceLister.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "medialibrary/IMedia.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"
#include "LockFile.h"
#include "database/SqliteConnection.h"
#include "filesystem/FsHolder.h"
#include "discoverer/DiscovererWorker.h"
#include "parser/Parser.h"
#include "CacheWorker.h"

#include <atomic>

namespace medialibrary
{

class ModificationNotifier;
class DiscovererWorker;
class ThumbnailerWorker;

class Album;
class Artist;
class Media;
class Movie;
class Show;
class Device;
class Folder;
class Genre;
class Playlist;
class File;
class ILogger;

namespace fs
{
class IFile;
class IDirectory;
}

namespace parser
{
class Parser;
class Worker;
}

class PriorityAccessImpl
{
public:
    PriorityAccessImpl( sqlite::Connection::PriorityContext priorityContext )
        : m_priorityContext( std::move( priorityContext ) )
    {
    }

private:
    sqlite::Connection::PriorityContext m_priorityContext;
};

class MediaLibrary : public IMediaLibrary
{
public:
    static std::unique_ptr<MediaLibrary> create( const std::string &dbPath,
                                                 const std::string &mlFolderPath,
                                                 bool lockFile, const SetupConfig* cfg );

    MediaLibrary( const std::string& dbPath, const std::string& mlFolderPath,
                  std::unique_ptr<LockFile> lockFile = {}, const SetupConfig* cfg = nullptr );

    virtual ~MediaLibrary();
    virtual InitializeResult initialize( IMediaLibraryCb* mlCallback ) override;
    virtual void setVerbosity( LogLevel v ) override;

    virtual MediaPtr media( int64_t mediaId ) const override;
    virtual MediaPtr media( const std::string& mrl ) const override;
    virtual MediaPtr addExternalMedia( const std::string& mrl, int64_t duration ) override;
    virtual MediaPtr addStream( const std::string& mrl ) override;
    virtual bool removeExternalMedia( MediaPtr media ) override;
    virtual Query<IMedia> audioFiles( const QueryParameters* params ) const override;
    virtual Query<IMedia> videoFiles( const QueryParameters* params ) const override;
    virtual Query<IMedia> movies( const QueryParameters* params ) const override;
    virtual Query<IMedia> subscriptionMedia( const QueryParameters* params ) const override;
    virtual Query<IMedia> inProgressMedia( IMedia::Type type,
                                           const QueryParameters* params ) const override;

    virtual MediaGroupPtr createMediaGroup( std::string name ) override;
    virtual MediaGroupPtr createMediaGroup( const std::vector<int64_t>& mediaIds ) override;
    virtual bool deleteMediaGroup( int64_t id ) override;
    virtual MediaGroupPtr mediaGroup( int64_t id ) const override;
    virtual Query<IMediaGroup> mediaGroups( IMedia::Type mediaType,
                                            const QueryParameters* params ) const override;
    virtual Query<IMediaGroup> searchMediaGroups( const std::string& pattern,
                                                  const QueryParameters* params ) const override;
    virtual bool regroupAll() override;

    virtual void onDiscoveredFile( std::shared_ptr<fs::IFile> fileFs,
                                   std::shared_ptr<Folder> parentFolder,
                                   std::shared_ptr<fs::IDirectory> parentFolderFs,
                                   IFile::Type fileType );
    void onDiscoveredLinkedFile( const fs::IFile& fileFs, IFile::Type fileType );
    void onUpdatedFile( std::shared_ptr<File> file, std::shared_ptr<fs::IFile> fileFs,
                        std::shared_ptr<Folder> parentFolder,
                        std::shared_ptr<fs::IDirectory> parentFolderFs );

    virtual LabelPtr createLabel( const std::string& label ) override;
    virtual bool deleteLabel( LabelPtr label ) override;

    virtual AlbumPtr album( int64_t id ) const override;
    std::shared_ptr<Album> createAlbum( std::string title );
    virtual Query<IAlbum> albums( const QueryParameters* params ) const override;

    virtual Query<IGenre> genres( const QueryParameters* params ) const override;
    virtual GenrePtr genre( int64_t id ) const override;

    virtual ShowPtr show( int64_t id ) const override;
    std::shared_ptr<Show> createShow( std::string name );
    virtual Query<IShow> shows( const QueryParameters* params = nullptr ) const override;

    virtual MoviePtr movie( int64_t id ) const override;
    std::shared_ptr<Movie> createMovie( Media& media );

    virtual ArtistPtr artist( int64_t id ) const override;
    std::shared_ptr<Artist> createArtist( std::string name );
    virtual Query<IArtist> artists( ArtistIncluded included,
                                    const QueryParameters* params ) const override;

    virtual PlaylistPtr createPlaylist( std::string name ) override;
    virtual Query<IPlaylist> playlists( PlaylistType type,
                                        const QueryParameters* params ) override;
    virtual PlaylistPtr playlist( int64_t id ) const override;
    virtual bool deletePlaylist( int64_t playlistId ) override;

    virtual Query<IMedia> history( HistoryType,
                                   const QueryParameters* params = nullptr ) const override;
    virtual Query<IMedia> audioHistory( const QueryParameters* params = nullptr ) const override;
    virtual Query<IMedia> videoHistory( const QueryParameters* params = nullptr ) const override;
    virtual bool clearHistory( HistoryType ) override;

    virtual Query<IMedia> searchMedia( const std::string& title,
                                       const QueryParameters* params ) const override;
    virtual Query<IMedia> searchAudio( const std::string& pattern,
                                       const QueryParameters* params = nullptr ) const override;
    virtual Query<IMedia> searchVideo( const std::string& pattern,
                                       const QueryParameters* params = nullptr ) const override;
    virtual Query<IMedia> searchMovie( const std::string& pattern,
                                       const QueryParameters* params = nullptr ) const override;
    virtual Query<IMedia>
    searchSubscriptionMedia( const std::string& pattern,
                             const QueryParameters* params = nullptr ) const override;
    virtual Query<IPlaylist> searchPlaylists( const std::string& name, PlaylistType type,
                                              const QueryParameters* params ) const override;
    virtual Query<IAlbum> searchAlbums( const std::string& pattern,
                                        const QueryParameters* params ) const override;
    virtual Query<IGenre> searchGenre( const std::string& genre,
                                       const QueryParameters* params ) const override;
    virtual Query<IArtist> searchArtists( const std::string& name, ArtistIncluded includeAll,
                                          const QueryParameters* params ) const override;
    virtual Query<IShow> searchShows( const std::string& pattern,
                                      const QueryParameters* params = nullptr ) const override;
    virtual SearchAggregate search( const std::string& pattern,
                                    const QueryParameters* params ) const override;

    virtual void discover( const std::string& entryPoint ) override;
    virtual bool setDiscoverNetworkEnabled( bool enabled ) override;
    virtual bool isDiscoverNetworkEnabled() const override;
    virtual Query<IFolder> roots( const QueryParameters* params ) const override;
    virtual bool isIndexed( const std::string& mrl ) const override;
    virtual bool isBanned( const std::string& mrl ) const override;
    virtual Query<IFolder> folders( IMedia::Type type,
                                    const QueryParameters* params = nullptr ) const override;
    virtual Query<IFolder> searchFolders( const std::string& pattern,
                                          IMedia::Type type,
                                          const QueryParameters* params ) const override;
    virtual FolderPtr folder( int64_t id ) const override;
    virtual FolderPtr folder( const std::string& mrl ) const override;
    virtual void removeEntryPoint( const std::string& entryPoint ) override;
    virtual void banFolder( const std::string& entryPoint ) override;
    virtual void unbanFolder( const std::string& entryPoint ) override;
    virtual Query<IFolder> bannedEntryPoints() const override;

    const std::string& thumbnailPath() const;
    const std::string& playlistPath() const;
    /**
     * @brief cachePath Returns the path to the medialib cache folder
     */
    const std::string& cachePath();

    //Temporarily public, move back to private as soon as we start monitoring the FS
    virtual void reload() override;
    virtual void reload( const std::string& entryPoint ) override;
    virtual bool forceParserRetry() override;
    virtual bool clearDatabase( bool restorePlaylists ) override;

    virtual void pauseBackgroundOperations() override;
    virtual void resumeBackgroundOperations() override;
    void onDiscovererIdleChanged( bool idle );
    void onParserIdleChanged( bool idle );

    sqlite::Connection* getConn() const;
    IMediaLibraryCb* getCb() const;
    std::shared_ptr<ModificationNotifier> getNotifier() const;
    virtual parser::Parser* getParser() const;
    ThumbnailerWorker* thumbnailer() const;
    CacheWorker& cacheWorker();

    virtual DeviceListerPtr deviceLister( const std::string& scheme ) const override;

    std::shared_ptr<fs::IFileSystemFactory> fsFactoryForMrl( const std::string& mrl ) const;

    void startFsFactory( fs::IFileSystemFactory& fsFactory ) const;

    const Settings& settings() const;

    virtual bool forceRescan() override;

    virtual void enableFailedThumbnailRegeneration() override;

    virtual void addThumbnailer( std::shared_ptr<IThumbnailer> thumbnailer ) override;

    static void removeOldEntities( MediaLibraryPtr ml );

    virtual const std::vector<const char*>& supportedMediaExtensions() const override;
    virtual const std::vector<const char*>& supportedPlaylistExtensions() const override;
    virtual bool isMediaExtensionSupported( const char* ext ) const override;
    virtual bool isPlaylistExtensionSupported( const char* ext ) const override;
    virtual const std::vector<const char*>& supportedSubtitleExtensions() const override;
    virtual bool isSubtitleExtensionSupported( const char* ext ) const override;

    virtual bool isDeviceKnown( const std::string& uuid,
                                const std::string& mountpoint,
                                bool isRemovable ) override;

    virtual bool deleteRemovableDevices() override;

    virtual bool requestThumbnail( int64_t mediaId, ThumbnailSizeType sizeType,
                                   uint32_t desiredWidth, uint32_t desiredHeight,
                                   float position ) override;

    virtual BookmarkPtr bookmark( int64_t bookmarkId ) const override;

    virtual bool setExternalLibvlcInstance( libvlc_instance_t* inst ) override;

    virtual PriorityAccess acquirePriorityAccess() override;

    virtual bool flushUserProvidedThumbnails() override;

    virtual SubscriptionPtr subscription( int64_t id ) const override;
    virtual bool removeSubscription( int64_t subscriptionId ) override;

    virtual bool fitsInSubscriptionCache( const IMedia& m ) const override;

    virtual void cacheNewSubscriptionMedia() override;

    virtual ServicePtr service( IService::Type type ) const override;
    virtual SubscriptionPtr subscription(uint64_t id) const override;

    virtual bool setSubscriptionMaxCachedMedia( uint32_t nbCachedMedia ) override;
    virtual bool setSubscriptionMaxCacheSize( uint64_t maxCacheSize ) override;
    virtual bool setMaxCacheSize( uint64_t maxCacheSize ) override;

    virtual uint32_t getSubscriptionMaxCachedMedia() const override;
    virtual uint64_t getSubscriptionMaxCacheSize() const override;
    virtual uint64_t getMaxCacheSize() const override;
    virtual bool refreshAllSubscriptions() override;

private:
    static const std::vector<const char*> SupportedMediaExtensions;
    static const std::vector<const char*> SupportedPlaylistExtensions;
    static const std::vector<const char*> SupportedSubtitleExtensions;

protected:
    virtual void startDeletionNotifier();
    virtual void populateNetworkFsFactories();
    /*
     * This allows tests to execute code once the database connection is ready
     * and before all tables are created
     */
    virtual void onDbConnectionReady( sqlite::Connection* dbConn );
    void onBackgroundTasksIdleChanged( bool idle );
    /**
     * @brief stopBackgroundJobs Explicitely stop all background jobs
     *
     * This is intended to be used when tearing down the media library to ensure
     * the background jobs don't use fields that would be destroyed before the
     * worker instances, and from tests to avoid a 'data race on vptr' report
     * from TSAN
     */
    void stopBackgroundJobs();

private:
    bool recreateDatabase();
    InitializeResult updateDatabaseModel( unsigned int previousVersion );
    void migrateModel15to16();
    void migrateModel16to17();
    void migrateModel17to18(uint32_t originalPreviousVersion);
    bool migrateModel18to19();
    void migrateModel19to20();
    void migrateModel20to21();
    void migrateModel21to22();
    void migrateModel22to23();
    void migrateModel23to24();
    void migrateModel24to25();
    void migrateModel25to26();
    void migrateModel26to27();
    void migrateModel27to28();
    void migrateModel28to29();
    void migrateModel29to30();
    void migrateModel30to31();
    void migrateModel31to32();
    void migrateModel32to33();
    void migrateModel33to34();
    void migrateModel34to35();
    void migrateModel35to36();
    void migrateModel36to37();
    /**
     * Runs some migration steps that depend on the actual C++ code, and that
     * therefor require the migration to have already completed
     */
    void migrationEpilogue( uint32_t originalPreviousVersion );
    bool createAllTables();
    void createAllTriggers();
    bool checkDatabaseIntegrity();
    void registerEntityHooks();
    void removeThumbnails();
    void startThumbnailer() const;

protected:
    virtual void addLocalFsFactory();
    void deleteAllTables( medialibrary::sqlite::Connection *dbConn );
    void waitForBackgroundTasksIdle();

protected:
    mutable compat::Mutex m_mutex;
    std::shared_ptr<sqlite::Connection> m_dbConnection;

    Settings m_settings;
    bool m_initialized;
    bool m_discovererIdle;
    bool m_parserIdle;
    compat::ConditionVariable m_idleCond;

    const std::string m_dbPath;
    const std::string m_mlFolderPath;
    const std::string m_thumbnailPath;
    const std::string m_playlistPath;
    const std::string m_cachePath;

    std::unique_ptr<LockFile> m_lockFile;

    IMediaLibraryCb* m_callback;

    FsHolder m_fsHolder;

    mutable std::shared_ptr<IThumbnailer> m_thumbnailer;
    // Keep the parser as last field.
    // The parser holds a (raw) pointer to the media library. When MediaLibrary's destructor gets called
    // it might still finish a few operations before exiting the parser thread. Those operations are
    // likely to require a valid MediaLibrary, which would be compromised if some fields have already been
    // deleted/destroyed.
    parser::Parser m_parser;
    // Same reasoning applies here.
    //FIXME: Having to maintain a specific ordering sucks, let's use shared_ptr or something
    DiscovererWorker m_discovererWorker;
    std::shared_ptr<ModificationNotifier> m_modificationNotifier;
    mutable compat::Mutex m_thumbnailerWorkerMutex;
    mutable std::unique_ptr<ThumbnailerWorker> m_thumbnailerWorker;
    CacheWorker m_cacheWorker;
};

}

#endif // MEDIALIBRARY_H
