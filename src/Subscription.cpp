/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Subscription.h"
#include "Media.h"
#include "File.h"
#include "parser/Task.h"
#include "utils/ModificationsNotifier.h"
#include "utils/Enums.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"

namespace medialibrary
{

const std::string Subscription::Table::Name = "Subscription";
const std::string Subscription::Table::PrimaryKeyColumn = "id_subscription";
int64_t Subscription::*const Subscription::Table::PrimaryKey = &Subscription::m_id;
const std::string Subscription::MediaRelationTable::Name = "SubscriptionMediaRelation";

Subscription::Subscription( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_service( row.extract<decltype(m_service)>() )
    , m_name( row.extract<decltype(m_name)>() )
    , m_parentId( row.extract<decltype(m_parentId)>() )
    , m_cachedSize( row.extract<decltype(m_cachedSize)>() )
    , m_maxCachedMedia( row.extract<decltype(m_maxCachedMedia)>() )
    , m_maxCachedSize( row.extract<decltype(m_maxCachedSize)>() )
{
    assert( row.hasRemainingColumns() == false );
}

Subscription::Subscription( MediaLibraryPtr ml, Service service, std::string name,
                        int64_t parentId )
    : m_ml( ml )
    , m_id( 0 )
    , m_service( service )
    , m_name( std::move( name ) )
    , m_parentId( parentId )
    , m_cachedSize( 0 )
    , m_maxCachedMedia( -1 )
    , m_maxCachedSize( -1 )
{
}

int64_t Subscription::id() const
{
    return m_id;
}

const std::string& Subscription::name() const
{
    return m_name;
}

Query<ISubscription> Subscription::childSubscriptions( const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " c WHERE parent_id = ?";
    return make_query<Subscription, ISubscription>( m_ml, "c.*", req,
                                                orderBy( params ), m_id ).build();
}

SubscriptionPtr Subscription::parent()
{
    if ( m_parentId == 0 )
        return nullptr;
    return fetch( m_ml, m_parentId );
}

Query<IMedia> Subscription::media( const QueryParameters* params )
{
    return Media::fromSubscription( m_ml, m_id, params );
}

int64_t Subscription::cachedSize() const
{
    return m_cachedSize;
}

int32_t Subscription::maxCachedMedia() const
{
    return m_maxCachedMedia;
}

bool Subscription::setMaxCachedMedia( int32_t nbCachedMedia )
{
    if ( m_maxCachedMedia == nbCachedMedia )
        return true;
    if ( nbCachedMedia < 0 )
        nbCachedMedia = -1;
    const std::string req = "UPDATE " + Table::Name +
            " SET max_cached_media = ?1 WHERE id_subscription = ?2";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req,
                                       nbCachedMedia, m_id ) == false )
        return false;
    m_maxCachedMedia = nbCachedMedia;
    return true;
}

int64_t Subscription::maxCachedSize() const
{
    return m_maxCachedSize;
}

bool Subscription::setMaxCachedSize( int64_t maxCachedSize )
{
    if ( m_maxCachedSize == maxCachedSize )
        return true;
    if ( maxCachedSize < 0 )
        maxCachedSize = -1;
    const std::string req = "UPDATE " + Table::Name +
            " SET max_cached_size = ?1 WHERE id_subscription = ?2";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req,
                                       maxCachedSize, m_id ) == false )
        return false;
    m_maxCachedSize = maxCachedSize;
    return true;
}

bool Subscription::refresh()
{
    auto f = file();
    if ( f == nullptr )
        return false;
    auto t = parser::Task::createRefreshTask( m_ml, std::move( f ) );
    if ( t == nullptr )
        return false;
    auto parser = m_ml->getParser();
    if ( parser == nullptr )
        return false;
    parser->parse( std::move( t ) );
    return true;
}

std::shared_ptr<File> Subscription::file() const
{
    const std::string req = "SELECT * FROM " + File::Table::Name +
            " WHERE subscription_id = ?";
    return File::fetch( m_ml, req, m_id );
}

