/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Thumbnail.h"
#include "utils/File.h"
#include "utils/Filename.h"
#include "Album.h"
#include "Artist.h"
#include "Media.h"
#include "medialibrary/filesystem/Errors.h"

namespace medialibrary
{

const std::string Thumbnail::Table::Name = "Thumbnail";
const std::string Thumbnail::Table::PrimaryKeyColumn = "id_thumbnail";
int64_t Thumbnail::*const Thumbnail::Table::PrimaryKey = &Thumbnail::m_id;
const std::string Thumbnail::LinkingTable::Name = "ThumbnailLinking";

const std::string Thumbnail::EmptyMrl;

Thumbnail::Thumbnail( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_mrl( row.extract<decltype(m_mrl)>() )
    , m_origin( row.extract<decltype(m_origin)>() )
    , m_sizeType( row.extract<decltype(m_sizeType)>() )
    , m_status( row.extract<decltype(m_status)>() )
    , m_nbAttempts( row.extract<decltype(m_nbAttempts)>() )
    , m_isOwned( row.extract<decltype(m_isOwned)>() )
    , m_sharedCounter( row.extract<decltype(m_sharedCounter)>() )
{
    assert( row.hasRemainingColumns() == false );
    // If the thumbnail was generated by the medialibrary, we store it as a
    // relative path, from the user provided workspace
    if ( m_isOwned == true )
    {
        auto thumbnailDirMrl = utils::file::toMrl( m_ml->thumbnailPath() );
        assert( m_mrl.find( thumbnailDirMrl ) == std::string::npos );
        // We expect the relative part of the mrl to be already encoded, however
        // the path to the thumbnail directory is stored as a file path
        m_mrl = thumbnailDirMrl + m_mrl;
    }
}

Thumbnail::Thumbnail( MediaLibraryPtr ml, std::string mrl,
                      Thumbnail::Origin origin, ThumbnailSizeType sizeType, bool isOwned )
    : m_ml( ml )
    , m_id( 0 )
    , m_mrl( std::move( mrl ) )
    , m_origin( origin )
    , m_sizeType( sizeType )
    , m_status( ThumbnailStatus::Available )
    , m_nbAttempts( 0 )
    , m_isOwned( isOwned )
    , m_sharedCounter( 0 )
{
    // Store the mrl as is, and fiddle with it upon insertion as we only care
    // about storing a relative path in db, but we want to return the mrl as it
    // was given, ie. as an absolute mrl.
    assert( m_mrl.empty() == false &&
            utils::file::scheme( m_mrl ).empty() == false );
}

Thumbnail::Thumbnail( MediaLibraryPtr ml, ThumbnailStatus status,
                      Thumbnail::Origin origin, ThumbnailSizeType sizeType )
    : m_ml( ml )
    , m_id( 0 )
    , m_origin( origin )
    , m_sizeType( sizeType )
    , m_status( status )
    , m_nbAttempts( 0 )
    , m_isOwned( false )
    , m_sharedCounter( 0 )
{
    assert( m_status != ThumbnailStatus::Available &&
            m_status != ThumbnailStatus::Missing );
}

int64_t Thumbnail::id() const
{
    return m_id;
}

const std::string& Thumbnail::mrl() const
{
    assert( status() == ThumbnailStatus::Available  );
    return m_mrl;
}

bool Thumbnail::update( std::string mrl, bool isOwned )
{
    if ( m_mrl == mrl && isOwned == m_isOwned &&
         m_status == ThumbnailStatus::Available )
        return true;
    std::string storedMrl;
    if ( isOwned )
        storedMrl = toRelativeMrl( mrl );
    else
        storedMrl = mrl;
    // Also include the current generated state to the request, in case this update
    // request came while the thumbnailer was also generating a thumbnail
    static const std::string req = "UPDATE " + Thumbnail::Table::Name +
            " SET mrl = ?, status = ?, nb_attempts = 0, is_owned = ? "
            "WHERE id_thumbnail = ? AND is_owned = ?";
    if( sqlite::Tools::executeUpdate( m_ml->getConn(), req, storedMrl,
                                      ThumbnailStatus::Available, isOwned,
                                      m_id, m_isOwned ) == false )
        return false;
    m_mrl = std::move( mrl );
    m_isOwned = isOwned;
    m_status = ThumbnailStatus::Available;
    m_nbAttempts = 0;
    return true;
}

bool Thumbnail::updateLinkRecord( int64_t entityId, EntityType type,
                                  Thumbnail::Origin origin )
{
    const std::string req = "UPDATE " + LinkingTable::Name +
        " SET thumbnail_id = ?, origin = ?"
        " WHERE entity_id = ? AND entity_type = ? AND size_type = ?";
    // This needs to be run in a transaction, as we insert the new thumbnail
    // record or update the linked thumbnail
    assert( sqlite::Transaction::transactionInProgress() == true );
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id, origin,
                                       entityId, type, m_sizeType ) == false )
        return false;
    m_origin = origin;
    return true;
}

