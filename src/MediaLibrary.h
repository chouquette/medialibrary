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

class MediaLibrary : public IMediaLibrary
{
public:
    MediaLibrary();
    virtual ~MediaLibrary();
    virtual InitializeResult initialize( const std::string& dbPath,
                                         const std::string& thumbnailPath,
                                         IMediaLibraryCb* mlCallback ) override;
    virtual bool isInitialized() const override;
    virtual StartResult start() override;
    virtual bool isStarted() const override;
    virtual void setVerbosity( LogLevel v ) override;

    virtual MediaPtr media( int64_t mediaId ) const override;
    virtual MediaPtr media( const std::string& mrl ) const override;
    virtual MediaPtr addExternalMedia( const std::string& mrl ) override;
    virtual MediaPtr addStream( const std::string& mrl ) override;
    virtual bool removeExternalMedia( MediaPtr media ) override;
    virtual Query<IMedia> audioFiles( const QueryParameters* params ) const override;
    virtual Query<IMedia> videoFiles( const QueryParameters* params ) const override;

    virtual MediaGroupPtr createMediaGroup( std::string name ) override;
    virtual MediaGroupPtr createMediaGroup( const std::vector<int64_t>& mediaIds ) override;
    virtual bool deleteMediaGroup( int64_t id ) override;
    virtual MediaGroupPtr mediaGroup( int64_t id ) const override;
    virtual Query<IMediaGroup> mediaGroups( const QueryParameters* params ) const override;
    virtual Query<IMediaGroup> searchMediaGroups( const std::string& pattern,
                                                  const QueryParameters* params ) const override;
    virtual bool regroupAll() override;

    virtual void onDiscoveredFile(std::shared_ptr<fs::IFile> fileFs,
                                   std::shared_ptr<Folder> parentFolder,
                                   std::shared_ptr<fs::IDirectory> parentFolderFs,
                                   IFile::Type fileType , std::pair<int64_t, int64_t> parentPlaylist);
    void onUpdatedFile( std::shared_ptr<File> file, std::shared_ptr<fs::IFile> fileFs,
                        std::shared_ptr<Folder> parentFolder,
                        std::shared_ptr<fs::IDirectory> parentFolderFs );

    bool deleteFolder(const Folder& folder );

    virtual LabelPtr createLabel( const std::string& label ) override;
    virtual bool deleteLabel( LabelPtr label ) override;

    virtual AlbumPtr album( int64_t id ) const override;
    std::shared_ptr<Album> createAlbum( const std::string& title );
    virtual Query<IAlbum> albums( const QueryParameters* params ) const override;

    virtual Query<IGenre> genres( const QueryParameters* params ) const override;
    virtual GenrePtr genre( int64_t id ) const override;

    virtual ShowPtr show( int64_t id ) const override;
    std::shared_ptr<Show> createShow( const std::string& name );
    virtual Query<IShow> shows( const QueryParameters* params = nullptr ) const override;

    virtual MoviePtr movie( int64_t id ) const override;
    std::shared_ptr<Movie> createMovie( Media& media );

    virtual ArtistPtr artist( int64_t id ) const override;
    std::shared_ptr<Artist> createArtist( const std::string& name );
    virtual Query<IArtist> artists( ArtistIncluded included,
                                    const QueryParameters* params ) const override;

    virtual PlaylistPtr createPlaylist( const std::string& name ) override;
    virtual Query<IPlaylist> playlists( const QueryParameters* params ) override;
    virtual PlaylistPtr playlist( int64_t id ) const override;
    virtual bool deletePlaylist( int64_t playlistId ) override;

    virtual Query<IMedia> history() const override;
    virtual Query<IMedia> history( IMedia::Type type ) const override;
    virtual Query<IMedia> streamHistory() const override;
    virtual bool clearHistory() override;

