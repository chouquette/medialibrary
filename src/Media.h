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
#include "database/DatabaseHelpers.h"
#include "utils/Cache.h"

class Album;
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
    public:
        // Those should be private, however the standard states that the expression
        // ::new (pv) T(std::forward(args)...)
        // shall be well-formed, and private constructor would prevent that.
        // There might be a way with a user-defined allocator, but we'll see that later...
        Media(DBConnection dbConnection , sqlite::Row& row);
        Media( const fs::IFile* file, unsigned int folderId, const std::string &title, Type type, bool isRemovable );

        static std::shared_ptr<Media> create( DBConnection dbConnection, Type type, const fs::IFile* file , unsigned int folderId, bool isRemovable );
        static bool createTable( DBConnection connection );

        virtual unsigned int id() const override;
        virtual Type type() override;
        void setType( Type type );
        virtual const std::string& title() override;
        void setTitle( const std::string& title );
        virtual AlbumTrackPtr albumTrack() override;
        bool setAlbumTrack( AlbumTrackPtr albumTrack );
        virtual const std::string& artist() const override;
        void setArtist( const std::string& artist );
        virtual int64_t duration() const override;
        void setDuration( int64_t duration);
        virtual std::shared_ptr<IShowEpisode> showEpisode() override;
        void setShowEpisode( ShowEpisodePtr showEpisode );
        virtual bool addLabel( LabelPtr label ) override;
        virtual bool removeLabel( LabelPtr label ) override;
        virtual std::vector<LabelPtr> labels() override;
        virtual int playCount() const  override;
        virtual void increasePlayCount() override;
        virtual float progress() const override;
        virtual void setProgress( float progress ) override;
        virtual int rating() const override;
        virtual void setRating( int rating ) override;
        virtual const std::string& mrl() const override;
        virtual MoviePtr movie() override;
        void setMovie( MoviePtr movie );
        bool addVideoTrack( const std::string& codec, unsigned int width,
                                    unsigned int height, float fps );
        virtual std::vector<VideoTrackPtr> videoTracks() override;
        bool addAudioTrack(const std::string& codec, unsigned int bitrate, unsigned int sampleRate,
                           unsigned int nbChannels, const std::string& language, const std::string& desc );
        virtual std::vector<AudioTrackPtr> audioTracks() override;
        virtual const std::string& snapshot() override;
        virtual unsigned int insertionDate() const override;
        void setSnapshot( const std::string& snapshot );
        bool save();

        unsigned int lastModificationDate();

        /// Explicitely mark a file as fully parsed, meaning no metadata service needs to run anymore.
        //FIXME: This lacks granularity as we don't have a straight forward way to know which service
        //needs to run or not.
        void markParsed();
        bool isParsed() const;

    private:
        DBConnection m_dbConnection;

        // DB fields:
        unsigned int m_id;
        Type m_type;
        int64_t m_duration;
        unsigned int m_playCount;
        float m_progress;
        int m_rating;
        unsigned int m_showEpisodeId;
        std::string m_mrl;
        std::string m_artist;
        unsigned int m_movieId;
        unsigned int m_folderId;
        unsigned int m_lastModificationDate;
        unsigned int m_insertionDate;
        std::string m_snapshot;
        bool m_isParsed;
        std::string m_title;
        bool m_isPresent;
        bool m_isRemovable;

        // Auto fetched related properties
        AlbumTrackPtr m_albumTrack;
        ShowEpisodePtr m_showEpisode;
        MoviePtr m_movie;
        bool m_changed;
        mutable Cache<std::string> m_fullPath;

        friend struct policy::MediaTable;
};

#endif // FILE_H