bool Thumbnail::insertLinkRecord( int64_t entityId, EntityType type,
                                  Thumbnail::Origin origin )
{
    const std::string req = "INSERT INTO " + LinkingTable::Name +
            " (entity_id, entity_type, size_type, thumbnail_id, origin)"
            " VALUES(?, ?, ?, ?, ?)";
    if ( sqlite::Tools::executeInsert( m_ml->getConn(), req, entityId, type,
                                  m_sizeType, m_id, origin ) == false )
        return false;
    m_sharedCounter++;
    return true;
}

void Thumbnail::unlinkThumbnail( int64_t entityId, EntityType type )
{
    const std::string req = "DELETE FROM " + LinkingTable::Name +
            " WHERE entity_id = ? AND entity_type = ? AND size_type = ?";
    if ( sqlite::Tools::executeDelete( m_ml->getConn(), req, entityId, type,
                                       m_sizeType ) == false )
        return;
    --m_sharedCounter;
}

Thumbnail::Origin Thumbnail::origin() const
{
    return m_origin;
}

bool Thumbnail::isOwned() const
{
    return m_isOwned;
}

bool Thumbnail::isShared() const
{
    assert( m_sharedCounter != 0 || m_id == 0 );
    return m_sharedCounter > 1;
}

ThumbnailSizeType Thumbnail::sizeType() const
{
    return m_sizeType;
}

ThumbnailStatus Thumbnail::status() const
{
    /*
     * Missing & PersistentFailure are only meant as a value to be returned when
     * no thumbnail record is present or when the generation repeatidly fails.
     * They are not meant to be inserted in database.
     */
    assert( m_status != ThumbnailStatus::Missing &&
            m_status != ThumbnailStatus::PersistentFailure );
    if ( m_status == ThumbnailStatus::Failure && m_nbAttempts >= 3 )
        return ThumbnailStatus::PersistentFailure;
    return m_status;
}

bool Thumbnail::markFailed()
{
    const std::string req = "UPDATE " + Table::Name +
            " SET status = ?, nb_attempts = nb_attempts + 1 WHERE id_thumbnail = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req,
                                       ThumbnailStatus::Failure, m_id ) == false )
        return false;
    m_status = ThumbnailStatus::Failure;
    ++m_nbAttempts;
    return true;
}

uint32_t Thumbnail::nbAttempts() const
{
    return m_nbAttempts;
}

void Thumbnail::relocate()
{
    // There is no point in relocating a failure record.
    assert( status() == ThumbnailStatus::Available );

    auto originalMrl = m_mrl;
    auto destPath = m_ml->thumbnailPath() +
                    std::to_string( m_id ) + "." +
                    utils::file::extension( originalMrl );
    std::string localPath;
    try
    {
        localPath = utils::file::toLocalPath( originalMrl );
    }
    catch ( const fs::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to relocate thumbnail ", originalMrl, ": ", ex.what() );
        return;
    }
    if ( utils::fs::copy( localPath, destPath ) == true )
    {
        auto destMrl = utils::file::toMrl( destPath );
        if ( sqlite::Tools::withRetries( 3, [this, &destMrl]() {
                return update( destMrl, true );
            }) == false )
        {
            utils::fs::remove( destPath );
        }
    }
}

