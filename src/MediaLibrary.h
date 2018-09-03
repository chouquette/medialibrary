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

#ifndef MEDIALIBRARY_H
#define MEDIALIBRARY_H

#include "medialibrary/IMediaLibrary.h"
#include "Settings.h"

#include "medialibrary/IDeviceLister.h"
#include "medialibrary/IMedia.h"

#include <atomic>

namespace medialibrary
{

class ModificationNotifier;
class DiscovererWorker;
class VLCThumbnailer;

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
class IFileSystemFactory;
}

namespace parser
{
class Parser;
class Worker;
}

class MediaLibrary : public IMediaLibrary, public IDeviceListerCb
{
    public:
        MediaLibrary();
        ~MediaLibrary();
        virtual InitializeResult initialize( const std::string& dbPath,
                                             const std::string& thumbnailPath,
                                             IMediaLibraryCb* mlCallback ) override;
        virtual bool start() override;
        virtual void setVerbosity( LogLevel v ) override;

        virtual MediaPtr media( int64_t mediaId ) const override;
        virtual MediaPtr media( const std::string& mrl ) const override;
        virtual MediaPtr addExternalMedia( const std::string& mrl ) override;
        virtual MediaPtr addStream( const std::string& mrl ) override;
        virtual Query<IMedia> audioFiles( const QueryParameters* params ) const override;
        virtual Query<IMedia> videoFiles( const QueryParameters* params ) const override;

        virtual void onDiscoveredFile( std::shared_ptr<fs::IFile> fileFs,
                                        std::shared_ptr<Folder> parentFolder,
                                        std::shared_ptr<fs::IDirectory> parentFolderFs,
                                        std::pair<std::shared_ptr<Playlist>, unsigned int> parentPlaylist );
        void onUpdatedFile( std::shared_ptr<File> file,
                            std::shared_ptr<fs::IFile> fileFs );

        bool deleteFolder(const Folder& folder );

        virtual LabelPtr createLabel( const std::string& label ) override;
        virtual bool deleteLabel( LabelPtr label ) override;

        virtual AlbumPtr album( int64_t id ) const override;
        std::shared_ptr<Album> createAlbum( const std::string& title, int64_t thumbnailId );
        virtual Query<IAlbum> albums( const QueryParameters* params ) const override;

        virtual Query<IGenre> genres( const QueryParameters* params ) const override;
        virtual GenrePtr genre( int64_t id ) const override;

        virtual ShowPtr show( int64_t id ) const override;
        std::shared_ptr<Show> createShow( const std::string& name );
        virtual Query<IShow> shows( const QueryParameters* params = nullptr ) const override;

        virtual MoviePtr movie( int64_t id ) const override;
        std::shared_ptr<Movie> createMovie( Media& media );

        virtual ArtistPtr artist( int64_t id ) const override;
        ArtistPtr artist( const std::string& name );
        std::shared_ptr<Artist> createArtist( const std::string& name );
        virtual Query<IArtist> artists( bool includeAll,
                                        const QueryParameters* params ) const override;

        virtual PlaylistPtr createPlaylist( const std::string& name ) override;
        virtual Query<IPlaylist> playlists( const QueryParameters* params ) override;
        virtual PlaylistPtr playlist( int64_t id ) const override;
        virtual bool deletePlaylist( int64_t playlistId ) override;

        virtual Query<IMedia> history() const override;
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
        virtual Query<IArtist> searchArtists( const std::string& name, bool includeAll,
                                              const QueryParameters* params ) const override;
        virtual Query<IShow> searchShows( const std::string& pattern,
                                          const QueryParameters* params = nullptr ) const override;
        virtual SearchAggregate search( const std::string& pattern,
                                        const QueryParameters* params ) const override;

        virtual void discover( const std::string& entryPoint ) override;
        virtual bool setDiscoverNetworkEnabled( bool enabled ) override;
        virtual Query<IFolder> entryPoints() const override;
        virtual FolderPtr folder( const std::string& mrl ) const override;
        virtual void removeEntryPoint( const std::string& entryPoint ) override;
        virtual void banFolder( const std::string& path ) override;
        virtual void unbanFolder( const std::string& path ) override;

