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

#include "medialibrary/IGenre.h"

#include "database/DatabaseHelpers.h"
#include "Thumbnail.h"

namespace medialibrary
{

class Genre : public IGenre, public DatabaseHelpers<Genre>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Genre::*const PrimaryKey;
    };
    struct FtsTable
    {
        static const std::string Name;
    };
    enum class Triggers : uint8_t
    {
        InsertFts,
        DeleteFts,
        UpdateOnNewTrack,
        UpdateOnTrackDelete
    };

    Genre( MediaLibraryPtr ml, sqlite::Row& row );
    Genre( MediaLibraryPtr ml, const std::string& name );
    virtual int64_t id() const override;
    virtual const std::string& name() const override;
    virtual uint32_t nbTracks() const override;
    void updateCachedNbTracks( int increment );
    virtual Query<IArtist> artists( const QueryParameters* params ) const override;
    virtual Query<IArtist> searchArtists( const std::string& pattern,
                                         const QueryParameters* params = nullptr ) const override;
    virtual Query<IMedia> tracks( TracksIncluded included,
                                  const QueryParameters* params ) const override;
    virtual Query<IMedia> searchTracks( const std::string& pattern,
                                  const QueryParameters* params = nullptr ) const override;
    virtual Query<IAlbum> albums( const QueryParameters* params ) const override;
    virtual Query<IAlbum> searchAlbums( const std::string& pattern,
                                        const QueryParameters* params = nullptr ) const override;

    virtual const std::string& thumbnailMrl( ThumbnailSizeType sizeType ) const override;
    virtual bool hasThumbnail( ThumbnailSizeType sizeType ) const override;
    virtual bool setThumbnail( const std::string& mrl, ThumbnailSizeType sizeType,
                               bool takeOwnership ) override;
    std::shared_ptr<Thumbnail> thumbnail( ThumbnailSizeType sizeType ) const;

    static void createTable( sqlite::Connection* dbConn );
    static void createTriggers( sqlite::Connection* dbConn );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static std::string trigger( Triggers trigger, uint32_t dbModel );
    static std::string triggerName( Triggers trigger, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static std::shared_ptr<Genre> create( MediaLibraryPtr ml, const std::string& name );
    static std::shared_ptr<Genre> fromName( MediaLibraryPtr ml, const std::string& name );
    static Query<IGenre> search( MediaLibraryPtr ml, const std::string& name, const QueryParameters* params );
    static Query<IGenre> listAll( MediaLibraryPtr ml, const QueryParameters* params );

private:
    static bool shouldUpdateThumbnail( const Thumbnail& oldThumbnail );

private:
    MediaLibraryPtr m_ml;

    int64_t m_id;
    const std::string m_name;
    uint32_t m_nbTracks;
    mutable std::shared_ptr<Thumbnail> m_thumbnails[Thumbnail::SizeToInt( ThumbnailSizeType::Count )];

    friend Genre::Table;
};

}
