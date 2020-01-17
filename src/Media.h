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

#include "medialibrary/IMedia.h"
#include "Thumbnail.h"
#include "database/DatabaseHelpers.h"
#include "Metadata.h"

namespace medialibrary
{

class Media : public IMedia,
              public DatabaseHelpers<Media>,
              public std::enable_shared_from_this<Media>
{
    public:
        enum class ImportType : uint8_t
        {
            /**
             * Internal media, which was discovered and imported by the media
             * library.
             */
            Internal,
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
        struct Table
        {
            static const std::string Name;
            static const std::string PrimaryKeyColumn;
            static int64_t Media::*const PrimaryKey;
        };
        struct FtsTable
        {
            static const std::string Name;
        };
        enum class Triggers : uint8_t
        {
            IsPresent,
            CascadeFileDeletion,
            IncrementNbPlaylist,
            DecrementNbPlaylist,
            InsertFts,
            DeleteFts,
            UpdateFts,
        };
        enum class Indexes : uint8_t
        {
            LastPlayedDate,
            Presence,
            Types,
            LastUsageDate,
            Folder,
            MediaGroup,
        };

        // Those should be private, however the standard states that the expression
        // ::new (pv) T(std::forward(args)...)
        // shall be well-formed, and private constructor would prevent that.
        // There might be a way with a user-defined allocator, but we'll see that later...
        Media( MediaLibraryPtr ml , sqlite::Row& row );
        ///
        /// \brief Media Construct a discovered media
        /// \param ml A media library instance pointer
        /// \param title The media title
        /// \param type The media type
        /// \param duration The media duration
        /// \param deviceId The device containing this media's ID
        /// \param folderId The folder containing this media's ID
        ///
        Media( MediaLibraryPtr ml, const std::string& title, Type type,
               int64_t duration, int64_t deviceId, int64_t folderId );

        Media( MediaLibraryPtr ml, const std::string& fileName, ImportType type );

        static std::shared_ptr<Media> create( MediaLibraryPtr ml, Type type,
                                              int64_t deviceId, int64_t folderId,
                                              const std::string& fileName,
                                              int64_t duration );
        static std::shared_ptr<Media> createExternal( MediaLibraryPtr ml,
                                                      const std::string& fileName );
        static std::shared_ptr<Media> createStream( MediaLibraryPtr ml,
                                                    const std::string& fileName );
        static void createTable( sqlite::Connection* connection );
        static void createTriggers( sqlite::Connection* connection, uint32_t modelVersion );
        static void createIndexes( sqlite::Connection* connection, uint32_t modelVersion );
        static std::string schema( const std::string& tableName, uint32_t dbModel );
        static std::string trigger( Triggers trigger, uint32_t dbModel );
        static std::string triggerName( Triggers trigger, uint32_t dbModel );
        static std::string index( Indexes index, uint32_t dbModel );
        static std::string indexName( Indexes index, uint32_t dbModel );
        static bool checkDbModel( MediaLibraryPtr ml );

        virtual int64_t id() const override;
        virtual Type type() const override;
        bool setType( Type type ) override;
        void setTypeBuffered( Type type );
        virtual SubType subType() const override;
        void setSubType( SubType subType );

        virtual const std::string& title() const override;
        bool setTitle( const std::string& title, bool forced );
        ///
        /// \brief setForcedTitle Force the forced_title field to true
        /// \param mediaId The targetted media id
        ///
        /// This is only used for migration 23 -> 24
        ///
        static bool setForcedTitle(MediaLibraryPtr ml, int64_t mediaId );
        ///
        /// \brief isForcedTitle returns true if the title is forced by the user
        ///
        /// This is only intended for unit tests purposes
        ///
        bool isForcedTitle() const;
        ///
        /// \brief setTitleBuffered Mark the media as changed but doesn't save the change in DB
        /// Querying the title after this method will return the new title, but it won't appear in DB
        /// until save() is called
        ///
        void setTitleBuffered( const std::string& title );
        // Should only be used by 13->14 migration
        bool setFileName( std::string fileName );
        virtual AlbumTrackPtr albumTrack() const override;
        void setAlbumTrack( AlbumTrackPtr albumTrack );
        virtual int64_t duration() const override;
        void setDuration( int64_t duration);
        virtual ShowEpisodePtr showEpisode() const override;
        void setShowEpisode( ShowEpisodePtr episode );
        virtual bool addLabel( LabelPtr label ) override;
        virtual bool removeLabel( LabelPtr label ) override;
        virtual Query<ILabel> labels() const override;
        virtual uint32_t playCount() const override;
        virtual bool increasePlayCount() override;
        virtual bool setPlayCount( uint32_t playCount ) override;
        virtual time_t lastPlayedDate() const override;
        virtual bool removeFromHistory() override;
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
        bool addSubtitleTrack( std::string codec, std::string language,
                               std::string description, std::string encoding );
        virtual Query<IAudioTrack> audioTracks() const override;
        virtual Query<ISubtitleTrack> subtitleTracks() const override;
        virtual Query<IChapter> chapters( const QueryParameters* params ) const override;
        bool addChapter( int64_t offset, int64_t duration, std::string name );
        std::shared_ptr<Thumbnail> thumbnail( ThumbnailSizeType sizeType ) const;
        virtual const std::string& thumbnailMrl( ThumbnailSizeType sizeType ) const override;
        virtual ThumbnailStatus thumbnailStatus( ThumbnailSizeType sizeType ) const override;
        virtual bool setThumbnail( const std::string &thumbnail,
                                   ThumbnailSizeType sizeType ) override;
        bool setThumbnail( std::shared_ptr<Thumbnail> thumbnail );
        void removeThumbnail( ThumbnailSizeType sizeType );
        virtual unsigned int insertionDate() const override;
        virtual unsigned int releaseDate() const override;
        /**
         * @brief nbPlaylists Returns the number of occurence of this media in playlists
         *
         * If the media is inserted twice in a playlist, this method will return 2.
         * This is solely meant for unit testing, and the value that matters is
         * the one in database, which is used to filter out some results
         */
        uint32_t nbPlaylists() const;

        virtual const IMetadata& metadata( MetadataType type ) const override;
        virtual std::unordered_map<MetadataType, std::string> metadata() const override;
        virtual bool setMetadata( MetadataType type, const std::string& value ) override;
        virtual bool setMetadata( MetadataType type, int64_t value ) override;
        virtual bool unsetMetadata( MetadataType type ) override;
        virtual bool setMetadata( std::unordered_map<MetadataType, std::string> metadata ) override;

        virtual bool requestThumbnail( ThumbnailSizeType sizeType, uint32_t desiredWidth,
                                       uint32_t desiredHeight, float position ) override;
        virtual bool isDiscoveredMedia() const override;
        virtual bool isExternalMedia() const override;
        virtual bool isStream() const override;

        virtual bool addToGroup( IMediaGroup& group ) override;
        virtual bool addToGroup( int64_t groupId ) override;
        virtual bool removeFromGroup() override;
        virtual bool isGrouped() const override;
        virtual MediaGroupPtr group() const override;
        virtual int64_t groupId() const override;
        bool hasBeenGrouped() const;

        void setReleaseDate( unsigned int date );
        int64_t deviceId() const; // Used for unit tests purposes only
        void setDeviceId( int64_t deviceId );
        int64_t folderId() const; // Used for unit tests purposes only
        void setFolderId( int64_t folderId );
        void markAsInternal();
        bool save();

        std::shared_ptr<File> addFile( const fs::IFile& fileFs, int64_t parentFolderId,
                                       bool isFolderFsRemovable, IFile::Type type );
        virtual FilePtr addFile( const std::string& mrl, IFile::Type fileType ) override;
        virtual FilePtr addExternalMrl( const std::string& mrl, IFile::Type type ) override;
        void removeFile( File& file );

        virtual Query<IBookmark> bookmarks( const QueryParameters* params ) const override;
        virtual BookmarkPtr bookmark( int64_t time ) const override;
        virtual BookmarkPtr addBookmark(int64_t time) override;
        virtual bool removeBookmark( int64_t time ) override;
        virtual bool removeAllBookmarks() override;

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
        static Query<IMedia> fetchHistory( MediaLibraryPtr ml, IMedia::Type type );
        static Query<IMedia> fetchStreamHistory( MediaLibraryPtr ml );
        static Query<IMedia> fromFolderId( MediaLibraryPtr ml, Type type,
                                           int64_t folderId,
                                           const QueryParameters* params );
        static Query<IMedia> searchFromFolderId( MediaLibraryPtr ml,
                                                 const std::string& pattern,
                                                 Type type, int64_t folderId,
                                                 const QueryParameters* params );
        static Query<IMedia> fromMediaGroup( MediaLibraryPtr ml,
                                             int64_t groupId, Type type,
                                             const QueryParameters* params );
        static Query<IMedia> searchFromMediaGroup( MediaLibraryPtr ml,
                                                   int64_t groupId, Type type,
                                                   const std::string& pattern,
                                                   const QueryParameters* params );
        static bool setMediaGroup(MediaLibraryPtr ml, int64_t mediaId, int64_t groupId );

        static bool clearHistory( MediaLibraryPtr ml );
        static bool removeOldMedia( MediaLibraryPtr ml, std::chrono::seconds maxLifeTime );

        /**
         * @brief resetSubTypes Reset all parsed media subtypes
         *
         * This is meant to be invoked as part of a rescan setup, so that the media
         * aren't tagged as an albumtrack/showepisode/movie, since the associated
         * entities will have been deleted
         * This will only affect media with a `Type` of Video/Audio
         */
        static bool resetSubTypes( MediaLibraryPtr ml );

private:
        static std::string addRequestJoin(const QueryParameters* params, bool forceFile , bool forceAlbumTrack);
        static std::string sortRequest( const QueryParameters* params );
        static Query<IMedia> fetchHistoryByType( MediaLibraryPtr ml, IMedia::Type type );
        static bool shouldUpdateThumbnail( const Thumbnail& currentThumbnail );
        static std::shared_ptr<Media> createExternalMedia( MediaLibraryPtr ml,
                                                           const std::string& mrl,
                                                           ImportType importType );

        /*
         * Marked as private so that it can only be used through IMedia interface
         * Internally, we have to use the setTitle( title, force ) overload
        */
        virtual bool setTitle( const std::string& title ) override;


private:
        MediaLibraryPtr m_ml;

        // DB fields:
        int64_t m_id;
        Type m_type;
        SubType m_subType;
        int64_t m_duration;
        unsigned int m_playCount;
        std::time_t m_lastPlayedDate;
        const std::time_t m_insertionDate;
        unsigned int m_releaseDate;
        std::string m_title;
        // We store the filename as a shortcut when sorting. The filename (*not* the title
        // might be used as a fallback
        std::string m_filename;
        bool m_isFavorite;
        int64_t m_deviceId;
        uint32_t m_nbPlaylists;
        int64_t m_folderId;
        ImportType m_importType;
        int64_t m_groupId;
        // Will be false until the media is grouped for the first time.
        // If true, we must not assign this media to a group automatically
        bool m_hasBeenGrouped;
        bool m_forcedTitle;

        // Auto fetched related properties
        mutable AlbumTrackPtr m_albumTrack;
        mutable ShowEpisodePtr m_showEpisode;
        mutable MoviePtr m_movie;
        mutable std::vector<FilePtr> m_files;
        mutable Metadata m_metadata;
        mutable std::shared_ptr<Thumbnail> m_thumbnails[Thumbnail::SizeToInt( ThumbnailSizeType::Count )];
        bool m_changed;

        friend Media::Table;
};

}
