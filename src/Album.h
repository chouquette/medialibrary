/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#include "medialibrary/IMediaLibrary.h"

#include "database/DatabaseHelpers.h"
#include "medialibrary/IAlbum.h"
#include "Thumbnail.h"

namespace medialibrary
{

class AlbumTrack;
class Artist;
class Media;

class Album : public IAlbum, public DatabaseHelpers<Album>
{
    public:
        struct Table
        {
            static const std::string Name;
            static const std::string PrimaryKeyColumn;
            static int64_t Album::*const PrimaryKey;
        };
        struct FtsTable
        {
            static const std::string Name;
        };
        enum class Triggers : uint8_t
        {
            IsPresent,
            AddTrack, // Deprecated in model 34
            DeleteTrack,
            InsertFts,
            DeleteFts,
            DeleteEmpty, // Introduced in model 34
        };
        enum class Indexes : uint8_t
        {
            ArtistId,
        };

        Album( MediaLibraryPtr ml, sqlite::Row& row );
        Album( MediaLibraryPtr ml, std::string title );
        /**
         * @brief Album Constructs an unknown album for the provided artist
         */
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
        virtual ThumbnailStatus thumbnailStatus( ThumbnailSizeType sizeType ) const override;
        virtual const std::string& thumbnailMrl( ThumbnailSizeType sizeType ) const override;
        std::shared_ptr<Thumbnail> thumbnail( ThumbnailSizeType sizeType ) const;
        bool setThumbnail(std::shared_ptr<Thumbnail> thumbnail );
        virtual Query<IMedia> tracks( const QueryParameters* params ) const override;
        virtual Query<IMedia> tracks( GenrePtr genre, const QueryParameters* params ) const override;
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
        bool addTrack( std::shared_ptr<Media> media, unsigned int trackNb,
                                         unsigned int discNumber, int64_t artistId, Genre* genre );
        bool removeTrack( Media& media );
        unsigned int nbTracks() const override;
        uint32_t nbPresentTracks() const override;
        virtual uint32_t nbDiscs() const override;
        bool setNbDiscs( uint32_t nbDiscs );
        virtual int64_t duration() const override;
        virtual bool isUnknownAlbum() const override;

        virtual ArtistPtr albumArtist() const override;
        bool setAlbumArtist( std::shared_ptr<Artist> artist );
        virtual Query<IArtist> artists( const QueryParameters* params ) const override;

        virtual Query<IMedia> searchTracks( const std::string& pattern,
                                            const QueryParameters* params = nullptr ) const override;

        static void createTable( sqlite::Connection* dbConnection );
        static void createTriggers( sqlite::Connection* dbConnection );
        static void createIndexes( sqlite::Connection* dbConnection );
        static std::string schema( const std::string& tableName, uint32_t dbModel );
        static std::string trigger( Triggers trigger, uint32_t dbModel );
        static std::string triggerName(Triggers trigger , uint32_t dbModel);
        static std::string index( Indexes index, uint32_t dbModel );
        static std::string indexName( Indexes index, uint32_t dbModel );
        static bool checkDbModel( MediaLibraryPtr ml );
        static std::shared_ptr<Album> create( MediaLibraryPtr ml, std::string title );
        static std::shared_ptr<Album> createUnknownAlbum( MediaLibraryPtr ml, const Artist* artist );
        ///
        /// \brief search search for an album, through its albumartist or title
        /// \param pattern A pattern representing the title, or the name of the main artist
        /// \return
        ///
        static Query<IAlbum> search( MediaLibraryPtr ml, const std::string& pattern,
                                     const QueryParameters* params );
        static Query<IAlbum> searchFromArtist( MediaLibraryPtr ml, const std::string& pattern,
                                               int64_t artistId, const QueryParameters* params );
        static Query<IAlbum> fromArtist( MediaLibraryPtr ml, int64_t artistId, const QueryParameters* params );
        static Query<IAlbum> fromGenre( MediaLibraryPtr ml, int64_t genreId, const QueryParameters* params );
        static Query<IAlbum> searchFromGenre( MediaLibraryPtr ml, const std::string& pattern,
                                              int64_t genreId, const QueryParameters* params );
        static Query<IAlbum> listAll( MediaLibraryPtr ml, const QueryParameters* params );
        /**
         * @brief checkDBConsistency Checks the consistency of all artists records
         * @return false in case of an inconsistency
         *
         * This is meant for testing only, and expected all devices to be back to
         * present once this is called
         */
        static bool checkDBConsistency( MediaLibraryPtr ml );

    private:
        static std::string addRequestJoin( const QueryParameters* params,
                                           bool albumTrack);
        static std::string orderTracksBy( const QueryParameters* params );
        static std::string orderBy( const QueryParameters* params );
        static bool shouldUpdateThumbnail( const Thumbnail& currentThumbnail );

    protected:
        MediaLibraryPtr m_ml;
        int64_t m_id;
        const std::string m_title;
        int64_t m_artistId;
        unsigned int m_releaseYear;
        std::string m_shortSummary;
        unsigned int m_nbTracks;
        int64_t m_duration;
        uint32_t m_nbDiscs;
        uint32_t m_nbPresentTracks;

        mutable std::vector<MediaPtr> m_tracks;
        mutable std::shared_ptr<Artist> m_albumArtist;
        mutable std::shared_ptr<Thumbnail> m_thumbnails[Thumbnail::SizeToInt( ThumbnailSizeType::Count )];
};

}

#endif // ALBUM_H