std::shared_ptr<Thumbnail>
Thumbnail::updateOrReplace( MediaLibraryPtr ml,
                            std::shared_ptr<Thumbnail> oldThumbnail,
                            std::shared_ptr<Thumbnail> newThumbnail,
                            int64_t entityId, EntityType entityType )
{
    std::shared_ptr<Thumbnail> res;
    std::unique_ptr<sqlite::Transaction> t;
    if ( sqlite::Transaction::transactionInProgress() == false )
        t = ml->getConn()->newTransaction();
    /**
     * We are trying to assign the values from newThumbnail to oldThumbnail.
     * Multiple cases exist:
     * - oldThumbnail is a nullptr
     * - newThumbnail is not inserted, and is only a temporary object to provide
     *   the values that the user wishes to assign
     * - newThumbnail already exists
     *
     * If oldThumbnail is a nullptr, simply insert newThumbnail in DB if needed
     * and insert a linking record.
     *
     * If newThumbnail already exists, that's easy, we update the linking record
     * if any, or insert a new one.
     *
     * If oldThumbnail was already inserted, we might be able to update the
     * Thumbnail record itself, but only if the thumbnail isn't shared, as changing
     * it would change the thumbnail for other entities. So if the thumbnail isn't
     * shared, we update it, otherwise we update the linking record, which means
     * we may have to insert the newThumbnail if it hasn't been already
     */
    if ( oldThumbnail == nullptr )
    {
        if ( newThumbnail->id() == 0 )
        {
            if ( newThumbnail->insert() == 0 )
                return nullptr;
        }
        if ( newThumbnail->insertLinkRecord( entityId, entityType,
                                             newThumbnail->origin() ) == false )
            return nullptr;
        res = std::move( newThumbnail );
    }
    else if ( newThumbnail->id() != 0 )
    {
        newThumbnail->updateLinkRecord( entityId, entityType, newThumbnail->origin() );
        --oldThumbnail->m_sharedCounter;
        ++newThumbnail->m_sharedCounter;
        res = std::move( newThumbnail );
    }
    else
    {
        if ( oldThumbnail->id() != 0 && oldThumbnail->isShared() == false )
        {
            if ( oldThumbnail->update( newThumbnail->mrl(),
                                       newThumbnail->isOwned() ) == false )
                return nullptr;
            if ( oldThumbnail->origin() != newThumbnail->origin() )
            {
                if ( oldThumbnail->updateLinkRecord( entityId, entityType,
                                                     newThumbnail->origin() ) == false )
                    return nullptr;
            }
            res = oldThumbnail;
        }
        else
        {
            assert( oldThumbnail != nullptr );
            if ( newThumbnail->insert() == 0 )
                return nullptr;
            if ( newThumbnail->updateLinkRecord( entityId, entityType,
                                                 newThumbnail->origin() ) == false )
                return nullptr;
            --oldThumbnail->m_sharedCounter;
            ++newThumbnail->m_sharedCounter;
            res = newThumbnail;
        }
    }

    if ( t != nullptr )
        t->commit();
    return res;
}

void Thumbnail::createTable( sqlite::Connection* dbConnection )
{
    const std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
        schema( LinkingTable::Name, Settings::DbModelVersion )
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConnection, req );
}

