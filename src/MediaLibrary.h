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

class ModificationNotifier;
class DiscovererWorker;
class Parser;
class ParserService;
class SqliteConnection;

#include "medialibrary/IMediaLibrary.h"
#include "logging/Logger.h"
#include "Settings.h"

class Album;
class Artist;
class Media;
class Movie;
class Show;
class Device;
class Folder;
class Genre;

namespace factory
{
class IFileSystem;
}
namespace fs
{
class IFile;
class IDirectory;
}

class MediaLibrary : public medialibrary::IMediaLibrary
{
    public:
        MediaLibrary();
        ~MediaLibrary();
        virtual bool initialize( const std::string& dbPath, const std::string& thumbnailPath, medialibrary::IMediaLibraryCb* metadataCb ) override;
        virtual void setVerbosity( medialibrary::LogLevel v ) override;

        virtual std::vector<medialibrary::MediaPtr> audioFiles( medialibrary::SortingCriteria sort, bool desc) const override;
        virtual std::vector<medialibrary::MediaPtr> videoFiles( medialibrary::SortingCriteria sort, bool desc) const override;

        std::shared_ptr<Media> addFile( const fs::IFile& fileFs, Folder& parentFolder, fs::IDirectory& parentFolderFs );

        bool deleteFolder(const Folder& folder );
        std::shared_ptr<Device> device( const std::string& uuid );

        virtual medialibrary::LabelPtr createLabel( const std::string& label ) override;
        virtual bool deleteLabel( medialibrary::LabelPtr label ) override;

        virtual medialibrary::AlbumPtr album( int64_t id ) const override;
        std::shared_ptr<Album> createAlbum( const std::string& title );
        virtual std::vector<medialibrary::AlbumPtr> albums(medialibrary::SortingCriteria sort, bool desc) const override;

        virtual std::vector<medialibrary::GenrePtr> genres( medialibrary::SortingCriteria sort, bool desc ) const override;
        virtual medialibrary::GenrePtr genre( int64_t id ) const override;

        virtual medialibrary::ShowPtr show( const std::string& name ) const override;
        std::shared_ptr<Show> createShow( const std::string& name );

        virtual medialibrary::MoviePtr movie( const std::string& title ) const override;
        std::shared_ptr<Movie> createMovie( Media& media, const std::string& title );

        virtual medialibrary::ArtistPtr artist( int64_t id ) const override;
        medialibrary::ArtistPtr artist( const std::string& name );
        std::shared_ptr<Artist> createArtist( const std::string& name );
        virtual std::vector<medialibrary::ArtistPtr> artists( medialibrary::SortingCriteria sort, bool desc ) const override;

        virtual medialibrary::PlaylistPtr createPlaylist( const std::string& name ) override;
        virtual std::vector<medialibrary::PlaylistPtr> playlists( medialibrary::SortingCriteria sort, bool desc ) override;
        virtual medialibrary::PlaylistPtr playlist( int64_t id ) const override;
        virtual bool deletePlaylist( int64_t playlistId ) override;

        virtual bool addToHistory( const std::string& mrl );
        virtual std::vector<medialibrary::HistoryPtr> lastStreamsPlayed() const override;
        virtual std::vector<medialibrary::MediaPtr> lastMediaPlayed() const override;

        virtual medialibrary::MediaSearchAggregate searchMedia( const std::string& title ) const override;
        virtual std::vector<medialibrary::PlaylistPtr> searchPlaylists( const std::string& name ) const override;
        virtual std::vector<medialibrary::AlbumPtr> searchAlbums( const std::string& pattern ) const override;
        virtual std::vector<medialibrary::GenrePtr> searchGenre( const std::string& genre ) const override;
        virtual std::vector<medialibrary::ArtistPtr> searchArtists( const std::string& name ) const override;
        virtual medialibrary::SearchAggregate search( const std::string& pattern ) const override;

        virtual void discover( const std::string& entryPoint ) override;
        virtual bool banFolder( const std::string& path ) override;
        virtual bool unbanFolder( const std::string& path ) override;

        virtual const std::string& thumbnailPath() const override;
        virtual void setLogger( medialibrary::ILogger* logger ) override;
        //Temporarily public, move back to private as soon as we start monitoring the FS
        virtual void reload() override;
        virtual void reload( const std::string& entryPoint ) override;

        virtual void pauseBackgroundOperations() override;
        virtual void resumeBackgroundOperations() override;

        DBConnection getConn() const;
        medialibrary::IMediaLibraryCb* getCb() const;
        std::shared_ptr<ModificationNotifier> getNotifier() const;

    public:
        static const uint32_t DbModelVersion;

    private:
        static const std::vector<std::string> supportedVideoExtensions;
        static const std::vector<std::string> supportedAudioExtensions;

    private:
        virtual void startParser();
        virtual void startDiscoverer();
        virtual void startDeletionNotifier();
        bool updateDatabaseModel( unsigned int previousVersion );
        bool createAllTables();
        void registerEntityHooks();
        static bool validateSearchPattern( const std::string& pattern );

    protected:
        std::unique_ptr<SqliteConnection> m_dbConnection;
        std::shared_ptr<factory::IFileSystem> m_fsFactory;
        std::string m_thumbnailPath;
        medialibrary::IMediaLibraryCb* m_callback;

        // Keep the parser as last field.
        // The parser holds a (raw) pointer to the media library. When MediaLibrary's destructor gets called
        // it might still finish a few operations before exiting the parser thread. Those operations are
        // likely to require a valid MediaLibrary, which would be compromised if some fields have already been
        // deleted/destroyed.
        std::unique_ptr<Parser> m_parser;
        // Same reasoning applies here.
        //FIXME: Having to maintain a specific ordering sucks, let's use shared_ptr or something
        std::unique_ptr<DiscovererWorker> m_discoverer;
        std::shared_ptr<ModificationNotifier> m_modificationNotifier;
        medialibrary::LogLevel m_verbosity;
        Settings m_settings;
};
#endif // MEDIALIBRARY_H
