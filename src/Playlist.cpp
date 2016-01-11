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

#include "Playlist.h"

#include "Media.h"

namespace policy
{
const std::string PlaylistTable::Name = "Playlist";
const std::string PlaylistTable::PrimaryKeyColumn = "id_playlist";
unsigned int Playlist::* const PlaylistTable::PrimaryKey = &Playlist::m_id;
}

Playlist::Playlist( DBConnection dbConn, sqlite::Row& row )
    : m_dbConnection( dbConn )
{
    row >> m_id
            >> m_name;
}

Playlist::Playlist( const std::string& name )
    : m_id( 0 )
    , m_name( name )
{
}

std::shared_ptr<Playlist> Playlist::create( DBConnection dbConn, const std::string& name )
{
    auto self = std::make_shared<Playlist>( name );
    static const std::string req = "INSERT INTO " + policy::PlaylistTable::Name + "(name) VALUES(?)";
    if ( insert( dbConn, self, req, name ) == false )
        return nullptr;
    self->m_dbConnection = dbConn;
    return self;
}

unsigned int Playlist::id() const
{
    return m_id;
}

const std::string& Playlist::name() const
{
    return m_name;
}

bool Playlist::setName( const std::string& name )
{
    if ( name == m_name )
        return true;
    static const std::string req = "UPDATE " + policy::PlaylistTable::Name + " SET name = ? WHERE id_playlist = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, name, m_id ) == false )
        return false;
    m_name = name;
    return true;
}

std::vector<MediaPtr> Playlist::media() const
{
    static const std::string req = "SELECT m.* FROM " + policy::MediaTable::Name + " m "
            "LEFT JOIN PlaylistMediaRelation pmr ON pmr.media_id = m.id_media "
            "WHERE pmr.playlist_id = ? AND m.is_present = 1 "
            "ORDER BY pmr.position";
    return Media::fetchAll<IMedia>( m_dbConnection, req, m_id );
}

bool Playlist::append( unsigned int mediaId )
{
    return add( mediaId, 0 );
}

bool Playlist::add( unsigned int mediaId, unsigned int position )
{
    static const std::string req = "INSERT INTO PlaylistMediaRelation(media_id, playlist_id, position) VALUES(?, ?, ?)";
    // position isn't a foreign key, but we want it to be passed as NULL if it equals to 0
    // When the position is NULL, the insertion triggers takes care of counting the number of records to auto append.
    return sqlite::Tools::insert( m_dbConnection, req, mediaId, m_id, sqlite::ForeignKey{ position } );
}

bool Playlist::move( unsigned int mediaId, unsigned int position )
{
    if ( position == 0 )
        return false;
    static const std::string req = "UPDATE PlaylistMediaRelation SET position = ? WHERE "
            "playlist_id = ? AND media_id = ?";
    return sqlite::Tools::executeUpdate( m_dbConnection, req, position, m_id, mediaId );
}

bool Playlist::remove( unsigned int mediaId )
{
    static const std::string req = "DELETE FROM PlaylistMediaRelation WHERE playlist_id = ? AND media_id = ?";
    return sqlite::Tools::executeDelete( m_dbConnection, req, m_id, mediaId );
}

bool Playlist::createTable( DBConnection dbConn )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::PlaylistTable::Name + "("
            + policy::PlaylistTable::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT"
        ")";
    static const std::string relTableReq = "CREATE TABLE IF NOT EXISTS PlaylistMediaRelation("
            "media_id INTEGER,"
            "playlist_id INTEGER,"
            "position INTEGER,"
            "PRIMARY KEY(media_id, playlist_id),"
            "FOREIGN KEY(media_id) REFERENCES " + policy::MediaTable::Name + "("
                + policy::MediaTable::PrimaryKeyColumn + ") ON DELETE CASCADE,"
            "FOREIGN KEY(playlist_id) REFERENCES " + policy::PlaylistTable::Name + "("
                + policy::PlaylistTable::PrimaryKeyColumn + ") ON DELETE CASCADE"
        ")";
    //FIXME Enforce (playlist_id,position) uniqueness
    return sqlite::Tools::executeRequest( dbConn, req ) &&
            sqlite::Tools::executeRequest( dbConn, relTableReq );
}

bool Playlist::createTriggers( DBConnection dbConn )
{
    static const std::string req = "CREATE TRIGGER IF NOT EXISTS update_playlist_order AFTER UPDATE OF position"
            " ON PlaylistMediaRelation"
            " BEGIN "
                "UPDATE PlaylistMediaRelation SET position = position + 1"
                " WHERE playlist_id = new.playlist_id"
                " AND position = new.position"
                // We don't to trigger a self-update when the insert trigger fires.
                " AND media_id != new.media_id;"
            " END";
    static const std::string autoAppendReq = "CREATE TRIGGER IF NOT EXISTS append_new_playlist_record AFTER INSERT"
            " ON PlaylistMediaRelation"
            " WHEN new.position IS NULL"
            " BEGIN "
                " UPDATE PlaylistMediaRelation SET position = ("
                    "SELECT COUNT(media_id) FROM PlaylistMediaRelation WHERE playlist_id = new.playlist_id"
                ") WHERE playlist_id=new.playlist_id AND media_id = new.media_id;"
            " END";
    static const std::string autoShiftPosReq = "CREATE TRIGGER IF NOT EXISTS update_playlist_order_on_insert AFTER INSERT"
            " ON PlaylistMediaRelation"
            " WHEN new.position IS NOT NULL"
            " BEGIN "
                "UPDATE PlaylistMediaRelation SET position = position + 1"
                " WHERE playlist_id = new.playlist_id"
                " AND position = new.position"
                " AND media_id != new.media_id;"
            " END";
    return sqlite::Tools::executeRequest( dbConn, req ) &&
            sqlite::Tools::executeRequest( dbConn, autoAppendReq ) &&
            sqlite::Tools::executeRequest( dbConn, autoShiftPosReq );
}
