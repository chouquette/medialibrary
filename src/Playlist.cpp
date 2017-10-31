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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "Playlist.h"

#include "Media.h"

namespace medialibrary
{

namespace policy
{
const std::string PlaylistTable::Name = "Playlist";
const std::string PlaylistTable::PrimaryKeyColumn = "id_playlist";
int64_t Playlist::* const PlaylistTable::PrimaryKey = &Playlist::m_id;
}

Playlist::Playlist( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    row >> m_id
        >> m_name
        >> m_fileId
        >> m_creationDate
        >> m_artworkMrl;
}

Playlist::Playlist( MediaLibraryPtr ml, const std::string& name, int64_t fileId )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( name )
    , m_fileId( fileId )
    , m_creationDate( time( nullptr ) )
{
}

Playlist::Playlist( MediaLibraryPtr ml, const std::string& name)
    : Playlist( ml, name, 0 )
{
}

std::shared_ptr<Playlist> Playlist::create( MediaLibraryPtr ml, const std::string& name )
{
    return createFromFile( ml, name, 0 );
}

std::shared_ptr<Playlist> Playlist::createFromFile( MediaLibraryPtr ml, const std::string& name, int64_t fileId )
{
    auto self = std::make_shared<Playlist>( ml, name, fileId );
    static const std::string req = "INSERT INTO " + policy::PlaylistTable::Name + \
            "(name, file_id, creation_date, artwork_mrl) VALUES(?, ?, ?, ?)";
    try
    {
        if ( insert( ml, self, req, name, sqlite::ForeignKey( fileId ), self->m_creationDate, self->m_artworkMrl ) == false )
            return nullptr;
        return self;
    }
    catch( sqlite::errors::ConstraintViolation& ex )
    {
        LOG_WARN( "Failed to create playlist: ", ex.what() );
    }
    return nullptr;
}

int64_t Playlist::id() const
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
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, name, m_id ) == false )
        return false;
    m_name = name;
    return true;
}

unsigned int Playlist::creationDate() const
{
    return m_creationDate;
}

const std::string& Playlist::artworkMrl() const
{
    return m_artworkMrl;
}

std::vector<MediaPtr> Playlist::media() const
{
    static const std::string req = "SELECT m.* FROM " + policy::MediaTable::Name + " m "
            "LEFT JOIN PlaylistMediaRelation pmr ON pmr.media_id = m.id_media "
            "WHERE pmr.playlist_id = ? AND m.is_present = 1 "
            "ORDER BY pmr.position";
    return Media::fetchAll<IMedia>( m_ml, req, m_id );
}

bool Playlist::append( int64_t mediaId )
{
    return add( mediaId, 0 );
}

bool Playlist::add( int64_t mediaId, unsigned int position )
{
    static const std::string req = "INSERT INTO PlaylistMediaRelation(media_id, playlist_id, position) VALUES(?, ?, ?)";
    // position isn't a foreign key, but we want it to be passed as NULL if it equals to 0
    // When the position is NULL, the insertion triggers takes care of counting the number of records to auto append.
    try
    {
        return sqlite::Tools::executeInsert( m_ml->getConn(), req, mediaId, m_id, sqlite::ForeignKey{ position } );
    }
    catch (const sqlite::errors::ConstraintViolation& ex)
    {
        LOG_WARN( "Rejected playlist insertion: ", ex.what() );
        return false;
    }
}

bool Playlist::move( int64_t mediaId, unsigned int position )
{
    if ( position == 0 )
        return false;
    static const std::string req = "UPDATE PlaylistMediaRelation SET position = ? WHERE "
            "playlist_id = ? AND media_id = ?";
    return sqlite::Tools::executeUpdate( m_ml->getConn(), req, position, m_id, mediaId );
}

bool Playlist::remove( int64_t mediaId )
{
    static const std::string req = "DELETE FROM PlaylistMediaRelation WHERE playlist_id = ? AND media_id = ?";
    return sqlite::Tools::executeDelete( m_ml->getConn(), req, m_id, mediaId );
}

