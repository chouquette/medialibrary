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

class Parser;
class DiscovererWorker;
class SqliteConnection;

#include "IMediaLibrary.h"
#include "IDiscoverer.h"
#include "logging/Logger.h"
#include "vlcpp/vlc.hpp"
#include "Settings.h"

class Album;
class Artist;
class Media;
class Movie;
class Show;
class Device;
class Folder;

class MediaLibrary : public IMediaLibrary
{
    public:
        MediaLibrary();
        ~MediaLibrary();
        virtual bool initialize( const std::string& dbPath, const std::string& snapshotPath, IMediaLibraryCb* metadataCb ) override;
        virtual void setVerbosity( LogLevel v ) override;
        virtual void setFsFactory( std::shared_ptr<factory::IFileSystem> fsFactory ) override;

        std::vector<MediaPtr> files();
        virtual std::vector<MediaPtr> audioFiles() override;
        virtual std::vector<MediaPtr> videoFiles() override;
        std::shared_ptr<Media> addFile(const std::string& path, Folder* parentFolder, fs::IDirectory* parentFolderFs);
        virtual bool deleteFile(const Media* file );

        std::shared_ptr<Folder> folder( const std::string& path );
        bool deleteFolder( const Folder* folder );
        std::shared_ptr<Device> device( const std::string& uuid );
        std::shared_ptr<Device> addDevice( const std::string& uuid, bool isRemovable );

        virtual LabelPtr createLabel( const std::string& label ) override;
        virtual bool deleteLabel( LabelPtr label ) override;

        virtual AlbumPtr album( unsigned int id ) override;
        std::shared_ptr<Album> createAlbum( const std::string& title );
        virtual std::vector<AlbumPtr> albums() override;

        virtual ShowPtr show( const std::string& name ) override;
        std::shared_ptr<Show> createShow( const std::string& name );

        virtual MoviePtr movie( const std::string& title ) override;
        std::shared_ptr<Movie> createMovie( const std::string& title );

        virtual ArtistPtr artist( unsigned int id ) override;
        ArtistPtr artist( const std::string& name );
        std::shared_ptr<Artist> createArtist( const std::string& name );
        virtual std::vector<ArtistPtr> artists() const override;

        virtual void discover( const std::string& entryPoint ) override;
        bool ignoreFolder( const std::string& path ) override;

        virtual const std::string& snapshotPath() const override;
        virtual void setLogger( ILogger* logger ) override;
        //Temporarily public, move back to private as soon as we start monitoring the FS
        virtual void reload() override;

        virtual void pauseBackgroundOperations() override;
        virtual void resumeBackgroundOperations() override;

    public:
        static const uint32_t DbModelVersion;

    private:
        static const std::vector<std::string> supportedVideoExtensions;
        static const std::vector<std::string> supportedAudioExtensions;

    private:
        void addMetadataService( std::unique_ptr<IMetadataService> service );
        virtual void startParser();
        virtual void startDiscoverer();

    protected:
        std::unique_ptr<SqliteConnection> m_dbConnection;
        std::shared_ptr<factory::IFileSystem> m_fsFactory;
        std::string m_snapshotPath;
        IMediaLibraryCb* m_callback;

        // This probably qualifies as a work around, but we need to keep the VLC::Instance
        // alive to be able to use the logging wrapper lambda
        VLC::Instance m_vlcInstance;

        // Keep the parser as last field.
        // The parser holds a (raw) pointer to the media library. When MediaLibrary's destructor gets called
        // it might still finish a few operations before exiting the parser thread. Those operations are
        // likely to require a valid MediaLibrary, which would be compromised if some fields have already been
        // deleted/destroyed.
        std::unique_ptr<Parser> m_parser;
        // Same reasoning applies here.
        //FIXME: Having to maintain a specific ordering sucks, let's use shared_ptr or something
        std::unique_ptr<DiscovererWorker> m_discoverer;
        LogLevel m_verbosity;
        Settings m_settings;
};
#endif // MEDIALIBRARY_H
