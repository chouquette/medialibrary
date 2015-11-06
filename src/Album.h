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

#ifndef ALBUM_H
#define ALBUM_H

#include <memory>
#include <sqlite3.h>
#include <mutex>

#include "IMediaLibrary.h"

#include "database/Cache.h"
#include "IAlbum.h"

class Album;
class AlbumTrack;
class Artist;
class Media;

namespace policy
{
struct AlbumTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Album::*const PrimaryKey;
};
}

class Album : public IAlbum, public Cache<Album, IAlbum, policy::AlbumTable>
{
    private:
        typedef Cache<Album, IAlbum, policy::AlbumTable> _Cache;
    public:
        Album( DBConnection dbConnection, sqlite::Row& row );
        Album( const std::string& title );
        Album( const Artist* artist );

        virtual unsigned int id() const override;
        virtual const std::string& title() const override;
        virtual unsigned int releaseYear() const override;
        /**
         * @brief setReleaseYear Updates the release year
         * @param date The desired date.
         * @param force If force is true, the date will be applied no matter what.
         *              If force is false, the date will be applied if it never was
         *              applied before. Otherwise, setReleaseYear() will restore the release
         *              date to 0.
         * @return
         */
        bool setReleaseYear( unsigned int date, bool force );
        virtual const std::string& shortSummary() const override;
        bool setShortSummary( const std::string& summary );
        virtual const std::string& artworkUrl() const override;
        bool setArtworkUrl( const std::string& artworkUrl );
        virtual time_t lastSyncDate() const override;
        virtual std::vector<MediaPtr> tracks() const override;
        std::shared_ptr<AlbumTrack> addTrack(std::shared_ptr<Media> media, unsigned int trackNb , unsigned int discNumber);
        unsigned int nbTracks() const override;

        virtual ArtistPtr albumArtist() const override;
        bool setAlbumArtist( Artist* artist );
        virtual std::vector<ArtistPtr> artists() const override;
        bool addArtist( std::shared_ptr<Artist> artist );
        bool removeArtist( Artist* artist );

        static bool createTable( DBConnection dbConnection );
        static std::shared_ptr<Album> create(DBConnection dbConnection, const std::string& title );
        static std::shared_ptr<Album> createUnknownAlbum( DBConnection dbConnection, const Artist* artist );

    protected:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_title;
        unsigned int m_artistId;
        unsigned int m_releaseYear;
        std::string m_shortSummary;
        std::string m_artworkUrl;
        time_t m_lastSyncDate;
        unsigned int m_nbTracks;

        mutable std::vector<MediaPtr> m_tracks;
        mutable bool m_tracksCached;
        mutable std::mutex m_tracksLock;

        friend class Cache<Album, IAlbum, policy::AlbumTable>;
        friend struct policy::AlbumTable;
};

#endif // ALBUM_H
