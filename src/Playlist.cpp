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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "Playlist.h"

#include "Media.h"
#include "utils/ModificationsNotifier.h"
#include "database/SqliteQuery.h"

#include <algorithm>

namespace medialibrary
{

const std::string Playlist::Table::Name = "Playlist";
const std::string Playlist::Table::PrimaryKeyColumn = "id_playlist";
int64_t Playlist::* const Playlist::Table::PrimaryKey = &Playlist::m_id;
const std::string Playlist::FtsTable::Name = "PlaylistFts";
const std::string Playlist::MediaRelationTable::Name = "PlaylistMediaRelation";


Playlist::Playlist( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_name( row.extract<decltype(m_name)>() )
    , m_fileId( row.extract<decltype(m_fileId)>() )
    , m_creationDate( row.extract<decltype(m_creationDate)>() )
    , m_artworkMrl( row.extract<decltype(m_artworkMrl)>() )
{
    assert( row.hasRemainingColumns() == false );
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
    static const std::string req = "INSERT INTO " + Playlist::Table::Name +
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
    static const std::string req = "UPDATE " + Playlist::Table::Name + " SET name = ? WHERE id_playlist = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, name, m_id ) == false )
        return false;
    m_name = name;

    auto notifier = m_ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyPlaylistModification( shared_from_this() );
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
    static const std::string base = "FROM " + Media::Table::Name + " m "
        "LEFT JOIN " + Playlist::MediaRelationTable::Name + " pmr ON pmr.media_id = m.id_media "
        "WHERE pmr.playlist_id = ? AND m.is_present != 0";
    static const std::string req = "SELECT m.* " + base + " ORDER BY pmr.position";
    static const std::string countReq = "SELECT COUNT(*) " + base;
    curateNullMediaID();
    return make_query_with_count<Media, IMedia>( m_ml, countReq, req, m_id );
}

Query<IMedia> Playlist::searchMedia( const std::string& pattern,
                                     const QueryParameters* params ) const
{
    if ( pattern.size() < 3 )
        return {};
    curateNullMediaID();
    return Media::searchInPlaylist( m_ml, pattern, m_id, params );
}

void Playlist::curateNullMediaID() const
{
    auto dbConn = m_ml->getConn();
    auto t = dbConn->newTransaction();
    std::string req = "SELECT rowid, mrl FROM " + Playlist::MediaRelationTable::Name + " "
            "WHERE media_id IS NULL "
            "AND playlist_id = ?";
    sqlite::Statement stmt{ dbConn->handle(), req };
    stmt.execute( m_id );
    std::string updateReq = "UPDATE " + Playlist::MediaRelationTable::Name +
            " SET media_id = ? WHERE rowid = ?";
    auto mediaNotFound = false;

    for ( sqlite::Row row = stmt.row(); row != nullptr; row = stmt.row() )
    {
        int64_t rowId;
        std::string mrl;
        row >> rowId >> mrl;
        auto media = m_ml->media( mrl );
        if ( media != nullptr )
        {
            LOG_INFO( "Updating playlist item mediaId (playlist: ", m_id,
                      "; mrl: ", mrl, ')' );
            sqlite::Tools::executeUpdate( dbConn, updateReq, media->id(), rowId );
        }
        else
        {
            LOG_INFO( "Can't restore media association for media ", mrl,
                      " in playlist ", m_id, ". Media will be removed from the playlist" );
            mediaNotFound = true;
        }
    }
    if ( mediaNotFound )
    {
        // Batch all deletion at once instead of doing it during the loop
        std::string deleteReq = "DELETE FROM " + Playlist::MediaRelationTable::Name + " "
                "WHERE playlist_id = ? AND media_id IS NULL";
        sqlite::Tools::executeDelete( dbConn, deleteReq, m_id );
    }
    t->commit();
}

bool Playlist::append( const IMedia& media )
{
    return add( media, UINT32_MAX );
}

bool Playlist::add( const IMedia& media, uint32_t position )
{
    auto files = media.files();
    assert( files.size() > 0 );
    auto mainFile = std::find_if( begin( files ), end( files ), []( const FilePtr& f) {
        return f->isMain();
    });
    if ( mainFile == end( files ) )
    {
        LOG_ERROR( "Can't add a media without any files to a playlist" );
        return false;
    }
    if ( position == UINT32_MAX )
    {
        static const std::string req = "INSERT INTO " + Playlist::MediaRelationTable::Name +
                "(media_id, mrl, playlist_id, position) VALUES(?1, ?2, ?3,"
                "(SELECT COUNT(media_id) FROM " + Playlist::MediaRelationTable::Name +
                " WHERE playlist_id = ?3))";
        if ( sqlite::Tools::executeInsert( m_ml->getConn(), req, media.id(),
                                           (*mainFile)->mrl(), m_id ) == false )
            return false;
    }
    else
    {
        static const std::string req = "INSERT INTO " + Playlist::MediaRelationTable::Name + " "
                "(media_id, mrl, playlist_id, position) VALUES(?1, ?2, ?3,"
                "min(?4, (SELECT COUNT(media_id) FROM " + Playlist::MediaRelationTable::Name +
                " WHERE playlist_id = ?3)))";
        if ( sqlite::Tools::executeInsert( m_ml->getConn(), req, media.id(),
                                           (*mainFile)->mrl(), m_id, position ) == false )
            return false;
    }
    auto notifier = m_ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyPlaylistModification( shared_from_this() );
    return true;
}

bool Playlist::append( int64_t mediaId )
{
    auto media = m_ml->media( mediaId );
    if ( media == nullptr )
        return false;
    return append( *media );
}

bool Playlist::add( int64_t mediaId, uint32_t position )
{
    auto media = m_ml->media( mediaId );
    if ( media == nullptr )
        return false;
    return add( *media, position );
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
    static const std::string req = "UPDATE " + Playlist::Table::Name + " SET file_id = ? WHERE id_playlist = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, file->id(), m_id ) == false )
        return nullptr;
    m_fileId = file->id();
    return file;
}

bool Playlist::contains( int64_t mediaId, uint32_t position )
{
    static const std::string req = "SELECT COUNT(media_id) FROM "
            + Playlist::MediaRelationTable::Name +
            " WHERE media_id = ? AND playlist_id = ? AND position = ?";
    uint32_t count;
    auto dbConn = m_ml->getConn();
    {
        auto ctx = dbConn->acquireReadContext();
        auto chrono = std::chrono::steady_clock::now();
        sqlite::Statement stmt( dbConn->handle(), req );
        stmt.execute( mediaId, m_id, position );
        auto duration = std::chrono::steady_clock::now() - chrono;
        LOG_VERBOSE("Executed ", req, " in ",
                 std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
        auto row = stmt.row();
        row >> count;
    }
    return count != 0;
}

bool Playlist::move( uint32_t from, uint32_t position )
{
    auto dbConn = m_ml->getConn();

    /*
     * We can't have triggers that update position during insertion and trigger
     * that update after modifying the position, as they would fire and wreck
     * the expected results.
     * To work around this, we delete the previous record, and insert it again.
     * However to do so, we need to fetch the media ID at the previous location.
     */
    auto t = dbConn->newTransaction();
    const std::string fetchReq = "SELECT media_id FROM " +
            Playlist::MediaRelationTable::Name +
            " WHERE playlist_id = ? AND position = ?";
    sqlite::Statement stmt( dbConn->handle(), fetchReq );
    int64_t mediaId;
    stmt.execute( m_id, from );
    {
        auto row = stmt.row();
        if ( row == nullptr )
        {
            LOG_ERROR( "Failed to find an item at position ", from, " in playlist" );
            return false;
        }
        row >> mediaId;
    }
    if ( remove( from ) == false )
    {
        LOG_ERROR( "Failed to remove element ", from, " from playlist" );
        return false;
    }
    if ( add( mediaId, position ) == false )
    {
        LOG_ERROR( "Failed to re-add element in playlist" );
        return false;
    }
    t->commit();

    auto notifier = m_ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyPlaylistModification( shared_from_this() );
    return true;
}

bool Playlist::remove( uint32_t position )
{
    static const std::string req = "DELETE FROM " +
            Playlist::MediaRelationTable::Name +
            " WHERE playlist_id = ? AND position = ?";
    if ( sqlite::Tools::executeDelete( m_ml->getConn(), req, m_id, position ) == false )
        return false;
    auto notifier = m_ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyPlaylistModification( shared_from_this() );
    return true;
}

void Playlist::createTable( sqlite::Connection* dbConn )
{
    std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
        schema( FtsTable::Name, Settings::DbModelVersion ),
        schema( MediaRelationTable::Name, Settings::DbModelVersion )
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );
}

void Playlist::createTriggers( sqlite::Connection* dbConn, uint32_t dbModel )
{
    std::string reqs[] = {
        #include "database/tables/Playlist_triggers_v16.sql"
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );
    // Playlist doesn't have an mrl field before version 14, so we must not
    // create the trigger before migrating to that version
    if ( dbModel >= 14 )
    {
        sqlite::Tools::executeRequest( dbConn, "CREATE INDEX IF NOT EXISTS "
            "playlist_file_id ON " + Playlist::Table::Name + "(file_id)" );
    }
    else
    {
        sqlite::Tools::executeRequest( dbConn,
            "CREATE INDEX IF NOT EXISTS playlist_media_pl_id_index "
            "ON " + Playlist::MediaRelationTable::Name + "(media_id, playlist_id)" );
    }
}

std::string Playlist::schema( const std::string& tableName, uint32_t )
{
    if ( tableName == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE IF NOT EXISTS " + FtsTable::Name +
               " USING FTS3(name)";
    }
    else if ( tableName == Table::Name )
    {
        return "CREATE TABLE IF NOT EXISTS " + Table::Name +
        "("
            + Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT COLLATE NOCASE,"
            "file_id UNSIGNED INT DEFAULT NULL,"
            "creation_date UNSIGNED INT NOT NULL,"
            "artwork_mrl TEXT,"
            "FOREIGN KEY(file_id) REFERENCES " + File::Table::Name
            + "(id_file) ON DELETE CASCADE"
        ")";
    }
    assert( tableName == MediaRelationTable::Name );
    return "CREATE TABLE IF NOT EXISTS " + MediaRelationTable::Name +
    "("
        "media_id INTEGER,"
        "mrl STRING,"
        "playlist_id INTEGER,"
        "position INTEGER,"
        "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name + "("
            + Media::Table::PrimaryKeyColumn + ") ON DELETE SET NULL,"
        "FOREIGN KEY(playlist_id) REFERENCES " + Playlist::Table::Name + "("
            + Playlist::Table::PrimaryKeyColumn + ") ON DELETE CASCADE"
    ")";
}

Query<IPlaylist> Playlist::search( MediaLibraryPtr ml, const std::string& name,
                                   const QueryParameters* params )
{
    std::string req = "FROM " + Playlist::Table::Name + " WHERE id_playlist IN "
            "(SELECT rowid FROM " + FtsTable::Name + " WHERE name MATCH ?)";
    return make_query<Playlist, IPlaylist>( ml, "*", std::move( req ),
                                            sortRequest( params ),
                                            sqlite::Tools::sanitizePattern( name ) );
}

Query<IPlaylist> Playlist::listAll( MediaLibraryPtr ml, const QueryParameters* params )
{
    std::string req = "FROM " + Playlist::Table::Name;
    return make_query<Playlist, IPlaylist>( ml, "*", std::move( req ),
                                            sortRequest( params ) );
}

void Playlist::clearExternalPlaylistContent(MediaLibraryPtr ml)
{
    // We can't delete all external playlist as such, since this would cause the
    // deletion of the associated task through the Task.playlist_id Playlist.id_playlist
    // foreign key, and therefor they wouldn't be rescanned.
    // Instead, flush the playlist content.
    const std::string req = "DELETE FROM " + Playlist::MediaRelationTable::Name +
            " WHERE playlist_id IN ("
            "SELECT id_playlist FROM " + Playlist::Table::Name + " WHERE "
            "file_id IS NOT NULL)";
    sqlite::Tools::executeDelete( ml->getConn(), req );
}

void Playlist::clearContent()
{
    const std::string req = "DELETE FROM " + Playlist::MediaRelationTable::Name +
            " WHERE playlist_id = ?";
    sqlite::Tools::executeDelete( m_ml->getConn(), req, m_id );
}

std::shared_ptr<Playlist> Playlist::fromFile( MediaLibraryPtr ml, int64_t fileId )
{
    static const std::string req = "SELECT * FROM " + Table::Name +
            " WHERE file_id = ?";
    return fetch( ml, req, fileId );
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
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default (Alpha)" );
        /* fall-through */
    case SortingCriteria::Default:
    case SortingCriteria::Alpha:
        req += "name";
        break;
    }
    if ( params != nullptr && params->desc == true )
        req += " DESC";
    return req;
}

}
