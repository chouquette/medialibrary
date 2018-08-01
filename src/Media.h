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

#include <sqlite3.h>

#include "medialibrary/IMedia.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "File.h"
#include "Thumbnail.h"
#include "database/DatabaseHelpers.h"
#include "utils/Cache.h"
#include "medialibrary/IMetadata.h"
#include "Metadata.h"

#include <atomic>

namespace medialibrary
{

class Album;
class Folder;
class ShowEpisode;
class AlbumTrack;

class Media;

class Media : public IMedia, public DatabaseHelpers<Media>
{
    public:
        struct Table
        {
            static const std::string Name;
            static const std::string PrimaryKeyColumn;
            static int64_t Media::*const PrimaryKey;
        };
        // Those should be private, however the standard states that the expression
        // ::new (pv) T(std::forward(args)...)
        // shall be well-formed, and private constructor would prevent that.
        // There might be a way with a user-defined allocator, but we'll see that later...
        Media( MediaLibraryPtr ml , sqlite::Row& row );
        Media( MediaLibraryPtr ml, const std::string& title, Type type);

        static std::shared_ptr<Media> create( MediaLibraryPtr ml, Type type, const std::string& fileName );
        static void createTable( sqlite::Connection* connection );
        static void createTriggers( sqlite::Connection* connection, uint32_t modelVersion );

        virtual int64_t id() const override;
        virtual Type type() const override;
        virtual SubType subType() const override;
        virtual const std::string& title() const override;
        virtual bool setTitle( const std::string& title ) override;
        ///
        /// \brief setTitleBuffered Mark the media as changed but doesn't save the change in DB
        /// Querying the title after this method will return the new title, but it won't appear in DB
        /// until save() is called
        ///
        void setTitleBuffered( const std::string& title );
        virtual AlbumTrackPtr albumTrack() const override;
        void setAlbumTrack( AlbumTrackPtr albumTrack );
        virtual int64_t duration() const override;
        void setDuration( int64_t duration);
        virtual ShowEpisodePtr showEpisode() const override;
        void setShowEpisode( ShowEpisodePtr episode );
        virtual bool addLabel( LabelPtr label ) override;
        virtual bool removeLabel( LabelPtr label ) override;
        virtual Query<ILabel> labels() const override;
        virtual int playCount() const  override;
        virtual bool increasePlayCount() override;
        virtual time_t lastPlayedDate() const override;
        virtual bool isFavorite() const override;
        virtual bool setFavorite( bool favorite ) override;
        virtual const std::vector<FilePtr>& files() const override;
        virtual const std::string& fileName() const override;
        virtual MoviePtr movie() const override;
        void setMovie( MoviePtr movie );
        bool addVideoTrack( const std::string& codec, unsigned int width,
                            unsigned int height, uint32_t fpsNum,
                            uint32_t fpsDen, uint32_t bitrate, uint32_t sarNum,
                            uint32_t sarDen, const std::string& language,
                            const std::string& description );
        virtual Query<IVideoTrack> videoTracks() const override;
        bool addAudioTrack( const std::string& codec, unsigned int bitrate, unsigned int sampleRate,
                            unsigned int nbChannels, const std::string& language, const std::string& desc );
        virtual Query<IAudioTrack> audioTracks() const override;
        virtual const std::string& thumbnail() const override;
        virtual bool isThumbnailGenerated() const override;
        virtual bool setThumbnail( const std::string &thumbnail ) override;
        bool setThumbnail( const std::string& thumbnail, Thumbnail::Origin origin );
        virtual unsigned int insertionDate() const override;
        virtual unsigned int releaseDate() const override;
        uint32_t nbPlaylists() const;
        void udpateNbPlaylist( int32_t increment ) const;

        virtual const IMetadata& metadata( MetadataType type ) const override;
        virtual bool setMetadata( MetadataType type, const std::string& value ) override;
        virtual bool setMetadata( MetadataType type, int64_t value ) override;
        virtual bool unsetMetadata( MetadataType type ) override;

        void setReleaseDate( unsigned int date );
        bool save();

        std::shared_ptr<File> addFile( const fs::IFile& fileFs, int64_t parentFolderId,
                                       bool isFolderFsRemovable, IFile::Type type );
        virtual FilePtr addExternalMrl( const std::string& mrl, IFile::Type type ) override;
        void removeFile( File& file );

        static Query<IMedia> listAll(MediaLibraryPtr ml, Type type, const QueryParameters* params );

        static Query<IMedia> search( MediaLibraryPtr ml, const std::string& title,
                                     const QueryParameters* params );
        static Query<IMedia> search(MediaLibraryPtr ml, const std::string& title,
                                    Media::Type subType, const QueryParameters* params );
        static Query<IMedia> searchAlbumTracks( MediaLibraryPtr ml, const std::string& pattern,
                                                int64_t albumId, const QueryParameters* params );
        static Query<IMedia> searchArtistTracks( MediaLibraryPtr ml, const std::string& pattern,
                                                 int64_t artistId, const QueryParameters* params );
        static Query<IMedia> searchGenreTracks( MediaLibraryPtr ml, const std::string& pattern,
                                                 int64_t genreId, const QueryParameters* params );
        static Query<IMedia> searchShowEpisodes( MediaLibraryPtr ml, const std::string& pattern,
                                                 int64_t showId, const QueryParameters* params );
        static Query<IMedia> searchInPlaylist( MediaLibraryPtr ml, const std::string& pattern,
                                                 int64_t playlistId, const QueryParameters* params );
        static Query<IMedia> fetchHistory( MediaLibraryPtr ml );
        static Query<IMedia> fetchStreamHistory( MediaLibraryPtr ml );

        static void clearHistory( MediaLibraryPtr ml );
        static void removeOldMedia( MediaLibraryPtr ml, std::chrono::seconds maxLifeTime );

private:
        static std::string sortRequest( const QueryParameters* params, bool hasAlbTrackTable );

private:
        MediaLibraryPtr m_ml;

        // DB fields:
        int64_t m_id;
        const Type m_type;
        SubType m_subType;
        int64_t m_duration;
        unsigned int m_playCount;
        std::time_t m_lastPlayedDate;
        const std::time_t m_insertionDate;
        unsigned int m_releaseDate;
        int64_t m_thumbnailId;
        unsigned int m_thumbnailGenerated;
        std::string m_title;
        // We store the filename as a shortcut when sorting. The filename (*not* the title
        // might be used as a fallback
        const std::string m_filename;
        bool m_isFavorite;
        bool m_isPresent;
        mutable std::atomic_uint m_nbPlaylists;

        // Auto fetched related properties
        mutable Cache<AlbumTrackPtr> m_albumTrack;
        mutable Cache<ShowEpisodePtr> m_showEpisode;
        mutable Cache<MoviePtr> m_movie;
        mutable Cache<std::vector<FilePtr>> m_files;
        mutable Metadata m_metadata;
        mutable Cache<std::shared_ptr<Thumbnail>> m_thumbnail;
        bool m_changed;

        friend Media::Table;
};

}
