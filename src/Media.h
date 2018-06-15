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

namespace medialibrary
{

class Album;
class Folder;
class ShowEpisode;
class AlbumTrack;

class Media;

namespace policy
{
struct MediaTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t Media::*const PrimaryKey;
};

struct MediaMetadataTable
{
    static const std::string Name;
};
}

class Media : public IMedia, public DatabaseHelpers<Media, policy::MediaTable>
{
    class MediaMetadata : public IMediaMetadata
    {
    public:
        MediaMetadata(MetadataType t, std::string v) : m_type( t ), m_value( std::move( v ) ), m_isSet( true ) {}
        MediaMetadata(MetadataType t) : m_type( t ), m_isSet( false ) {}
        virtual bool isSet() const override;
        virtual int64_t integer() const override;
        virtual const std::string& str() const override;

    private:
        void set( const std::string& value );

    private:
        MetadataType m_type;
        std::string m_value;
        bool m_isSet;
        friend class Media;
    };

    public:
        // Those should be private, however the standard states that the expression
        // ::new (pv) T(std::forward(args)...)
        // shall be well-formed, and private constructor would prevent that.
        // There might be a way with a user-defined allocator, but we'll see that later...
        Media( MediaLibraryPtr ml , sqlite::Row& row );
        Media( MediaLibraryPtr ml, const std::string& title, Type type);

        static std::shared_ptr<Media> create( MediaLibraryPtr ml, Type type, const std::string& fileName );
        static void createTable( sqlite::Connection* connection );
        static void createTriggers( sqlite::Connection* connection );

        virtual int64_t id() const override;
        virtual Type type() const override;
        virtual SubType subType() const override;
        void setType( Type type );
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
        virtual Query<ILabel> labels() override;
        virtual int playCount() const  override;
        virtual bool increasePlayCount() override;
        virtual bool isFavorite() const override;
        virtual bool setFavorite( bool favorite ) override;
        virtual const std::vector<FilePtr>& files() const override;
        virtual const std::string& fileName() const override;
        virtual MoviePtr movie() const override;
        void setMovie( MoviePtr movie );
        bool addVideoTrack( const std::string& codec, unsigned int width, unsigned int height,
                            float fps, const std::string& language, const std::string& description );
        virtual Query<IVideoTrack> videoTracks() override;
        bool addAudioTrack( const std::string& codec, unsigned int bitrate, unsigned int sampleRate,
                            unsigned int nbChannels, const std::string& language, const std::string& desc );
        virtual Query<IAudioTrack> audioTracks() override;
        virtual const std::string& thumbnail() const override;
        virtual bool isThumbnailGenerated() const override;
        virtual bool setThumbnail( const std::string &thumbnail ) override;
        bool setThumbnail( const std::string& thumbnail, Thumbnail::Origin origin );
        virtual unsigned int insertionDate() const override;
        virtual unsigned int releaseDate() const override;

        virtual const IMediaMetadata& metadata( MetadataType type ) const override;
        virtual bool setMetadata( MetadataType type, const std::string& value ) override;
        virtual bool setMetadata( MetadataType type, int64_t value ) override;

        void setReleaseDate( unsigned int date );
        bool save();

        std::shared_ptr<File> addFile( const fs::IFile& fileFs, int64_t parentFolderId,
                                       bool isFolderFsRemovable, IFile::Type type );
        virtual FilePtr addExternalMrl( const std::string& mrl, IFile::Type type ) override;
        void removeFile( File& file );

        static Query<IMedia> listAll(MediaLibraryPtr ml, Type type, const QueryParameters* params );

        static Query<IMedia> search( MediaLibraryPtr ml, const std::string& title,
                                     Media::SubType subType, const QueryParameters* params );
        static Query<IMedia> fetchHistory( MediaLibraryPtr ml );

        static void clearHistory( MediaLibraryPtr ml );

private:
        static std::string sortRequest( const QueryParameters* params );

private:
        MediaLibraryPtr m_ml;

        // DB fields:
        int64_t m_id;
        Type m_type;
        SubType m_subType;
        int64_t m_duration;
        unsigned int m_playCount;
        std::time_t m_lastPlayedDate;
        std::time_t m_insertionDate;
        unsigned int m_releaseDate;
        int64_t m_thumbnailId;
        unsigned int m_thumbnailGenerated;
        std::string m_title;
        // We store the filename as a shortcut when sorting. The filename (*not* the title
        // might be used as a fallback
        std::string m_filename;
        bool m_isFavorite;
        bool m_isPresent;

        // Auto fetched related properties
        mutable Cache<AlbumTrackPtr> m_albumTrack;
        mutable Cache<ShowEpisodePtr> m_showEpisode;
        mutable Cache<MoviePtr> m_movie;
        mutable Cache<std::vector<FilePtr>> m_files;
        mutable Cache<std::vector<MediaMetadata>> m_metadata;
        mutable Cache<std::shared_ptr<Thumbnail>> m_thumbnail;
        bool m_changed;

        friend policy::MediaTable;
};

}