Query<Media> Subscription::cachedMedia( bool evictableOnly ) const
{
    if ( evictableOnly == true )
    {
        const std::string req = "FROM " + Media::Table::Name + " m "
                " INNER JOIN " + MediaRelationTable::Name + " mrt "
                " ON m.id_media = mrt.media_id"
                " WHERE mrt.subscription_id = ? AND"
                " EXISTS(SELECT id_file FROM " + File::Table::Name +
                " WHERE media_id = m.id_media AND type = ? AND "
                " cache_type = ? OR m.play_count > 0)";
        const std::string order = " ORDER BY m.play_count DESC, m.release_date ASC";
        return make_query<Media>( m_ml, "m.*", req, order, m_id,
                        IFile::Type::Cache, File::CacheType::Automatic ).build();
    }
    const std::string req = "FROM " + Media::Table::Name + " m "
                " INNER JOIN " + MediaRelationTable::Name + " mrt "
                " ON m.id_media = mrt.media_id"
                " WHERE mrt.subscription_id = ? AND"
                " EXISTS(SELECT id_file FROM " + File::Table::Name +
                " WHERE media_id = m.id_media AND type = ?)";
    const std::string order = " ORDER BY m.play_count DESC, m.release_date ASC";
    return make_query<Media>( m_ml, "m.*", req, order, m_id,
                    IFile::Type::Cache ).build();
}

std::vector<std::shared_ptr<Media>> Subscription::uncachedMedia( bool autoOnly ) const
{
    std::string req = "SELECT m.* FROM " + Media::Table::Name + " m "
            " INNER JOIN " + MediaRelationTable::Name + " mrt "
            " ON m.id_media = mrt.media_id"
            " WHERE mrt.subscription_id = ?1 AND"
            " NOT EXISTS(SELECT id_file FROM " + File::Table::Name +
            " WHERE media_id = m.id_media AND type = ?2)" +
            ( autoOnly == true ? " AND mrt.auto_cache_handled = 0" : "" ) +
            " ORDER BY m.release_date DESC"
            // it's easier to use IFNULL than IIF here, which is why the first
            // SELECT is written as to return NULL if the setting is set to -1
            " LIMIT IFNULL( "
                "(SELECT max_cached_media FROM " + Table::Name +
                    " WHERE id_subscription = ?1 AND max_cached_media >= 0),"
                "(SELECT nb_cached_media_per_subscription FROM Settings)"
            ")";
    return Media::fetchAll<Media>( m_ml, req, m_id, IFile::Type::Cache );
}

bool Subscription::markCacheAsHandled()
{
    const std::string req = "UPDATE " + MediaRelationTable::Name +
            " SET auto_cache_handled = 1 WHERE subscription_id = ?";
    return sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id );
}

bool Subscription::addMedia( Media& m )
{
    return addMedia( m_ml, m_id, m.id() );
}

bool Subscription::addMedia( MediaLibraryPtr ml, int64_t subscriptionId, int64_t mediaId )
{
    const std::string req = "INSERT INTO " + MediaRelationTable::Name +
            "(media_id, subscription_id) VALUES(?, ?)";
    return sqlite::Tools::executeUpdate( ml->getConn(), req, mediaId, subscriptionId );
}

bool Subscription::removeMedia( int64_t mediaId )
{
    const std::string req = "DELETE FROM " + MediaRelationTable::Name +
            " WHERE media_id = ? AND subscription_id = ?";
    return sqlite::Tools::executeDelete( m_ml->getConn(), req, mediaId, m_id );
}

std::shared_ptr<Subscription> Subscription::addChildSubscription( std::string name )
{
    return create( m_ml, m_service, std::move( name ), m_id );
}

bool Subscription::clearContent()
{
    const std::string req = "DELETE FROM " + MediaRelationTable::Name +
            " WHERE subscription_id = ?";
    return sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id );
}

void Subscription::createTable( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   schema( MediaRelationTable::Name, Settings::DbModelVersion ) );
}

