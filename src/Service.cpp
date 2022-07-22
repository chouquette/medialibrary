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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Service.h"
#include "Subscription.h"
#include "parser/Task.h"

namespace medialibrary
{

const std::string Service::Table::Name = "Service";
const std::string Service::Table::PrimaryKeyColumn = "id_service";
int64_t Service::*const Service::Table::PrimaryKey = &Service::m_id;

Service::Service( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_autoDownload( row.extract<decltype(m_autoDownload)>() )
    , m_newMediaNotif( row.extract<decltype(m_newMediaNotif)>() )
    , m_maxCacheSize( row.extract<decltype(m_maxCacheSize)>() )
{
    assert( row.hasRemainingColumns() == false );
}

Service::Service( MediaLibraryPtr ml, Type type )
    : m_ml( ml )
    , m_id( static_cast<std::underlying_type_t<Type>>( type ) )
    , m_autoDownload( true )
    , m_newMediaNotif( true )
    , m_maxCacheSize( -1 )
{
}

Service::Type Service::type() const
{
    return static_cast<Type>( m_id );
}

bool Service::addSubscription( std::string mrl )
{
    auto t = parser::Task::create( m_ml, std::move( mrl ), type() );
    if ( t == nullptr )
        return false;
    auto parser = m_ml->getParser();
    if ( parser == nullptr )
        return false;
    parser->parse( std::move( t ) );
    return true;
}

Query<ISubscription> Service::subscriptions( const QueryParameters* params )
{
    return Subscription::fromService( m_ml, type(), params );
}

bool Service::isAutoDownloadEnabled() const
{
    return m_autoDownload;
}

bool Service::setAutoDownloadEnabled( bool enabled )
{
    if ( m_autoDownload == enabled )
        return true;
    const std::string req = "UPDATE " + Table::Name +
            " SET auto_download = ? WHERE id_service = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, enabled, m_id ) == false )
        return false;
    m_autoDownload = enabled;
    return true;
}

bool Service::isNewMediaNotificationEnabled() const
{
    return m_newMediaNotif;
}

bool Service::setNewMediaNotificationEnabled( bool enabled )
{
    if ( m_newMediaNotif == enabled )
        return true;
    const std::string req = "UPDATE " + Table::Name +
            " SET notify = ? WHERE id_service = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, enabled, m_id ) == false )
        return false;
    m_newMediaNotif = enabled;
    return true;
}

int64_t Service::maxCachedSize() const
{
    return m_maxCacheSize;
}

bool Service::setMaxCachedSize( int64_t maxSize )
{
    if ( maxSize < 0 )
        maxSize = -1;
    if ( m_maxCacheSize == maxSize )
        return true;
    const std::string req = "UPDATE " + Table::Name +
            " SET max_cached_size = ? WHERE id_service = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, maxSize, m_id ) == false )
        return false;
    m_maxCacheSize = maxSize;
    return true;
}

std::string Service::schema( const std::string& name, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( name );
    UNUSED_IN_RELEASE( dbModel );

    assert( dbModel >= 37 );
    assert( name == Table::Name );
    return "CREATE TABLE " + Table::Name +
           "("
               + Table::PrimaryKeyColumn + " UNSIGNED INTEGER PRIMARY KEY,"
               "auto_download BOOLEAN NOT NULL DEFAULT 1,"
               "notify BOOLEAN NOT NULL DEFAULT 1,"
               "max_cached_size INTEGER NOT NULL DEFAULT -1"
           ")";
}

void Service::createTable( sqlite::Connection* dbConn )
{
    sqlite::Tools::executeRequest( dbConn,
                                   schema( Table::Name, Settings::DbModelVersion ) );
}

bool Service::checkDbModel( MediaLibraryPtr ml )
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );

    return sqlite::Tools::checkTableSchema(
                schema( Table::Name, Settings::DbModelVersion ), Table::Name );
}

std::shared_ptr<Service> Service::create( MediaLibraryPtr ml, Type type )
{
    auto self = std::make_shared<Service>( ml, type );
    const std::string req = "INSERT INTO " + Table::Name + " ("
            + Table::PrimaryKeyColumn + ") VALUES(?)";
    if ( insert( ml, self, req, type ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<Service> Service::fetch( MediaLibraryPtr ml, Type type )
{
    auto s = DatabaseHelpers<Service>::fetch( ml,
                static_cast<std::underlying_type_t<IService::Type>>( type ) );
    if ( s == nullptr )
        s = create( ml, type );
    return s;
}

}
