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

bool Subscription::addMedia( Media& m )
{
    return addMedia( m_ml, m_id, m.id() );
}

bool Subscription::addMedia(MediaLibraryPtr ml, int64_t subscriptionId, int64_t mediaId)
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
               "FOREIGN KEY(parent_id) REFERENCES " + Table::Name +
                   "(" + Table::PrimaryKeyColumn + ") ON DELETE CASCADE"
           ")";
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

    auto checkIndex = [](Indexes i) {
        return sqlite::Tools::checkIndexStatement(
                    index( i, Settings::DbModelVersion ),
                    indexName( i, Settings::DbModelVersion ) );
    };

    return sqlite::Tools::checkTableSchema(
                schema( Table::Name, Settings::DbModelVersion ), Table::Name ) &&
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
