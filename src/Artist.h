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
    virtual std::vector<AlbumPtr> albums( SortingCriteria sort, bool desc ) const override;
    virtual std::vector<MediaPtr> media(SortingCriteria sort, bool desc) const override;
    bool addMedia( Media& media );
    virtual const std::string& artworkMrl() const override;
    bool setArtworkMrl( const std::string& artworkMrl );
    bool updateNbAlbum( int increment );
    std::shared_ptr<Album> unknownAlbum();
    virtual const std::string& musicBrainzId() const override;
    bool setMusicBrainzId( const std::string& musicBrainzId );

    static bool createTable( sqlite::Connection* dbConnection );
    static bool createTriggers( sqlite::Connection* dbConnection );
    static bool createDefaultArtists( sqlite::Connection* dbConnection );
    static std::shared_ptr<Artist> create( MediaLibraryPtr ml, const std::string& name );
    static std::vector<ArtistPtr> search( MediaLibraryPtr ml, const std::string& name );
    static std::vector<ArtistPtr> listAll( MediaLibraryPtr ml, SortingCriteria sort, bool desc );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    std::string m_name;
    std::string m_shortBio;
    std::string m_artworkMrl;
    unsigned int m_nbAlbums;
    bool m_isPresent;
    std::string m_mbId;

    friend struct policy::ArtistTable;
};

}
