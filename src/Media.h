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

#ifndef FILE_H
#define FILE_H

#include <sqlite3.h>

#include "medialibrary/IMedia.h"
#include "factory/IFileSystem.h"
#include "File.h"
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
}

class Media : public IMedia, public DatabaseHelpers<Media, policy::MediaTable>
{
    public:
        // Those should be private, however the standard states that the expression
        // ::new (pv) T(std::forward(args)...)
        // shall be well-formed, and private constructor would prevent that.
        // There might be a way with a user-defined allocator, but we'll see that later...
        Media( MediaLibraryPtr ml , sqlite::Row& row );
        Media( MediaLibraryPtr ml, const std::string &title, Type type);

        static std::shared_ptr<Media> create( MediaLibraryPtr ml, Type type, const fs::IFile& file );
        static bool createTable( DBConnection connection );
        static bool createTriggers( DBConnection connection );

        virtual int64_t id() const override;
        virtual Type type() override;
        virtual SubType subType() const override;
        void setType( Type type );
        virtual const std::string& title() const override;
        void setTitle( const std::string& title );
        virtual AlbumTrackPtr albumTrack() const override;
        void setAlbumTrack( AlbumTrackPtr albumTrack );
        virtual int64_t duration() const override;
        void setDuration( int64_t duration);
        virtual ShowEpisodePtr showEpisode() const override;
        void setShowEpisode( ShowEpisodePtr episode );
        virtual bool addLabel( LabelPtr label ) override;
        virtual bool removeLabel( LabelPtr label ) override;
        virtual std::vector<LabelPtr> labels() override;
        virtual int playCount() const  override;
        virtual bool increasePlayCount() override;
        virtual float progress() const override;
        virtual bool setProgress( float progress ) override;
        virtual int rating() const override;
        virtual bool setRating( int rating ) override;
        virtual bool isFavorite() const override;
        virtual bool setFavorite( bool favorite ) override;
        virtual const std::vector<FilePtr>& files() const override;
        virtual MoviePtr movie() const override;
        void setMovie( MoviePtr movie );
        bool addVideoTrack( const std::string& codec, unsigned int width, unsigned int height,
                            float fps, const std::string& language, const std::string& description );
        virtual std::vector<VideoTrackPtr> videoTracks() override;
        bool addAudioTrack( const std::string& codec, unsigned int bitrate, unsigned int sampleRate,
                            unsigned int nbChannels, const std::string& language, const std::string& desc );
        virtual std::vector<AudioTrackPtr> audioTracks() override;
        virtual const std::string& thumbnail() override;
        virtual unsigned int insertionDate() const override;
        virtual unsigned int releaseDate() const override;
        void setReleaseDate( unsigned int date );
        void setThumbnail( const std::string& thumbnail );
        bool save();

        std::shared_ptr<File> addFile( const fs::IFile& fileFs, Folder& parentFolder, fs::IDirectory& parentFolderFs , IFile::Type type);
        void removeFile( File& file );

        static std::vector<MediaPtr> listAll(MediaLibraryPtr ml, Type type , SortingCriteria sort, bool desc);
        static std::vector<MediaPtr> search( MediaLibraryPtr ml, const std::string& title );
        static std::vector<MediaPtr> fetchHistory( MediaLibraryPtr ml );
        static bool clearHistory( MediaLibraryPtr ml );


private:
        MediaLibraryPtr m_ml;

        // DB fields:
        int64_t m_id;
        Type m_type;
        SubType m_subType;
        int64_t m_duration;
        unsigned int m_playCount;
        unsigned int m_lastPlayedDate;
        float m_progress;
        int m_rating;
        unsigned int m_insertionDate;
        unsigned int m_releaseDate;
        std::string m_thumbnail;
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
        bool m_changed;

        friend policy::MediaTable;
};

}
#endif // FILE_H
