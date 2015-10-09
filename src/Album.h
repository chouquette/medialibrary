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

#include "IMediaLibrary.h"

#include "database/Cache.h"
#include "IAlbum.h"

class Album;

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
        Album( DBConnection dbConnection, sqlite3_stmt* stmt );
        Album( const std::string& title );

        virtual unsigned int id() const;
        virtual const std::string& title() const;
        virtual time_t releaseDate() const;
        virtual bool setReleaseDate( time_t date );
        virtual const std::string& shortSummary() const;
        virtual bool setShortSummary( const std::string& summary );
        virtual const std::string& artworkUrl() const;
        virtual bool setArtworkUrl( const std::string& artworkUrl );
        virtual time_t lastSyncDate() const;
        virtual std::vector<std::shared_ptr<IAlbumTrack> > tracks() const;
        virtual AlbumTrackPtr addTrack( const std::string& title, unsigned int trackNb );

        virtual std::vector<ArtistPtr> artists() const override;
        virtual bool addArtist( ArtistPtr artist ) override;

        virtual bool destroy();

        static bool createTable( DBConnection dbConnection );
        static AlbumPtr create(DBConnection dbConnection, const std::string& title );

    protected:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_title;
        unsigned int m_releaseDate;
        std::string m_shortSummary;
        std::string m_artworkUrl;
        time_t m_lastSyncDate;

        friend class Cache<Album, IAlbum, policy::AlbumTable>;
        friend struct policy::AlbumTable;
};

#endif // ALBUM_H
