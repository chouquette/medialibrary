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
#include "IArtist.h"
#include "IMediaLibrary.h"

class Artist;
class Album;
class Media;

namespace policy
{
struct ArtistTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static unsigned int Artist::*const PrimaryKey;
};
}

class Artist : public IArtist, public DatabaseHelpers<Artist, policy::ArtistTable>
{
public:
    Artist( DBConnection dbConnection, sqlite::Row& row );
    Artist( const std::string& name );

    virtual unsigned int id() const override;
    virtual const std::string &name() const override;
    virtual const std::string& shortBio() const override;
    bool setShortBio( const std::string& shortBio );
    virtual std::vector<AlbumPtr> albums() const override;
    virtual std::vector<MediaPtr> media() const override;
    bool addMedia( Media* media );
    virtual const std::string& artworkMrl() const override;
    bool setArtworkMrl( const std::string& artworkMrl );
    bool updateNbAlbum( int increment );
    std::shared_ptr<Album> unknownAlbum();
    virtual const std::string& musicBrainzId() const override;
    bool setMusicBrainzId( const std::string& musicBrainzId );

    static bool createTable( DBConnection dbConnection );
    static bool createTriggers( DBConnection dbConnection );
    static bool createDefaultArtists( DBConnection dbConnection );
    static std::shared_ptr<Artist> create( DBConnection dbConnection, const std::string& name );

private:
    DBConnection m_dbConnection;
    unsigned int m_id;
    std::string m_name;
    std::string m_shortBio;
    std::string m_artworkMrl;
    unsigned int m_nbAlbums;
    bool m_isPresent;
    std::string m_mbId;

    friend struct policy::ArtistTable;
};
