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

#include "Device.h"
#include "File.h"
#include "Folder.h"
#include "Media.h"
#include "utils/Filename.h"
#include "utils/Directory.h"
#include "utils/Url.h"
#include "database/SqliteQuery.h"
#include "medialibrary/filesystem/Errors.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/IDevice.h"

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
            if ( sqlite::Tools::executeUpdate( dbConn, updateReq, media->id(),
                                               rowId ) == false )
            {
                LOG_WARN( "Failed to currate NULL media_id from playlist" );
                return;
            }
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
        if ( sqlite::Tools::executeDelete( dbConn, deleteReq, m_id ) == false )
        {
            LOG_WARN( "Failed to remove remaining NULL media_id from playlist" );
            return;
        }
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
        return sqlite::Tools::executeInsert( m_ml->getConn(), req, media.id(),
                                             (*mainFile)->mrl(), m_id );
    }
    static const std::string req = "INSERT INTO " + Playlist::MediaRelationTable::Name + " "
            "(media_id, mrl, playlist_id, position) VALUES(?1, ?2, ?3,"
            "min(?4, (SELECT COUNT(media_id) FROM " + Playlist::MediaRelationTable::Name +
            " WHERE playlist_id = ?3)))";
    return sqlite::Tools::executeInsert( m_ml->getConn(), req, media.id(),
                                       (*mainFile)->mrl(), m_id, position );
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

    return true;
}

bool Playlist::remove( uint32_t position )
{
    static const std::string req = "DELETE FROM " +
            Playlist::MediaRelationTable::Name +
            " WHERE playlist_id = ? AND position = ?";
    return sqlite::Tools::executeDelete( m_ml->getConn(), req, m_id, position );
}

bool Playlist::isReadOnly() const
{
    return m_fileId != 0;
}

std::string Playlist::mrl() const
{
    auto file = File::fetch( m_ml, m_fileId );
    if ( file == nullptr )
        return {};
    try
    {
        return file->mrl();
    }
    catch ( const fs::errors::DeviceRemoved& )
    {
        return {};
    }
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
    if ( dbModel < 16 )
    {
        sqlite::Tools::executeRequest( dbConn,
                                       trigger( Triggers::UpdateOrderOnInsert, dbModel ) );
        sqlite::Tools::executeRequest( dbConn,
                                       trigger( Triggers::Append, dbModel ) );
        sqlite::Tools::executeRequest( dbConn,
                                       trigger( Triggers::UpdateOrderOnPositionUpdate, dbModel ) );
    }
    else
    {
        sqlite::Tools::executeRequest( dbConn,
                                       trigger( Triggers::UpdateOrderOnInsert, dbModel ) );
        sqlite::Tools::executeRequest( dbConn,
                                       trigger( Triggers::UpdateOrderOnDelete, dbModel ) );
    }
    sqlite::Tools::executeRequest( dbConn,
                                   trigger( Triggers::InsertFts, dbModel ) );
    sqlite::Tools::executeRequest( dbConn,
                                   trigger( Triggers::UpdateFts, dbModel ) );
    sqlite::Tools::executeRequest( dbConn,
                                   trigger( Triggers::DeleteFts, dbModel ) );
}

void Playlist::createIndexes( sqlite::Connection* dbConn, uint32_t dbModel )
{
    sqlite::Tools::executeRequest( dbConn, index( Indexes::FileId, dbModel ) );
    if ( dbModel >= 16 )
        sqlite::Tools::executeRequest( dbConn, index( Indexes::PlaylistIdPosition, dbModel ) );
}

