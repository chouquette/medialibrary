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

#ifndef ALBUMTRACK_H
#define ALBUMTRACK_H

#include <sqlite3.h>
#include <string>

#include "IAlbumTrack.h"
#include "IMediaLibrary.h"
#include "database/Cache.h"

class Album;
class AlbumTrack;

namespace policy
{
struct AlbumTrackTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int AlbumTrack::*const PrimaryKey;
};
}

class AlbumTrack : public IAlbumTrack, public Cache<AlbumTrack, IAlbumTrack, policy::AlbumTrackTable>
{
    private:
        typedef Cache<AlbumTrack, IAlbumTrack, policy::AlbumTrackTable> _Cache;
    public:
        AlbumTrack( DBConnection dbConnection, sqlite3_stmt* stmt );
        AlbumTrack( const std::string& title, unsigned int trackNumber, unsigned int albumId );

        virtual unsigned int id() const;
        virtual const std::string& genre();
        virtual bool setGenre( const std::string& genre );
        virtual const std::string& title();
        virtual unsigned int trackNumber();
        virtual std::shared_ptr<IAlbum> album();
        virtual bool destroy();
        virtual bool addArtist( ArtistPtr artist ) override;
        virtual std::vector<ArtistPtr> artists() const override;
        virtual std::vector<MediaPtr> files();

        static bool createTable( DBConnection dbConnection );
        static std::shared_ptr<AlbumTrack> create( DBConnection dbConnection, unsigned int albumId,
                                     const std::string& name, unsigned int trackNb );

    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_title;
        std::string m_genre;
        unsigned int m_trackNumber;
        unsigned int m_albumId;

        std::shared_ptr<Album> m_album;

        friend class Cache<AlbumTrack, IAlbumTrack, policy::AlbumTrackTable>;
        friend struct policy::AlbumTrackTable;
};

#endif // ALBUMTRACK_H