void Subscription::createTriggers( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
            trigger( Triggers::PropagateTaskDeletion, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
            trigger( Triggers::IncrementCachedSize, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
            trigger( Triggers::DecrementCachedSize, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
            trigger( Triggers::DecrementCachedSizeOnRemoval, Settings::DbModelVersion ) );
}

void Subscription::createIndexes( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::ServiceId, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::RelationMediaId, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::RelationSubscriptionId, Settings::DbModelVersion ) );
}

std::string Subscription::schema( const std::string& name, uint32_t dbModel )
{
    assert( dbModel >= 37 );
    if ( name == MediaRelationTable::Name )
    {
        return "CREATE TABLE " + MediaRelationTable::Name +
               "("
                   "media_id UNSIGNED INTEGER,"
                   "subscription_id UNSIGNED INTEGER,"
                   "auto_cache_handled BOOLEAN NOT NULL DEFAULT 0,"
                   "UNIQUE(media_id, subscription_id) ON CONFLICT FAIL,"
                   "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name + "("
                       + Media::Table::PrimaryKeyColumn + ") ON DELETE CASCADE,"
                   "FOREIGN KEY(subscription_id) REFERENCES " + Table::Name + "("
                       + Table::PrimaryKeyColumn + ") ON DELETE CASCADE"
               ")";
    }
    assert( name == Table::Name );
    return "CREATE TABLE " + Table::Name +
           "("
               + Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
               "service_id UNSIGNED INTEGER NOT NULL,"
               "name TEXT NOT NULL,"
               "parent_id UNSIGNED INTEGER,"
               "cached_size UNSIGNED INTEGER NOT NULL DEFAULT 0,"
               "max_cached_media INTEGER NOT NULL DEFAULT -1,"
               "max_cached_size INTEGER NOT NULL DEFAULT -1,"
               "FOREIGN KEY(parent_id) REFERENCES " + Table::Name +
                   "(" + Table::PrimaryKeyColumn + ") ON DELETE CASCADE"
           ")";
}

std::string Subscription::trigger( Triggers trigger, uint32_t dbModel )
{
    assert( dbModel >= 37 );

    switch ( trigger )
    {
    case Triggers::PropagateTaskDeletion:
        return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
               " AFTER DELETE ON " + Table::Name +
               " BEGIN"
                   " DELETE FROM " + parser::Task::Table::Name + ";"
               " END";
    case Triggers::IncrementCachedSize:
        return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
               " AFTER INSERT ON " + File::Table::Name +
               " WHEN new.type = " + utils::enum_to_string( IFile::Type::Cache ) +
               " BEGIN"
                   " UPDATE " + Table::Name +
                       " SET cached_size = cached_size + IFNULL(new.size, 0)"
                           " WHERE id_subscription IN"
                           " (SELECT subscription_id FROM " + MediaRelationTable::Name +
                               " WHERE media_id = new.media_id);"
               " END";
    case Triggers::DecrementCachedSize:
        return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
               " AFTER DELETE ON " + File::Table::Name +
               " WHEN old.type = " + utils::enum_to_string( IFile::Type::Cache ) +
               " BEGIN"
                   " UPDATE " + Table::Name +
                       " SET cached_size = cached_size - IFNULL(old.size, 0)"
                           " WHERE id_subscription IN"
                           " (SELECT subscription_id FROM " + MediaRelationTable::Name +
                               " WHERE media_id = old.media_id);"
               " END";
    case Triggers::DecrementCachedSizeOnRemoval:
        return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
               " AFTER DELETE ON " + MediaRelationTable::Name +
               " BEGIN"
                   " UPDATE " + Table::Name +
                       " SET cached_size = cached_size - IFNULL((SELECT size FROM "
                       + File::Table::Name + " WHERE type = " +
                            utils::enum_to_string( IFile::Type::Cache ) +
                            " AND media_id = old.media_id), 0)"
                    " WHERE id_subscription = old.subscription_id;"
               " END";
    }
    return "<invalid trigger>";
}

std::string Subscription::triggerName( Triggers trigger, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( dbModel );
    assert( dbModel >= 37 );

    switch ( trigger )
    {
    case Triggers::PropagateTaskDeletion:
        return "subscription_propagate_task_deletion";
    case Triggers::IncrementCachedSize:
        return "subscription_increment_cached_size";
    case Triggers::DecrementCachedSize:
        return "subscription_decrement_cached_size";
    case Triggers::DecrementCachedSizeOnRemoval:
        return "subscription_decrement_cached_size_on_removal";
    }
    return "<invalid trigger>";
}

std::string Subscription::index( Indexes index, uint32_t dbModel )
{
    assert( dbModel >= 37 );
    switch ( index )
    {
    case Indexes::ServiceId:
        return "CREATE INDEX " + indexName( index, dbModel ) +
               " ON " + Table::Name + "(service_id)";
    case Indexes::RelationMediaId:
        return "CREATE INDEX " + indexName( index, dbModel ) +
               " ON " + MediaRelationTable::Name + "(media_id)";
    case Indexes::RelationSubscriptionId:
        return "CREATE INDEX " + indexName( index, dbModel ) +
               " ON " + MediaRelationTable::Name + "(subscription_id)";
    default:
        break;
    }
    return "<invalid index>";
}

std::string Subscription::indexName( Indexes index, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( dbModel );
    assert( dbModel >= 37 );
    switch ( index )
    {
    case Indexes::ServiceId:
        return "subscription_service_id_idx";
    case Indexes::RelationMediaId:
        return "subscription_rel_media_id_idx";
    case Indexes::RelationSubscriptionId:
        return "subscription_rel_subscription_id_idx";
    default:
        break;
    }
    return "<invalid index>";
}

bool Subscription::checkDbModel( MediaLibraryPtr ml )
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );

    auto checkTrigger = [](Triggers t) {
        return sqlite::Tools::checkTriggerStatement(
                    trigger( t, Settings::DbModelVersion ),
                    triggerName( t, Settings::DbModelVersion ) );
    };
    auto checkIndex = [](Indexes i) {
        return sqlite::Tools::checkIndexStatement(
                    index( i, Settings::DbModelVersion ),
                    indexName( i, Settings::DbModelVersion ) );
    };

    return sqlite::Tools::checkTableSchema(
                schema( Table::Name, Settings::DbModelVersion ), Table::Name ) &&
           checkTrigger( Triggers::PropagateTaskDeletion ) &&
           checkTrigger( Triggers::IncrementCachedSize ) &&
           checkTrigger( Triggers::DecrementCachedSize ) &&
           checkTrigger( Triggers::DecrementCachedSizeOnRemoval ) &&
           checkIndex( Indexes::ServiceId ) &&
           checkIndex( Indexes::RelationMediaId ) &&
           checkIndex( Indexes::RelationSubscriptionId );
}

