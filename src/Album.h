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

#include "medialibrary/IMediaLibrary.h"

#include "database/DatabaseHelpers.h"
#include "medialibrary/IAlbum.h"
#include "utils/Cache.h"

namespace medialibrary
{

class Album;
class AlbumTrack;
class Artist;
class Media;

namespace policy
{
struct AlbumTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t Album::*const PrimaryKey;
};
}

class Album : public IAlbum, public DatabaseHelpers<Album, policy::AlbumTable>
{
    public:
        Album( MediaLibraryPtr ml, sqlite::Row& row );
        Album( MediaLibraryPtr ml, const std::string& title );
        Album( MediaLibraryPtr ml, const Artist* artist );

        virtual int64_t id() const override;
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
        virtual const std::string& artworkMrl() const override;
        bool setArtworkMrl( const std::string& artworkMrl );
        virtual std::vector<MediaPtr> tracks( SortingCriteria sort, bool desc ) const override;
        virtual std::vector<MediaPtr> tracks( GenrePtr genre, SortingCriteria sort, bool desc ) const override;
        ///
        /// \brief cachedTracks Returns a cached list of tracks
        /// This has no warranty of ordering, validity, or anything else.
        /// \return An unordered-list of this album's tracks
        ///
        std::vector<MediaPtr> cachedTracks() const;
        ///
        /// \brief addTrack Add a track to the album.
        /// This will modify the media, but *not* save it.
        /// The media will be added to the tracks cache.
        ///
        std::shared_ptr<AlbumTrack> addTrack( std::shared_ptr<Media> media, unsigned int trackNb, unsigned int discNumber);
        unsigned int nbTracks() const override;
        unsigned int duration() const override;

        virtual ArtistPtr albumArtist() const override;
        bool setAlbumArtist( std::shared_ptr<Artist> artist );
        virtual std::vector<ArtistPtr> artists(bool desc) const override;
        bool addArtist( std::shared_ptr<Artist> artist );
        bool removeArtist( Artist* artist );

        static bool createTable( DBConnection dbConnection );
        static bool createTriggers( DBConnection dbConnection );
        static std::shared_ptr<Album> create( MediaLibraryPtr ml, const std::string& title );
        static std::shared_ptr<Album> createUnknownAlbum( MediaLibraryPtr ml, const Artist* artist );
        ///
        /// \brief search search for an album, through its albumartist or title
        /// \param pattern A pattern representing the title, or the name of the main artist
        /// \return
        ///
        static std::vector<AlbumPtr> search( MediaLibraryPtr ml, const std::string& pattern );
        static std::vector<AlbumPtr> fromArtist( MediaLibraryPtr ml, int64_t artistId, SortingCriteria sort, bool desc );
        static std::vector<AlbumPtr> fromGenre( MediaLibraryPtr ml, int64_t genreId, SortingCriteria sort, bool desc );
        static std::vector<AlbumPtr> listAll( MediaLibraryPtr ml, SortingCriteria sort, bool desc );

    private:
        static std::string orderTracksBy( SortingCriteria sort, bool desc );
        static std::string orderBy( SortingCriteria sort, bool desc );
    protected:
        MediaLibraryPtr m_ml;
        int64_t m_id;
        std::string m_title;
        int64_t m_artistId;
        unsigned int m_releaseYear;
        std::string m_shortSummary;
        std::string m_artworkMrl;
        unsigned int m_nbTracks;
        unsigned int m_duration;
        bool m_isPresent;

        mutable Cache<std::vector<MediaPtr>> m_tracks;
        mutable Cache<std::shared_ptr<Artist>> m_albumArtist;

        friend struct policy::AlbumTable;
};

}

#endif // ALBUM_H
