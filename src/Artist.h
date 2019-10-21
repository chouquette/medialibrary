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

#pragma once

#include "database/DatabaseHelpers.h"
#include "medialibrary/IArtist.h"
#include "medialibrary/IMediaLibrary.h"
#include "Thumbnail.h"

namespace medialibrary
{

class Artist;
class Album;
class Media;

class Artist : public IArtist, public DatabaseHelpers<Artist>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Artist::*const PrimaryKey;
    };
    struct FtsTable
    {
        static const std::string Name;
    };
    struct MediaRelationTable
    {
        static const std::string Name;
    };

    Artist( MediaLibraryPtr ml, sqlite::Row& row );
    Artist( MediaLibraryPtr ml, const std::string& name );

    virtual int64_t id() const override;
    virtual const std::string& name() const override;
    virtual const std::string& shortBio() const override;
    bool setShortBio( const std::string& shortBio );
    virtual Query<IAlbum> albums( const QueryParameters* params ) const override;
    virtual Query<IAlbum> searchAlbums( const std::string& pattern,
                                        const QueryParameters* params = nullptr ) const override;
    virtual Query<IMedia> tracks( const QueryParameters* params ) const override;
    virtual Query<IMedia> searchTracks( const std::string& pattern,
                                        const QueryParameters* params = nullptr ) const override;
    bool addMedia( Media& tracks );
    virtual bool isThumbnailGenerated( ThumbnailSizeType sizeType ) const override;
    virtual const std::string& thumbnailMrl( ThumbnailSizeType sizeType ) const override;
    std::shared_ptr<Thumbnail> thumbnail( ThumbnailSizeType sizeType ) const;
    /**
     * @brief setThumbnail Set a new thumbnail for this artist.
     * @param newThumbnail A thumbnail object. It can be already inserted in DB or not
     * @param origin The origin for this thumbnail
     * @return true if the thumbnail was updated (or wilfully ignored), false in case of failure
     *
     * The origin parameter is there in case the thumbnail was create for an entity
     * but shared through another one.
     * For instance, if an artist gets assigned the cover from an album, the
     * thumbnail object is likely to have the Media origin if the album was just
     * created.
     * Specifying the origin explicitely allows for a finer control on the hierarchy
     *
     * The implementation may chose to ignore a thumbnail update based on the
     * current & new origin. In this case, `true` will still be returned.
     */
    bool setThumbnail( std::shared_ptr<Thumbnail> newThumbnail, Thumbnail::Origin origin );
    virtual bool setThumbnail( const std::string& thumbnailMrl,
                               ThumbnailSizeType sizeType  ) override;
    std::shared_ptr<Album> unknownAlbum();
    virtual const std::string& musicBrainzId() const override;
    bool setMusicBrainzId( const std::string& musicBrainzId );
    virtual unsigned int nbAlbums() const override;
    virtual unsigned int nbTracks() const override;

    static void createTable( sqlite::Connection* dbConnection );
    static void createTriggers( sqlite::Connection* dbConnection, uint32_t dbModelVersion );
    static std::string schema( const std::string& tableName, uint32_t dbModelVersion );
    static bool checkDbModel( MediaLibraryPtr ml );
    static bool createDefaultArtists( sqlite::Connection* dbConnection );
    static std::shared_ptr<Artist> create( MediaLibraryPtr ml, const std::string& name );
    static Query<IArtist> search(MediaLibraryPtr ml, const std::string& name, bool includeAll,
                                           const QueryParameters* params );
    static Query<IArtist> listAll( MediaLibraryPtr ml, bool includeAll,
                                   const QueryParameters* params );
    static Query<IArtist> searchByGenre( MediaLibraryPtr ml, const std::string& pattern,
                                         const QueryParameters* params, int64_t genreId );
    /**
     * @brief dropMediaArtistRelation Drops any relation between a media and N artists
     * @param ml A media library instance
     * @param mediaId The media to drop the relation with
     *
     * This is intended to remove any link between media & artist(s) when none
     * of them is deleted. When any of those 2 entities is deleted, any relation
     * will automatically get dropped.
     * Effectively, this is meant to be used when refreshing a media, since we
     * can't delete it, at the risk of dropping it from any playlist, and we
     * won't delete an artist when a media gets updated.
     */
    static bool dropMediaArtistRelation( MediaLibraryPtr ml, int64_t mediaId );

    /**
     * @brief checkDBConsistency Checks the consistency of all artists records
     * @return false in case of an inconsistency
     *
     * This is meant for testing only, and expected all devices to be back to
     * present once this is called
     */
    static bool checkDBConsistency( MediaLibraryPtr ml );

private:
    static std::string sortRequest( const QueryParameters* params );
    bool shouldUpdateThumbnail( Thumbnail& currentThumbnail,
                                Thumbnail::Origin newOrigin );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    const std::string m_name;
    std::string m_shortBio;
    unsigned int m_nbAlbums;
    unsigned int m_nbTracks;
    std::string m_mbId;
    bool m_isPresent;

    mutable std::shared_ptr<Thumbnail> m_thumbnails[Thumbnail::SizeToInt( ThumbnailSizeType::Count )];

    friend struct Artist::Table;
};

}