bool Playlist::createTable( DBConnection dbConn )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::PlaylistTable::Name + "("
            + policy::PlaylistTable::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT UNIQUE,"
            "file_id UNSIGNED INT DEFAULT NULL,"
            "creation_date UNSIGNED INT NOT NULL,"
            "artwork_mrl TEXT,"
            "FOREIGN KEY (file_id) REFERENCES " + policy::FileTable::Name
            + "(id_file) ON DELETE CASCADE"
        ")";
    const std::string relTableReq = "CREATE TABLE IF NOT EXISTS PlaylistMediaRelation("
            "media_id INTEGER,"
            "playlist_id INTEGER,"
            "position INTEGER,"
            "PRIMARY KEY(media_id, playlist_id),"
            "FOREIGN KEY(media_id) REFERENCES " + policy::MediaTable::Name + "("
                + policy::MediaTable::PrimaryKeyColumn + ") ON DELETE CASCADE,"
            "FOREIGN KEY(playlist_id) REFERENCES " + policy::PlaylistTable::Name + "("
                + policy::PlaylistTable::PrimaryKeyColumn + ") ON DELETE CASCADE"
        ")";
    const std::string vtableReq = "CREATE VIRTUAL TABLE IF NOT EXISTS "
                + policy::PlaylistTable::Name + "Fts USING FTS3("
                "name"
            ")";
    //FIXME Enforce (playlist_id,position) uniqueness
    return sqlite::Tools::executeRequest( dbConn, req ) &&
            sqlite::Tools::executeRequest( dbConn, relTableReq ) &&
            sqlite::Tools::executeRequest( dbConn, vtableReq );
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
    static const std::string vtriggerInsert = "CREATE TRIGGER IF NOT EXISTS insert_playlist_fts AFTER INSERT ON "
            + policy::PlaylistTable::Name +
            " BEGIN"
            " INSERT INTO " + policy::PlaylistTable::Name + "Fts(rowid, name) VALUES(new.id_playlist, new.name);"
            " END";
    static const std::string vtriggerUpdate = "CREATE TRIGGER IF NOT EXISTS update_playlist_fts AFTER UPDATE OF name"
            " ON " + policy::PlaylistTable::Name +
            " BEGIN"
            " UPDATE " + policy::PlaylistTable::Name + "Fts SET name = new.name WHERE rowid = new.id_playlist;"
            " END";
    static const std::string vtriggerDelete = "CREATE TRIGGER IF NOT EXISTS delete_playlist_fts BEFORE DELETE ON "
            + policy::PlaylistTable::Name +
            " BEGIN"
            " DELETE FROM " + policy::PlaylistTable::Name + "Fts WHERE rowid = old.id_playlist;"
            " END";
    return sqlite::Tools::executeRequest( dbConn, req ) &&
            sqlite::Tools::executeRequest( dbConn, autoAppendReq ) &&
            sqlite::Tools::executeRequest( dbConn, autoShiftPosReq ) &&
            sqlite::Tools::executeRequest( dbConn, vtriggerInsert ) &&
            sqlite::Tools::executeRequest( dbConn, vtriggerUpdate ) &&
            sqlite::Tools::executeRequest( dbConn, vtriggerDelete );
}

std::vector<PlaylistPtr> Playlist::search( MediaLibraryPtr ml, const std::string& name )
{
    static const std::string req = "SELECT * FROM " + policy::PlaylistTable::Name + " WHERE id_playlist IN "
            "(SELECT rowid FROM " + policy::PlaylistTable::Name + "Fts WHERE name MATCH '*' || ? || '*')";
    return fetchAll<IPlaylist>( ml, req, name );
}

std::vector<PlaylistPtr> Playlist::listAll( MediaLibraryPtr ml, SortingCriteria sort, bool desc )
{
    std::string req = "SELECT * FROM " + policy::PlaylistTable::Name + " ORDER BY ";
    switch ( sort )
    {
    case SortingCriteria::InsertionDate:
        req += "creation_date";
        break;
    default:
        req += "name";
        break;
    }
    if ( desc == true )
        req += " DESC";
    return fetchAll<IPlaylist>( ml, req );
}

}
