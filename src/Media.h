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
#include "database/Cache.h"

class Album;
class ShowEpisode;
class AlbumTrack;

class Media;

namespace policy
{
struct MediaTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Media::*const PrimaryKey;
};
struct MediaCache
{
    typedef std::string KeyType;
    static const std::string& key(const std::shared_ptr<Media> self);
    static std::string key( sqlite3_stmt* stmt );
};
}

class Media : public IMedia, public Cache<Media, IMedia, policy::MediaTable, policy::MediaCache>
{
    private:
        typedef Cache<Media, IMedia, policy::MediaTable, policy::MediaCache> _Cache;
    public:

        // Those should be private, however the standard states that the expression
        // ::new (pv) T(std::forward(args)...)
        // shall be well-formed, and private constructor would prevent that.
        // There might be a way with a user-defined allocator, but we'll see that later...
        Media(DBConnection dbConnection , sqlite3_stmt* stmt);
        Media(const fs::IFile* file, unsigned int folderId, const std::string &title, Type type);

        static std::shared_ptr<Media> create(DBConnection dbConnection, Type type, const fs::IFile* file , unsigned int folderId);
        static bool createTable( DBConnection connection );

        virtual unsigned int id() const override;
        virtual Type type() override;
        bool setType( Type type );
        virtual const std::string& title() override;
        bool setTitle( const std::string& title );
        virtual AlbumTrackPtr albumTrack() override;
        bool setAlbumTrack( AlbumTrackPtr albumTrack );
        bool addArtist( ArtistPtr artist );
        virtual std::vector<ArtistPtr> artists() const override;
        virtual int64_t duration() const override;
        bool setDuration( int64_t duration);
        virtual std::shared_ptr<IShowEpisode> showEpisode() override;
        bool setShowEpisode( ShowEpisodePtr showEpisode );
        virtual bool addLabel( LabelPtr label ) override;
        virtual bool removeLabel( LabelPtr label ) override;
        virtual std::vector<std::shared_ptr<ILabel> > labels() override;
        virtual int playCount() const  override;
        virtual const std::string& mrl() const override;
        virtual MoviePtr movie() override;
        bool setMovie( MoviePtr movie );
        bool addVideoTrack( const std::string& codec, unsigned int width,
                                    unsigned int height, float fps );
        virtual std::vector<VideoTrackPtr> videoTracks() override;
        bool addAudioTrack(const std::string& codec, unsigned int bitrate , unsigned int sampleRate, unsigned int nbChannels);
        virtual std::vector<AudioTrackPtr> audioTracks() override;
        virtual const std::string& snapshot() override;
        bool setSnapshot( const std::string& snapshot );

        bool isStandAlone();
        unsigned int lastModificationDate();

        /// Explicitely mark a file as fully parsed, meaning no metadata service needs to run anymore.
        //FIXME: This lacks granularity as we don't have a straight forward way to know which service
        //needs to run or not.
        bool markParsed();
        bool isParsed() const;

    private:
        DBConnection m_dbConnection;

        // DB fields:
        unsigned int m_id;
        Type m_type;
        int64_t m_duration;
        unsigned int m_playCount;
        unsigned int m_showEpisodeId;
        std::string m_mrl;
        unsigned int m_movieId;
        unsigned int m_folderId;
        unsigned int m_lastModificationDate;
        std::string m_snapshot;
        bool m_isParsed;
        std::string m_title;

        // Auto fetched related properties
        Album* m_album;
        AlbumTrackPtr m_albumTrack;
        ShowEpisodePtr m_showEpisode;
        MoviePtr m_movie;

        friend class Cache<Media, IMedia, policy::MediaTable, policy::MediaCache>;
        friend struct policy::MediaTable;
};

#endif // FILE_H
