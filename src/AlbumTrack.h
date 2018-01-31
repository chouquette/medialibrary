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
#include <vector>

#include "medialibrary/IAlbumTrack.h"
#include "medialibrary/IMediaLibrary.h"
#include "database/DatabaseHelpers.h"
#include "utils/Cache.h"

namespace medialibrary
{

class Album;
class AlbumTrack;
class Artist;
class Media;
class Genre;

namespace policy
{
struct AlbumTrackTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t AlbumTrack::*const PrimaryKey;
};
}

class AlbumTrack : public IAlbumTrack, public DatabaseHelpers<AlbumTrack, policy::AlbumTrackTable>
{
    public:
        AlbumTrack( MediaLibraryPtr ml, sqlite::Row& row );
        AlbumTrack( MediaLibraryPtr ml, int64_t mediaId, int64_t artistId, int64_t genreId,
                    unsigned int trackNumber, int64_t albumId, unsigned int discNumber );

        virtual int64_t id() const override;
        virtual ArtistPtr artist() const override;
        virtual GenrePtr genre() override;
        bool setGenre( std::shared_ptr<Genre> genre );
        virtual unsigned int trackNumber() override;
        virtual unsigned int discNumber() const override;
        virtual std::shared_ptr<IAlbum> album() override;
        virtual std::shared_ptr<IMedia> media() override;

        static void createTable( sqlite::Connection* dbConnection );
        static std::shared_ptr<AlbumTrack> create(MediaLibraryPtr ml, int64_t albumId,
                                    std::shared_ptr<Media> media, unsigned int trackNb,
                                    unsigned int discNumber, int64_t artistId, int64_t genreId,
                                    int64_t duration );
        static AlbumTrackPtr fromMedia( MediaLibraryPtr ml, int64_t mediaId );
        static std::vector<MediaPtr> fromGenre( MediaLibraryPtr ml, int64_t genreId, SortingCriteria sort, bool desc );
        static std::vector<MediaPtr> search( sqlite::Connection* dbConn, const std::string& title );

    private:
        MediaLibraryPtr m_ml;
        int64_t m_id;
        int64_t m_mediaId;
        int64_t m_artistId;
        int64_t m_genreId;
        unsigned int m_trackNumber;
        int64_t m_albumId;
        unsigned int m_discNumber;
        bool m_isPresent;

        mutable Cache<std::weak_ptr<Album>> m_album;
        mutable Cache<std::shared_ptr<Artist>> m_artist;
        mutable Cache<std::shared_ptr<Genre>> m_genre;
        mutable Cache<std::weak_ptr<Media>> m_media;

        friend struct policy::AlbumTrackTable;
};

}

#endif // ALBUMTRACK_H