        virtual const std::string& thumbnailPath() const override;
        virtual void setLogger( ILogger* logger ) override;
        //Temporarily public, move back to private as soon as we start monitoring the FS
        virtual void reload() override;
        virtual void reload( const std::string& entryPoint ) override;
        virtual bool forceParserRetry() override;

        virtual void pauseBackgroundOperations() override;
        virtual void resumeBackgroundOperations() override;
        void onDiscovererIdleChanged( bool idle );
        void onParserIdleChanged( bool idle );

        sqlite::Connection* getConn() const;
        IMediaLibraryCb* getCb() const;
        std::shared_ptr<ModificationNotifier> getNotifier() const;

        virtual IDeviceListerCb* setDeviceLister( DeviceListerPtr lister ) override;
        std::shared_ptr<fs::IFileSystemFactory> fsFactoryForMrl( const std::string& path ) const;

        void refreshDevices(fs::IFileSystemFactory& fsFactory);

        virtual void forceRescan() override;

        virtual bool requestThumbnail( MediaPtr media ) override;

        virtual void addParserService( std::shared_ptr<parser::IParserService> service ) override;

        virtual void addNetworkFileSystemFactory( std::shared_ptr<fs::IFileSystemFactory> fsFactory ) override;

        static bool isExtensionSupported( const char* ext );

    protected:
        // Allow access to unit test MediaLibrary implementations
        static const char* const supportedExtensions[];
        static const size_t NbSupportedExtensions;

    protected:
        virtual bool startParser();
        virtual void startDiscoverer();
        virtual void startDeletionNotifier();
        virtual void startThumbnailer();
        virtual void populateFsFactories();

    private:
        bool recreateDatabase( const std::string& dbPath );
        InitializeResult updateDatabaseModel( unsigned int previousVersion,
                                              const std::string& path );
        void migrateModel3to5();
        void migrateModel5to6();
        void migrateModel7to8();
        void migrateModel8to9();
        void migrateModel9to10();
        void migrateModel10to11();
        void migrateModel12to13();
        void migrateModel13to14( uint32_t originalPreviousVersion );
        void createAllTables();
        void createAllTriggers();
        void registerEntityHooks();
        static bool validateSearchPattern( const std::string& pattern );
        // Returns true if the device actually changed
        bool onDeviceChanged( fs::IFileSystemFactory& fsFactory, Device& device );
        bool createThumbnailFolder( const std::string& thumbnailPath ) const;

    protected:
        virtual void addLocalFsFactory();
        MediaPtr addExternalMedia( const std::string& mrl, IMedia::Type type );

        // Mark IDeviceListerCb callbacks as private. They must be invoked through the interface.
    private:
        virtual bool onDevicePlugged( const std::string& uuid, const std::string& mountpoint ) override;
        virtual void onDeviceUnplugged(const std::string& uuid) override;
        virtual bool isDeviceKnown( const std::string& uuid ) const override;
        void clearCache();

    protected:
        std::shared_ptr<sqlite::Connection> m_dbConnection;
        std::vector<std::shared_ptr<fs::IFileSystemFactory>> m_fsFactories;
        std::vector<std::shared_ptr<fs::IFileSystemFactory>> m_externalFsFactories;
        std::string m_thumbnailPath;
        IMediaLibraryCb* m_callback;
        DeviceListerPtr m_deviceLister;

        // User provided parser services
        std::vector<std::shared_ptr<parser::IParserService>> m_services;
        // Keep the parser as last field.
        // The parser holds a (raw) pointer to the media library. When MediaLibrary's destructor gets called
        // it might still finish a few operations before exiting the parser thread. Those operations are
        // likely to require a valid MediaLibrary, which would be compromised if some fields have already been
        // deleted/destroyed.
        std::unique_ptr<parser::Parser> m_parser;
        // Same reasoning applies here.
        //FIXME: Having to maintain a specific ordering sucks, let's use shared_ptr or something
        std::unique_ptr<DiscovererWorker> m_discovererWorker;
        std::shared_ptr<ModificationNotifier> m_modificationNotifier;
        LogLevel m_verbosity;
        Settings m_settings;
        bool m_initialized;
        std::atomic_bool m_discovererIdle;
        std::atomic_bool m_parserIdle;
#ifdef HAVE_LIBVLC
        std::unique_ptr<VLCThumbnailer> m_thumbnailer;
#endif
};

}

#endif // MEDIALIBRARY_H
