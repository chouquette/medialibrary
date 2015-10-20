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

#include "Artist.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Media.h"

#include "database/SqliteTools.h"

const std::string policy::ArtistTable::Name = "artist";
const std::string policy::ArtistTable::CacheColumn = "id_artist";
unsigned int Artist::*const policy::ArtistTable::PrimaryKey = &Artist::m_id;


Artist::Artist( DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_name
        >> m_shortBio;
}

Artist::Artist( const std::string& name )
    : m_id( 0 )
    , m_name( name )
{
}

Artist::Artist(DBConnection dbConnection)
    : m_dbConnection( dbConnection )
    , m_id( 0 )
{
}

unsigned int Artist::id() const
{
    return m_id;
}

const std::string &Artist::name() const
{
    return m_name;
}

const std::string &Artist::shortBio() const
{
    return m_shortBio;
}

bool Artist::setShortBio(const std::string &shortBio)
{
    static const std::string req = "UPDATE " + policy::ArtistTable::Name
            + " SET shortbio = ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, shortBio, m_id ) == false )
        return false;
    m_shortBio = shortBio;
    return true;
}

std::vector<AlbumPtr> Artist::albums() const
{
    if ( m_id == 0 )
        return {};
    static const std::string req = "SELECT alb.* FROM " + policy::AlbumTable::Name + " alb "
            "LEFT JOIN AlbumArtistRelation aar ON aar.id_album = alb.id_album "
            "WHERE aar.id_artist = ?";
    return Album::fetchAll( m_dbConnection, req, m_id );
}

std::vector<MediaPtr> Artist::media() const
{
    if ( m_id )
    {
        static const std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
                "LEFT JOIN MediaArtistRelation mar ON mar.id_media = med.id_media "
                "WHERE mar.id_artist = ?";
        return Media::fetchAll( m_dbConnection, req, m_id );
    }
    else
    {
        // Not being able to rely on ForeignKey here makes me a saaaaad panda...
        // But sqlite only accepts "IS NULL" to compare against NULL...
        static const std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
                "LEFT JOIN MediaArtistRelation mar ON mar.id_media = med.id_media "
                "WHERE mar.id_artist IS NULL";
        return Media::fetchAll( m_dbConnection, req, m_id );
    }
}

bool Artist::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::ArtistTable::Name +
            "("
                "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT UNIQUE ON CONFLICT FAIL,"
                "shortbio TEXT"
            ")";
    static const std::string reqRel = "CREATE TABLE IF NOT EXISTS MediaArtistRelation("
                "id_media INTEGER NOT NULL,"
                "id_artist INTEGER,"
                "PRIMARY KEY (id_media, id_artist),"
                "FOREIGN KEY(id_media) REFERENCES " + policy::MediaTable::Name +
                "(id_media) ON DELETE CASCADE,"
                "FOREIGN KEY(id_artist) REFERENCES " + policy::ArtistTable::Name + "("
                    + policy::ArtistTable::CacheColumn + ") ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, reqRel );
}

std::shared_ptr<Artist> Artist::create( DBConnection dbConnection, const std::string &name )
{
    auto artist = std::make_shared<Artist>( name );
    static const std::string req = "INSERT INTO " + policy::ArtistTable::Name +
            "(id_artist, name) VALUES(NULL, ?)";
    if ( _Cache::insert( dbConnection, artist, req, name ) == false )
        return nullptr;
    artist->m_dbConnection = dbConnection;
    return artist;
}

