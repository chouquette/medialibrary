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

#include "IMedia.h"
#include "File.h"
#include "database/DatabaseHelpers.h"
#include "utils/Cache.h"

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
    static unsigned int Media::*const PrimaryKey;
};
}

class Media : public IMedia, public DatabaseHelpers<Media, policy::MediaTable>
{
    enum class SubType : uint8_t
    {
        Unknown,
        ShowEpisode,
        Movie,
        AlbumTrack,
    };

    public:
        // Those should be private, however the standard states that the expression
        // ::new (pv) T(std::forward(args)...)
        // shall be well-formed, and private constructor would prevent that.
        // There might be a way with a user-defined allocator, but we'll see that later...
        Media( DBConnection dbConnection , sqlite::Row& row );
        Media( const std::string &title, Type type);

        static std::shared_ptr<Media> create( DBConnection dbConnection, Type type, const fs::IFile* file );
        static bool createTable( DBConnection connection );
        static bool createTriggers( DBConnection connection );

        virtual unsigned int id() const override;
        virtual Type type() override;
        void setType( Type type );
        virtual const std::string& title() override;
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
        virtual void increasePlayCount() override;
        virtual float progress() const override;
        virtual void setProgress( float progress ) override;
        virtual int rating() const override;
        virtual void setRating( int rating ) override;
        virtual const std::vector<FilePtr>& files() const override;
        virtual MoviePtr movie() const override;
        void setMovie( MoviePtr movie );
        bool addVideoTrack( const std::string& codec, unsigned int width,
                                    unsigned int height, float fps );
        virtual std::vector<VideoTrackPtr> videoTracks() override;
        bool addAudioTrack(const std::string& codec, unsigned int bitrate, unsigned int sampleRate,
                           unsigned int nbChannels, const std::string& language, const std::string& desc );
        virtual std::vector<AudioTrackPtr> audioTracks() override;
        virtual const std::string& thumbnail() override;
        virtual unsigned int insertionDate() const override;
        void setThumbnail( const std::string& thumbnail );
        bool save();

        std::shared_ptr<File> addFile(const fs::IFile& fileFs, Folder& parentFolder, fs::IDirectory& parentFolderFs , IFile::Type type);
        void removeFile( File& file );

    private:
        DBConnection m_dbConnection;

        // DB fields:
        unsigned int m_id;
        Type m_type;
        SubType m_subType;
        int64_t m_duration;
        unsigned int m_playCount;
        float m_progress;
        int m_rating;
        unsigned int m_insertionDate;
        std::string m_thumbnail;
        std::string m_title;

        // Auto fetched related properties
        mutable Cache<AlbumTrackPtr> m_albumTrack;
        mutable Cache<ShowEpisodePtr> m_showEpisode;
        mutable Cache<MoviePtr> m_movie;
        mutable Cache<std::vector<FilePtr>> m_files;
        bool m_changed;

        friend struct policy::MediaTable;
};

#endif // FILE_H