std::string Playlist::schema( const std::string& tableName, uint32_t )
{
    if ( tableName == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE " + FtsTable::Name +
               " USING FTS3(name)";
    }
    else if ( tableName == Table::Name )
    {
        return "CREATE TABLE " + Table::Name +
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
    return "CREATE TABLE " + MediaRelationTable::Name +
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

std::string Playlist::trigger( Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::UpdateOrderOnInsert:
        {
            if ( dbModel < 16 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                            " AFTER INSERT ON " + MediaRelationTable::Name +
                       " WHEN new.position IS NOT NULL"
                       " BEGIN "
                           "UPDATE " + MediaRelationTable::Name +
                           " SET position = position + 1"
                           " WHERE playlist_id = new.playlist_id"
                           " AND position = new.position"
                           " AND media_id != new.media_id;"
                       " END";
            }
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER INSERT ON " + MediaRelationTable::Name +
                   " WHEN new.position IS NOT NULL"
                   " BEGIN "
                       "UPDATE " + MediaRelationTable::Name +
                       " SET position = position + 1"
                       " WHERE playlist_id = new.playlist_id"
                       " AND position >= new.position"
                       " AND rowid != new.rowid;"
                   " END";
        }
        case Triggers::UpdateOrderOnDelete:
        {
            assert( dbModel >= 16 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER DELETE ON " + MediaRelationTable::Name +
                   " BEGIN "
                       "UPDATE " + MediaRelationTable::Name +
                       " SET position = position - 1"
                       " WHERE playlist_id = old.playlist_id"
                       " AND position > old.position;"
                   " END";
        }
        case Triggers::InsertFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER INSERT ON " + Table::Name +
                   " BEGIN"
                       " INSERT INTO " + FtsTable::Name + "(rowid, name)"
                            " VALUES(new.id_playlist, new.name);"
                   " END";
        case Triggers::UpdateFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER UPDATE OF name ON " + Table::Name +
                   " BEGIN"
                       " UPDATE " + FtsTable::Name + " SET name = new.name"
                            " WHERE rowid = new.id_playlist;"
                   " END";
        case Triggers::DeleteFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " BEFORE DELETE ON " + Table::Name +
                   " BEGIN"
                        " DELETE FROM " + FtsTable::Name +
                            " WHERE rowid = old.id_playlist;"
                   " END";
        case Triggers::Append:
        {
            assert( dbModel <= 15 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   "AFTER INSERT ON " + MediaRelationTable::Name +
                   " WHEN new.position IS NULL"
                   " BEGIN "
                       " UPDATE " + MediaRelationTable::Name + " SET position = ("
                           "SELECT COUNT(media_id) FROM " + MediaRelationTable::Name +
                           " WHERE playlist_id = new.playlist_id"
                       ") WHERE playlist_id=new.playlist_id AND media_id = new.media_id;"
                   " END";
        }
        case Triggers::UpdateOrderOnPositionUpdate:
        {
            assert( dbModel <= 15 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER UPDATE OF position"
                   " ON " + MediaRelationTable::Name +
                   " BEGIN "
                       "UPDATE " + MediaRelationTable::Name +
                       " SET position = position + 1"
                       " WHERE playlist_id = new.playlist_id"
                       " AND position = new.position"
                       // We don't want to trigger a self-update when the insert trigger fires.
                       " AND media_id != new.media_id;"
                   " END";
        }
        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Playlist::triggerName(Playlist::Triggers trigger, uint32_t dbModel)
{
    switch ( trigger )
    {
        case Triggers::UpdateOrderOnInsert:
        {
            if ( dbModel < 16 )
                return "update_playlist_order_on_insert";
            return "update_playlist_order_on_insert";
        }
        case Triggers::UpdateOrderOnDelete:
        {
            assert( dbModel >= 16 );
            return "update_playlist_order_on_delete";
        }
        case Triggers::InsertFts:
            return "insert_playlist_fts";
        case Triggers::UpdateFts:
            return "update_playlist_fts";
        case Triggers::DeleteFts:
            return "delete_playlist_fts";
        case Triggers::Append:
        {
            assert( dbModel <= 15 );
            return "append_new_playlist_record";
        }
        case Triggers::UpdateOrderOnPositionUpdate:
        {
            assert( dbModel <= 15 );
            return "update_playlist_order";
        }
        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Playlist::index( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::FileId:
        {
            if ( dbModel < 14 )
            {
                return "CREATE INDEX " + indexName( index, dbModel ) +
                       " ON " + MediaRelationTable::Name + "(media_id, playlist_id)";
            }
            return "CREATE INDEX " + indexName( index, dbModel ) +
                        " ON " + Table::Name + "(file_id)";
        }
        case Indexes::PlaylistIdPosition:
        {
            assert( dbModel >= 16 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                   " ON " + MediaRelationTable::Name + "(playlist_id,position)";
        }

        default:
            assert( !"Invalid index provided" );
    }
    return "<invalid request>";
}

std::string Playlist::indexName( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::FileId:
        {
            if ( dbModel < 14 )
            {
                return "playlist_media_pl_id_index";
            }
            return "playlist_file_id";
        }
        case Indexes::PlaylistIdPosition:
        {
            assert( dbModel >= 16 );
            return "playlist_position_pl_id_index";
        }

        default:
            assert( !"Invalid index provided" );
    }
    return "<invalid request>";
}

bool Playlist::checkDbModel(MediaLibraryPtr ml)
{
    if ( sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) == false ||
           sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( FtsTable::Name, Settings::DbModelVersion ),
                                       FtsTable::Name ) == false ||
           sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( MediaRelationTable::Name, Settings::DbModelVersion ),
                                       MediaRelationTable::Name ) == false )
        return false;

    auto checkTrigger = []( sqlite::Connection* dbConn, Triggers t ) {
        return sqlite::Tools::checkTriggerStatement( dbConn,
                                    trigger( t, Settings::DbModelVersion ),
                                    triggerName( t, Settings::DbModelVersion ) );
    };

    auto checkIndex = []( sqlite::Connection* dbConn, Indexes i ) {
        return sqlite::Tools::checkIndexStatement( dbConn,
                                    index( i, Settings::DbModelVersion ),
                                    indexName( i, Settings::DbModelVersion ) );
    };

    return checkTrigger( ml->getConn(), Triggers::UpdateOrderOnInsert ) &&
            checkTrigger( ml->getConn(), Triggers::UpdateOrderOnDelete ) &&
            checkTrigger( ml->getConn(), Triggers::InsertFts ) &&
            checkTrigger( ml->getConn(), Triggers::UpdateFts ) &&
            checkTrigger( ml->getConn(), Triggers::DeleteFts ) &&
            checkIndex( ml->getConn(), Indexes::FileId ) &&
            checkIndex( ml->getConn(), Indexes::PlaylistIdPosition );
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

bool Playlist::clearExternalPlaylistContent(MediaLibraryPtr ml)
{
    // We can't delete all external playlist as such, since this would cause the
    // deletion of the associated task through the Task.playlist_id Playlist.id_playlist
    // foreign key, and therefor they wouldn't be rescanned.
    // Instead, flush the playlist content.
    const std::string req = "DELETE FROM " + Playlist::MediaRelationTable::Name +
            " WHERE playlist_id IN ("
            "SELECT id_playlist FROM " + Playlist::Table::Name + " WHERE "
            "file_id IS NOT NULL)";
    return sqlite::Tools::executeDelete( ml->getConn(), req );
}

bool Playlist::clearContent()
{
    const std::string req = "DELETE FROM " + Playlist::MediaRelationTable::Name +
            " WHERE playlist_id = ?";
    return sqlite::Tools::executeDelete( m_ml->getConn(), req, m_id );
}

Playlist::Backups Playlist::loadBackups( MediaLibraryPtr ml )
{
    auto playlistFolderMrl = utils::file::toMrl( ml->playlistPath() );
    auto fsFactory = ml->fsFactoryForMrl( playlistFolderMrl );
    Backups backups;

    try
    {
        auto plFolder = fsFactory->createDirectory( playlistFolderMrl );
        std::vector<std::shared_ptr<fs::IFile>> files;
        for ( const auto& folder : plFolder->dirs() )
        {
            std::vector<std::string> mrls;
            for ( const auto& f : folder->files() )
                mrls.push_back( f->mrl() );
            auto backupDate = std::stol( utils::file::directoryName( folder->mrl() ) );
            backups.emplace( backupDate, std::move( mrls ) );
        }
    }
    catch ( const fs::errors::System& ex )
    {
        LOG_ERROR( "Failed to list old playlist backups: ", ex.what() );
    }
    return backups;
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

std::tuple<bool, time_t, std::vector<std::string>>
Playlist::backupPlaylists( MediaLibraryPtr ml, uint32_t dbModel )
{
    /* We can't use the Playlist class directly for this, as it's tied with the
     * current database model, and we're trying to run this before a
     * migration, meaning we'd be using the old database model.
     * Instead, we have to pull the mrl by hand, and generate a simple playlist
     * with that
     */
    auto backupDate = time( nullptr );
    auto dbConn = ml->getConn();
    auto ctx = dbConn->acquireReadContext();
    struct Backup
    {
        Backup( int64_t id, std::string name ) : id(id), name(std::move(name)) {}
        int64_t id;
        std::string name;
        std::vector<std::string> mrls;
    };
    std::vector<Backup> pls;
    // There was no file_id field before model 5
    sqlite::Statement stmt{ dbConn->handle(),
        "SELECT id_playlist, name FROM " + Table::Name +
        (dbModel >= 5 ? " WHERE file_id IS NULL" : "")
    };
    stmt.execute();
    sqlite::Row row;
    while ( ( row = stmt.row() ) != nullptr )
        pls.push_back( { row.load<int64_t>( 0 ), row.load<std::string>( 1 ) } );

    auto backupFolder = utils::file::toFolderPath( ml->playlistPath() +
                                                   std::to_string( backupDate ) );
    try
    {
        // This sucks badly, isDirectory will throw if the directory doesn't exist
        // which is what we want, but in case it returns false, it means the
        // playlist backup folder is actually a file, so we definitely don't
        // want to do anything with it.
        // Baiscally, we *want* isDirectory to throw.
        utils::fs::isDirectory( backupFolder );
        return std::make_tuple( false, 0, std::vector<std::string>{} );
    }
    catch ( const fs::errors::System&  )
    {
        // isDirectory will throw if the directory doesn't exist, so we can just proceed
    }

    utils::fs::mkdir( backupFolder );
    std::vector<std::string> outputFiles;

    auto res = true;
    for ( auto& pl : pls )
    {
        /* We can't simply fetch the mrls from the MediaRelation table, since this
         * wouldn't work for media on removable devices.
         * If we find out that the file is not removable, then we don't need the device
         */
        stmt = sqlite::Statement{ dbConn->handle(),
            "SELECT f.mrl, f.is_removable, fo.path, d.uuid, d.scheme FROM " + File::Table::Name + " f"
            " INNER JOIN " + MediaRelationTable::Name + " pmr"
                " ON f.media_id = pmr.media_id"
            " LEFT JOIN " + Folder::Table::Name + " fo"
                " ON fo.id_folder = f.folder_id"
            " LEFT JOIN " + Device::Table::Name + " d"
                " ON d.id_device = fo.device_id"
            " WHERE pmr.playlist_id = ?"
                " AND f.type = ?"
            " ORDER BY pmr.position "
        };
        stmt.execute( pl.id, File::Type::Main );
        while ( ( row = stmt.row() ) != nullptr )
        {
            auto mrl = row.extract<std::string>();
            auto isRemovable = row.extract<bool>();
            auto folderPath = row.extract<std::string>();
            auto uuid = row.extract<std::string>();
            auto scheme = row.extract<std::string>();
            if ( isRemovable == true )
            {
                auto fsFactory = ml->fsFactoryForMrl( scheme );
                if ( fsFactory == nullptr )
                    continue;
                auto device = fsFactory->createDevice( uuid );
                if ( device == nullptr )
                    continue;
                mrl = device->mountpoint() + folderPath + mrl;
            }
            // account for potential leftovers & badly encoded mrls
            mrl = utils::url::decode( mrl );
            mrl = utils::url::encode( mrl );
            pl.mrls.push_back( std::move( mrl ) );
        }
        if ( pl.mrls.empty() == true )
            continue;
        auto output = backupFolder + std::to_string( pl.id ) + ".xspf";
        res = writeBackup( pl.name, pl.mrls, output ) && res;
        outputFiles.push_back( utils::file::toMrl( output ) );
    }
    return std::make_tuple( res, backupDate, std::move( outputFiles ) );
}

bool Playlist::writeBackup( const std::string& name,
                            const std::vector<std::string>& mrls,
                            const std::string& destFile )
{
    auto file = std::unique_ptr<FILE, decltype(&fclose)>{
        fopen( destFile.c_str(), "w" ), &fclose
    };
    if ( file == nullptr )
        return false;

    std::string doc{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n"
    };
    doc += "<title>" + name + "</title>\n<trackList>\n";
    for ( const auto mrl : mrls )
        doc += "<track><location>" + mrl + "</location></track>\n";
    doc += "</trackList>\n</playlist>";

    auto buff = doc.c_str();
    auto length = doc.size();
    auto i = 0u;
    constexpr auto ChunkSize = 4096u;
    while ( i < length )
    {
        auto remaining = length - i;
        auto nmemb = remaining <= ChunkSize ? remaining : ChunkSize;
        auto res = fwrite( buff + i, sizeof( *buff ), nmemb, file.get() );
        i += res;
    }
    return true;
}

}
