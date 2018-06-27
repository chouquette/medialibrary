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

#pragma once

#include "database/DatabaseHelpers.h"
#include "medialibrary/IArtist.h"
#include "medialibrary/IMediaLibrary.h"
#include "utils/Cache.h"
#include "Thumbnail.h"

namespace medialibrary
{

class Artist;
class Album;
class Media;

namespace policy
{
struct ArtistTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t Artist::*const PrimaryKey;
};
}

class Artist : public IArtist, public DatabaseHelpers<Artist, policy::ArtistTable>
{
public:
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
    bool addMedia( Media& tracks );
    virtual const std::string& artworkMrl() const override;
    std::shared_ptr<Thumbnail> thumbnail();
    bool setArtworkMrl( const std::string& artworkMrl, Thumbnail::Origin origin );
    bool updateNbAlbum( int increment );
    bool updateNbTrack( int increment );
    std::shared_ptr<Album> unknownAlbum();
    virtual const std::string& musicBrainzId() const override;
    bool setMusicBrainzId( const std::string& musicBrainzId );
    virtual unsigned int nbAlbums() const override;
    virtual unsigned int nbTracks() const override;

    static void createTable( sqlite::Connection* dbConnection );
    static void createTriggers( sqlite::Connection* dbConnection, uint32_t dbModelVersion );
    static bool createDefaultArtists( sqlite::Connection* dbConnection );
    static std::shared_ptr<Artist> create( MediaLibraryPtr ml, const std::string& name );
    static Query<IArtist> search( MediaLibraryPtr ml, const std::string& name,
                                           const QueryParameters* params );
    static Query<IArtist> listAll( MediaLibraryPtr ml, bool includeAll,
                                   const QueryParameters* params );

private:
    static std::string sortRequest( const QueryParameters* params );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    std::string m_name;
    std::string m_shortBio;
    int64_t m_thumbnailId;
    unsigned int m_nbAlbums;
    unsigned int m_nbTracks;
    bool m_isPresent;
    std::string m_mbId;

    mutable Cache<std::shared_ptr<Thumbnail>> m_thumbnail;

    friend struct policy::ArtistTable;
};

}