std::shared_ptr<Subscription> Subscription::create(MediaLibraryPtr ml, Service service,
                                                std::string name, int64_t parentId )
{
    auto self = std::make_shared<Subscription>( ml, service, std::move( name ), parentId );
    const std::string req = "INSERT INTO " + Table::Name +
            "(service_id, name, parent_id) VALUES(?, ?, ?)";
    if ( insert( ml, self, req, service, self->m_name,
                 sqlite::ForeignKey{ parentId } ) == false )
        return nullptr;
    auto notifier = ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifySubscriptionCreation( self );
    return self;
}

Query<ISubscription> Subscription::fromService( MediaLibraryPtr ml, Service service,
                                            const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name +
            " c WHERE c.service_id = ? AND c.parent_id IS NULL";
    return make_query<Subscription, ISubscription>( ml, "c.*", req,
                                                orderBy( params ), service ).build();
}

std::shared_ptr<Subscription> Subscription::fromFile( MediaLibraryPtr ml, int64_t fileId )
{
    const std::string req = "SELECT c.* FROM " + Table::Name + " c"
            " INNER JOIN " + File::Table::Name + " f"
            " ON f.subscription_id = c.id_subscription"
            " WHERE f.id_file = ?";
    return fetch( ml, req, fileId );
}

std::string Subscription::orderBy( const QueryParameters* params )
{
    auto desc = params != nullptr && params->desc;
    std::string order = "ORDER BY name";
    if ( desc )
        order += " DESC";
    return order;
}

}
