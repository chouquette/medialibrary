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

#include "database/SqliteQuery.h"

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

Playlist::Playlist( MediaLibraryPtr ml, const std::string& name )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( name )
    , m_fileId( 0 )
    , m_creationDate( time( nullptr ) )
{
}

std::shared_ptr<Playlist> Playlist::create( MediaLibraryPtr ml, const std::string& name )
{
    auto self = std::make_shared<Playlist>( ml, name );
    static const std::string req = "INSERT INTO " + policy::PlaylistTable::Name +
            "(name, file_id, creation_date, artwork_mrl) VALUES(?, ?, ?, ?)";
    try
    {
        if ( insert( ml, self, req, name, nullptr, self->m_creationDate, self->m_artworkMrl ) == false )
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

Query<IMedia> Playlist::media() const
{
    static const std::string req = "FROM " + policy::MediaTable::Name + " m "
            "LEFT JOIN PlaylistMediaRelation pmr ON pmr.media_id = m.id_media "
            "WHERE pmr.playlist_id = ? AND m.is_present != 0 "
            "ORDER BY pmr.position";
    return make_query<Media, IMedia>( m_ml, "m.*", req, m_id );
}

Query<IMedia> Playlist::searchMedia( const std::string& pattern,
                                     const QueryParameters* params ) const
{
    return Media::searchInPlaylist( m_ml, pattern, m_id, params );
}

bool Playlist::append( const IMedia& media )
{
    return add( media, 0 );
}

bool Playlist::add( const IMedia& media, unsigned int position )
{
    static const std::string req = "INSERT INTO PlaylistMediaRelation(media_id, playlist_id, position) VALUES(?, ?, ?)";
    // position isn't a foreign key, but we want it to be passed as NULL if it equals to 0
    // When the position is NULL, the insertion triggers takes care of counting the number of records to auto append.
    try
    {
        return sqlite::Tools::executeInsert( m_ml->getConn(), req, media.id(), m_id, sqlite::ForeignKey{ position } );
    }
    catch (const sqlite::errors::ConstraintViolation& ex)
    {
        LOG_WARN( "Rejected playlist insertion: ", ex.what() );
        return false;
    }
}

// Attach file object to Playlist
std::shared_ptr<File> Playlist::addFile( const fs::IFile& fileFs, int64_t parentFolderId,
                                         bool isFolderFsRemovable )
{
    assert( m_fileId == 0 );
    assert( sqlite::Transaction::transactionInProgress() == true );

    auto file = File::createFromPlaylist( m_ml, m_id, fileFs, parentFolderId, isFolderFsRemovable);
    if ( file == nullptr )
        return nullptr;
    static const std::string req = "UPDATE " + policy::PlaylistTable::Name + " SET file_id = ? WHERE id_playlist = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, file->id(), m_id ) == false )
        return nullptr;
    m_fileId = file->id();
    return file;
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

void Playlist::createTable( sqlite::Connection* dbConn )
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
            "FOREIGN KEY(media_id) REFERENCES " + policy::MediaTable::Name + "("
                + policy::MediaTable::PrimaryKeyColumn + ") ON DELETE CASCADE,"
            "FOREIGN KEY(playlist_id) REFERENCES " + policy::PlaylistTable::Name + "("
                + policy::PlaylistTable::PrimaryKeyColumn + ") ON DELETE CASCADE"
        ")";
    const std::string indexReq = "CREATE INDEX IF NOT EXISTS playlist_media_pl_id_index "
            "ON PlaylistMediaRelation(media_id, playlist_id)";

    const std::string vtableReq = "CREATE VIRTUAL TABLE IF NOT EXISTS "
                + policy::PlaylistTable::Name + "Fts USING FTS3("
                "name"
            ")";
    //FIXME Enforce (playlist_id,position) uniqueness
    sqlite::Tools::executeRequest( dbConn, req );
    sqlite::Tools::executeRequest( dbConn, relTableReq );
    sqlite::Tools::executeRequest( dbConn, indexReq );
    sqlite::Tools::executeRequest( dbConn, vtableReq );
}

void Playlist::createTriggers( sqlite::Connection* dbConn )
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
    sqlite::Tools::executeRequest( dbConn, req );
    sqlite::Tools::executeRequest( dbConn, autoAppendReq );
    sqlite::Tools::executeRequest( dbConn, autoShiftPosReq );
    sqlite::Tools::executeRequest( dbConn, vtriggerInsert );
    sqlite::Tools::executeRequest( dbConn, vtriggerUpdate );
    sqlite::Tools::executeRequest( dbConn, vtriggerDelete );
}

Query<IPlaylist> Playlist::search( MediaLibraryPtr ml, const std::string& name,
                                   const QueryParameters* params )
{
    std::string req = "FROM " + policy::PlaylistTable::Name + " WHERE id_playlist IN "
            "(SELECT rowid FROM " + policy::PlaylistTable::Name + "Fts WHERE name MATCH '*' || ? || '*')";
    req += sortRequest( params );
    return make_query<Playlist, IPlaylist>( ml, "*", req, name );
}

Query<IPlaylist> Playlist::listAll( MediaLibraryPtr ml, const QueryParameters* params )
{
    std::string req = "FROM " + policy::PlaylistTable::Name;
    req += sortRequest( params );
    return make_query<Playlist, IPlaylist>( ml, "*", req );
}

void Playlist::clearExternalPlaylistContent(MediaLibraryPtr ml)
{
    // We can't delete all external playlist as such, since this would cause the
    // deletion of the associated task through the Task.playlist_id Playlist.id_playlist
    // foreign key, and therefor they wouldn't be rescanned.
    // Instead, flush the playlist content.
    const std::string req = "DELETE FROM PlaylistMediaRelation WHERE playlist_id IN ("
            "SELECT id_playlist FROM " + policy::PlaylistTable::Name + " WHERE "
            "file_id IS NOT NULL)";
    sqlite::Tools::executeDelete( ml->getConn(), req );
}

std::string Playlist::sortRequest( const QueryParameters* params )
{
    std::string req = " ORDER BY ";
    SortingCriteria sort = params != nullptr ? params->sort : SortingCriteria::Default;
    switch ( sort )
    {
    case SortingCriteria::InsertionDate:
        req += "creation_date";
        break;
    default:
        req += "name";
        break;
    }
    if ( params != nullptr && params->desc == true )
        req += " DESC";
    return req;
}

}