void Thumbnail::createTriggers( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                trigger( Triggers::AutoDeleteAlbum, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                trigger( Triggers::AutoDeleteArtist, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                trigger( Triggers::AutoDeleteMedia, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                trigger( Triggers::IncrementRefcount, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                trigger( Triggers::DecrementRefcount, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                trigger( Triggers::UpdateRefcount, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                trigger( Triggers::DeleteUnused, Settings::DbModelVersion ) );
}

void Thumbnail::createIndexes( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::ThumbnailId, Settings::DbModelVersion ) );
}

std::string Thumbnail::schema( const std::string& tableName, uint32_t dbModel )
{
    if ( tableName == LinkingTable::Name )
    {
        // The linking table was added in model 17
        if ( dbModel < 17 )
        {
            assert( !"Invalid model version for thumbnail linking table schema" );
            return "<invalid request>";
        }
        return "CREATE TABLE " + LinkingTable::Name +
        "("
            "entity_id UNSIGNED INTEGER NOT NULL,"
            "entity_type UNSIGNED INTEGER NOT NULL,"
            "size_type UNSIGNED INTEGER NOT NULL,"
            "thumbnail_id UNSIGNED INTEGER NOT NULL,"
            "origin UNSIGNED INT NOT NULL,"

            "PRIMARY KEY(entity_id,entity_type,size_type),"
            "FOREIGN KEY(thumbnail_id) REFERENCES " +
                Table::Name + "(id_thumbnail) ON DELETE CASCADE"
        ")";
    }
    assert( tableName == Table::Name );
    if ( dbModel <= 17 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_thumbnail INTEGER PRIMARY KEY AUTOINCREMENT,"
            "mrl TEXT,"
            "is_generated BOOLEAN NOT NULL"
        ")";
    }
    if ( dbModel < 23 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_thumbnail INTEGER PRIMARY KEY AUTOINCREMENT,"
            "mrl TEXT,"
            "is_generated BOOLEAN NOT NULL,"
            "shared_counter INTEGER NOT NULL DEFAULT 0"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "("
        "id_thumbnail INTEGER PRIMARY KEY AUTOINCREMENT,"
        "mrl TEXT,"
        "status UNSIGNED INTEGER NOT NULL,"
        "nb_attempts UNSIGNED INTEGER DEFAULT 0,"
        "is_owned BOOLEAN NOT NULL,"
        "shared_counter INTEGER NOT NULL DEFAULT 0"
    ")";

}

std::string Thumbnail::trigger(Thumbnail::Triggers trigger, uint32_t dbModel)
{
    switch ( trigger )
    {
        case Triggers::AutoDeleteAlbum:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER DELETE ON " + Album::Table::Name +
                   " BEGIN"
                       " DELETE FROM " + LinkingTable::Name + " WHERE"
                           " entity_id = old.id_album AND"
                           " entity_type = " + std::to_string(
                               static_cast<std::underlying_type_t<EntityType>>(
                                   EntityType::Album ) ) + ";"
                   " END";
        case Triggers::AutoDeleteArtist:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER DELETE ON " + Artist::Table::Name +
                   " BEGIN"
                       " DELETE FROM " + LinkingTable::Name + " WHERE"
                           " entity_id = old.id_artist AND"
                           " entity_type = " + std::to_string(
                               static_cast<std::underlying_type_t<EntityType>>(
                                   EntityType::Artist ) ) + ";"
                   " END";
        case Triggers::AutoDeleteMedia:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER DELETE ON " + Media::Table::Name +
                   " BEGIN"
                       " DELETE FROM " + LinkingTable::Name + " WHERE"
                           " entity_id = old.id_media AND"
                           " entity_type = " + std::to_string(
                               static_cast<std::underlying_type_t<EntityType>>(
                                   EntityType::Media ) ) + ";"
                   " END";

        case Triggers::IncrementRefcount:
            assert( dbModel >= 18 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER INSERT ON " + LinkingTable::Name +
                   " BEGIN "
                       "UPDATE " + Table::Name + " "
                           "SET shared_counter = shared_counter + 1 "
                           "WHERE id_thumbnail = new.thumbnail_id;"
                   "END";
        case Triggers::DecrementRefcount:
            assert( dbModel >= 18 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER DELETE ON " + LinkingTable::Name +
                   " BEGIN "
                       "UPDATE " + Table::Name + " "
                           "SET shared_counter = shared_counter - 1 "
                           "WHERE id_thumbnail = old.thumbnail_id;"
                   "END";
        case Triggers::UpdateRefcount:
            assert( dbModel >= 18 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER UPDATE OF thumbnail_id ON " + LinkingTable::Name +
                   " WHEN old.thumbnail_id != new.thumbnail_id "
                   "BEGIN "
                       "UPDATE " + Table::Name +
                           " SET shared_counter = shared_counter - 1"
                               " WHERE id_thumbnail = old.thumbnail_id;"
                       "UPDATE " + Table::Name +
                           " SET shared_counter = shared_counter + 1"
                               " WHERE id_thumbnail = new.thumbnail_id;"
                   "END";
        case Triggers::DeleteUnused:
        {
            if ( dbModel <= 17 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                       " AFTER UPDATE OF thumbnail_id ON " + LinkingTable::Name +
                       " BEGIN "
                           " DELETE FROM " + Table::Name +
                           " WHERE id_thumbnail = old.thumbnail_id"
                           " AND (SELECT COUNT(*) FROM " + LinkingTable::Name +
                               " WHERE thumbnail_id = old.thumbnail_id) = 0;"
                       "END;";
            }
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER UPDATE OF shared_counter ON " + Table::Name +
                   " WHEN new.shared_counter = 0 "
                   "BEGIN "
                       "DELETE FROM " + Table::Name + " WHERE id_thumbnail = new.id_thumbnail;"
                   "END";
        }
        case Triggers::DeleteAfterLinkingDelete:
            assert( dbModel <= 17 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER DELETE ON " + LinkingTable::Name +
                   " BEGIN "
                       " DELETE FROM " + Table::Name +
                       " WHERE id_thumbnail = old.thumbnail_id"
                       " AND (SELECT COUNT(*) FROM " + LinkingTable::Name +
                           " WHERE thumbnail_id = old.thumbnail_id) = 0;"
                   "END";
        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Thumbnail::triggerName( Thumbnail::Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::AutoDeleteAlbum:
            return "auto_delete_album_thumbnail";
        case Triggers::AutoDeleteArtist:
            return "auto_delete_artist_thumbnail";
        case Triggers::AutoDeleteMedia:
            return "auto_delete_media_thumbnail";
        case Triggers::IncrementRefcount:
            assert( dbModel >= 18 );
            return "incr_thumbnail_refcount";
        case Triggers::DecrementRefcount:
            assert( dbModel >= 18 );
            return "decr_thumbnail_refcount";
        case Triggers::UpdateRefcount:
            assert( dbModel >= 18 );
            return "update_thumbnail_refcount";
        case Triggers::DeleteUnused:
        {
            if ( dbModel <= 17 )
                return "auto_delete_thumbnails_after_update";
            return "delete_unused_thumbnail";
        }
        case Triggers::DeleteAfterLinkingDelete:
            assert( dbModel <= 17 );
            return "auto_delete_thumbnails_after_delete";
        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Thumbnail::index( Indexes index, uint32_t dbModel )
{
    assert( index == Indexes::ThumbnailId );
    assert( dbModel >= 17 );
    return "CREATE INDEX " + indexName( index, dbModel ) +
           " ON " + Table::Name + "(id_thumbnail)";
}

std::string Thumbnail::indexName( Indexes index, uint32_t dbModel )
{
    assert( index == Indexes::ThumbnailId );
    assert( dbModel >= 17 );
    return "thumbnail_link_index";
}

bool Thumbnail::checkDbModel(MediaLibraryPtr ml)
{
    return sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) &&
           sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( LinkingTable::Name, Settings::DbModelVersion ),
                                       LinkingTable::Name );
}

std::shared_ptr<Thumbnail> Thumbnail::fetch( MediaLibraryPtr ml, EntityType type,
                                             int64_t entityId, ThumbnailSizeType sizeType )
{
    std::string req = "SELECT t.id_thumbnail, t.mrl, ent.origin, ent.size_type,"
            "t.status, t.nb_attempts, t.is_owned, t.shared_counter "
            "FROM " + Table::Name + " t "
            "INNER JOIN " + LinkingTable::Name + " ent "
                "ON t.id_thumbnail = ent.thumbnail_id "
            "WHERE ent.entity_id = ? AND ent.entity_type = ? AND ent.size_type = ?";
    return DatabaseHelpers<Thumbnail>::fetch( ml, req, entityId, type, sizeType );
}

int64_t Thumbnail::insert()
{
    assert( m_id == 0 );
    static const std::string req = "INSERT INTO " + Thumbnail::Table::Name +
            "(mrl, status, is_owned) VALUES(?, ?, ?)";
    auto pKey = sqlite::Tools::executeInsert( m_ml->getConn(), req,
                            m_isOwned == true ? toRelativeMrl( m_mrl ) : m_mrl,
                            m_status, m_isOwned );
    if ( pKey == 0 )
        return 0;
    m_id = pKey;
    return m_id;
}

bool Thumbnail::deleteFailureRecords(MediaLibraryPtr ml)
{
    static const std::string req = "DELETE FROM " + Table::Name +
                                   " WHERE mrl IS NULL";
    return sqlite::Tools::executeDelete( ml->getConn(), req );
}

std::string Thumbnail::path( MediaLibraryPtr ml, int64_t thumbnailId )
{
    return ml->thumbnailPath() + std::to_string( thumbnailId ) + ".jpg";
}

std::string Thumbnail::toRelativeMrl( const std::string& absoluteMrl )
{
    if ( absoluteMrl.empty() == true )
    {
        assert( status() != ThumbnailStatus::Available );
        return absoluteMrl;
    }
    // Ensure the thumbnail mrl is an absolute mrl and contained in the
    // thumbnail directory.
    assert( utils::file::schemeIs( "file://", absoluteMrl ) == true );
    auto thumbnailDirMrl = utils::file::toMrl( m_ml->thumbnailPath() );
    assert( absoluteMrl.find( thumbnailDirMrl ) == 0 );
    return utils::file::removePath( absoluteMrl, thumbnailDirMrl );
}


}
