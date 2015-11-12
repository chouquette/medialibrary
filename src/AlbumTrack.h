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
#include "database/SqliteTable.h"

class Album;
class AlbumTrack;
class Media;

namespace policy
{
struct AlbumTrackTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int AlbumTrack::*const PrimaryKey;
};
}

class AlbumTrack : public IAlbumTrack, public Table<AlbumTrack, policy::AlbumTrackTable>
{
    using _Cache = Table<AlbumTrack, policy::AlbumTrackTable>;

    public:
        AlbumTrack( DBConnection dbConnection, sqlite::Row& row );
        AlbumTrack(Media* media, unsigned int trackNumber, unsigned int albumId , unsigned int discNumber);

        virtual unsigned int id() const override;
        virtual const std::string& artist() const override;
        bool setArtist( const std::string& artist );
        virtual const std::string& genre() override;
        bool setGenre( const std::string& genre );
        virtual unsigned int trackNumber() override;
        virtual unsigned int releaseYear() const override;
        bool setReleaseYear( unsigned int year );
        virtual unsigned int discNumber() const override;
        virtual std::shared_ptr<IAlbum> album() override;

        static bool createTable( DBConnection dbConnection );
        static std::shared_ptr<AlbumTrack> create(DBConnection dbConnection, unsigned int albumId,
                                     Media* media, unsigned int trackNb , unsigned int discNumber);

    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        unsigned int m_mediaId;
        std::string m_artist;
        std::string m_genre;
        unsigned int m_trackNumber;
        unsigned int m_albumId;
        unsigned int m_releaseYear;
        unsigned int m_discNumber;

        std::shared_ptr<Album> m_album;

        friend _Cache;
        friend struct policy::AlbumTrackTable;
};

#endif // ALBUMTRACK_H