    virtual Query<IMedia> searchMedia( const std::string& title,
                                       const QueryParameters* params ) const override;
    virtual Query<IMedia> searchAudio( const std::string& pattern,
                                       const QueryParameters* params = nullptr ) const override;
    virtual Query<IMedia> searchVideo( const std::string& pattern,
                                       const QueryParameters* params = nullptr ) const override;
    virtual Query<IPlaylist> searchPlaylists( const std::string& name,
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
    virtual Query<IFolder> entryPoints() const override;
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
    virtual void setLogger( ILogger* logger ) override;
    //Temporarily public, move back to private as soon as we start monitoring the FS
    virtual void reload() override;
    virtual void reload( const std::string& entryPoint ) override;
    virtual bool forceParserRetry() override;
    virtual void clearDatabase( bool restorePlaylists ) override;

    virtual void pauseBackgroundOperations() override;
    virtual void resumeBackgroundOperations() override;
    void onDiscovererIdleChanged( bool idle );
    void onParserIdleChanged( bool idle );

    sqlite::Connection* getConn() const;
    IMediaLibraryCb* getCb() const;
    std::shared_ptr<ModificationNotifier> getNotifier() const;
    /**
     * @brief getParser Returns a parser instance
     *
     * This will instantiate and start the parser if needed. It may still return
     * a nullptr instance, in case no service are generated, or in some tests
     * configurations
     */
    parser::Parser* getParser() const;
    /**
     * @brief tryGetParser Returns a parser instance if it exists.
     *
     * If no parser has been created, nullptr will be returned
     */
    parser::Parser* tryGetParser();
    ThumbnailerWorker* thumbnailer() const;

    virtual void registerDeviceLister( DeviceListerPtr lister,
                                       const std::string& scheme ) override;
    virtual DeviceListerPtr deviceLister( const std::string& scheme ) const override;
    DeviceListerPtr deviceListerLocked( const std::string& scheme ) const;

    std::shared_ptr<fs::IFileSystemFactory> fsFactoryForMrl( const std::string& mrl ) const;

    /**
     * @brief refreshDevices Refreshes the devices from a specific FS factory
     * @param fsFactory The file system factory for which devices must be refreshed
     *
     * This is expected to be used when a specific factory signals that a device
     * was plugged/unplugged.
     */
    void refreshDevices(fs::IFileSystemFactory& fsFactory);
    /**
     * @brief startFsFactoriesAndRefresh Starts fs factories & refreshes all known devices
     *
     * This will start all provided & required file system factories (ie. local
     * ones, and network ones if network discovery is enabled), and refresh the
     * presence & last seen date for all known devices we have in database.
     * This operation must not be based on the available FsFactories, as we might
     * not have a factory that was used to create a device before.
     * We still need to mark all the associated devices as missing.
     */
    void startFsFactoriesAndRefresh();

    virtual bool forceRescan() override;

    virtual void enableFailedThumbnailRegeneration() override;

    virtual void addParserService( std::shared_ptr<parser::IParserService> service ) override;

    virtual void addThumbnailer( std::shared_ptr<IThumbnailer> thumbnailer ) override;

    virtual bool addFileSystemFactory( std::shared_ptr<fs::IFileSystemFactory> fsFactory ) override;
    bool addFileSystemFactoryLocked( std::shared_ptr<fs::IFileSystemFactory> fsFactory );

    static void removeOldEntities( MediaLibraryPtr ml );

    virtual const std::vector<const char*>& supportedMediaExtensions() const override;
    virtual const std::vector<const char*>& supportedPlaylistExtensions() const override;
    virtual bool isMediaExtensionSupported( const char* ext ) const override;
    virtual bool isPlaylistExtensionSupported( const char* ext ) const override;

protected:
    // Allow access to unit test MediaLibrary implementations
    static const std::vector<const char*> SupportedMediaExtensions;
    static const std::vector<const char*> SupportedPlaylistExtensions;

protected:
    virtual void startParser();
    virtual void startDiscoverer();
    virtual void startDeletionNotifier();
    virtual void populateNetworkFsFactories();

private:
    bool recreateDatabase();
    InitializeResult updateDatabaseModel( unsigned int previousVersion );
    void migrateModel3to5();
    void migrateModel5to6();
    void migrateModel7to8();
    bool migrateModel8to9();
    void migrateModel9to10();
    void migrateModel10to11();
    bool migrateModel12to13();
    void migrateModel13to14( uint32_t originalPreviousVersion );
    void migrateModel14to15();
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
    /**
     * Runs some migration steps that depend on the actual C++ code, and that
     * therefor require the migration to have already completed
     */
    void migrationEpilogue( uint32_t originalPreviousVersion );
    bool createAllTables( uint32_t dbModelVersion );
    void createAllTriggers( uint32_t dbModelVersion );
    bool checkDatabaseIntegrity();
    void registerEntityHooks();
    static bool validateSearchPattern( const std::string& pattern );
    void removeThumbnails();
    void refreshDevice( Device& device, fs::IFileSystemFactory* fsFactory );
    void startThumbnailer() const;
    parser::Parser* getParserLocked() const;
    virtual bool forceRescanLocked();

protected:
    virtual void addLocalFsFactory();
    void addDefaultDeviceListers();

    class FsFactoryCb : public fs::IFileSystemFactoryCb
    {
    public:
        explicit FsFactoryCb( MediaLibrary* ml );
    private:
        virtual bool onDeviceMounted( const fs::IDevice& deviceFs ) override;
        virtual void onDeviceUnmounted( const fs::IDevice& deviceFs ) override;
    private:
        MediaLibrary* m_ml;
    };

protected:
    mutable compat::Mutex m_mutex;
    std::shared_ptr<sqlite::Connection> m_dbConnection;

    LogLevel m_verbosity;
    Settings m_settings;
    bool m_initialized;
    bool m_started;
    bool m_networkDiscoveryEnabled;
    std::atomic_bool m_discovererIdle;
    std::atomic_bool m_parserIdle;

    /* All fs factory callbacks must outlive the fs factory itself, since
     * it might invoke some of the callback interface methods during teardown
     */
    FsFactoryCb m_fsFactoryCb;

    std::string m_thumbnailPath;
    std::string m_playlistPath;
    IMediaLibraryCb* m_callback;

    // External device lister
    std::unordered_map<std::string, DeviceListerPtr> m_deviceListers;
    std::vector<std::shared_ptr<fs::IFileSystemFactory>> m_fsFactories;

    // User provided parser services
    std::vector<std::shared_ptr<parser::IParserService>> m_services;
    mutable std::shared_ptr<IThumbnailer> m_thumbnailer;
    // Keep the parser as last field.
    // The parser holds a (raw) pointer to the media library. When MediaLibrary's destructor gets called
    // it might still finish a few operations before exiting the parser thread. Those operations are
    // likely to require a valid MediaLibrary, which would be compromised if some fields have already been
    // deleted/destroyed.
    mutable std::unique_ptr<parser::Parser> m_parser;
    // Same reasoning applies here.
    //FIXME: Having to maintain a specific ordering sucks, let's use shared_ptr or something
    std::unique_ptr<DiscovererWorker> m_discovererWorker;
    std::shared_ptr<ModificationNotifier> m_modificationNotifier;
    mutable std::unique_ptr<ThumbnailerWorker> m_thumbnailerWorker;
};

}

#endif // MEDIALIBRARY_H
