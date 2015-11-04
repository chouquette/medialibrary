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

#include "database/Cache.h"
#include "IArtist.h"
#include "IMediaLibrary.h"

class Artist;
class Media;

namespace policy
{
struct ArtistTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Artist::*const PrimaryKey;
};
}

class Artist : public IArtist, public Cache<Artist, IArtist, policy::ArtistTable>
{
private:
    using _Cache = Cache<Artist, IArtist, policy::ArtistTable>;

public:
    Artist( DBConnection dbConnection, sqlite::Row& row );
    Artist( const std::string& name );
    /**
     * @brief Artist Construct an empty artist, with a DB connection.
     * This is only meant to construct the Unknown Artist virtual representation
     */
    Artist( DBConnection dbConnection );

    virtual unsigned int id() const override;
    virtual const std::string &name() const override;
    virtual const std::string& shortBio() const override;
    bool setShortBio( const std::string& shortBio );
    virtual std::vector<AlbumPtr> albums() const override;
    virtual std::vector<MediaPtr> media() const override;
    bool addMedia( Media* media );
    virtual const std::string& artworkUrl() const override;
    bool setArtworkUrl( const std::string& artworkUrl );
    bool markAsAlbumArtist();

    static bool createTable( DBConnection dbConnection );
    static bool createDefaultArtists( DBConnection dbConnection );
    static std::shared_ptr<Artist> create( DBConnection dbConnection, const std::string& name );

private:
    DBConnection m_dbConnection;
    unsigned int m_id;
    std::string m_name;
    std::string m_shortBio;
    std::string m_artworkUrl;
    bool m_isAlbumArtist;

    friend _Cache;
    friend struct policy::ArtistTable;
};
