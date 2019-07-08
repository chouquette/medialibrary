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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Metadata.h"

#include "database/SqliteTools.h"

#include <algorithm>

namespace medialibrary
{

const std::string Metadata::Table::Name = "Metadata";

Metadata::Record::Record( uint32_t t, std::string v )
    : m_type( t )
    , m_value( std::move( v ) )
    , m_isSet( true )
{
}

Metadata::Record::Record( uint32_t t )
    : m_type( t )
    , m_isSet( false )
{
}

bool Metadata::Record::isSet() const
{
    return m_isSet;
}

int64_t Metadata::Record::integer() const
{
    return atoll( m_value.c_str() );
}

double Metadata::Record::asDouble() const
{
    return atof( m_value.c_str() );
}

const std::string& Metadata::Record::str() const
{
    return m_value;
}

void Metadata::Record::unset()
{
    m_isSet = false;
    m_value.clear();
}

Metadata::Metadata(MediaLibraryPtr ml , IMetadata::EntityType entityType)
    : m_ml( ml )
    , m_entityType( entityType )
    , m_nbMeta( 0 )
    , m_entityId( 0 )
{
}

void Metadata::init( int64_t entityId, uint32_t nbMeta )
{
    if ( isReady() == true )
        return;

    m_nbMeta = nbMeta;
    m_entityId = entityId;
    // Reserve the space for all meta to avoid a race condition where 2 threads
    // would cache different meta, invalidating the potential reference
    // to another IMediaMetadata held by another thread.
    // This guarantees the vector will not grow afterward.
    m_records.reserve( m_nbMeta );
    static const std::string req = "SELECT * FROM " + Metadata::Table::Name +
            " WHERE id_media = ? AND entity_type = ?";
    auto conn = m_ml->getConn();
    auto ctx = conn->acquireReadContext();
    sqlite::Statement stmt( conn->handle(), req );
    stmt.execute( m_entityId, m_entityType );
    for ( sqlite::Row row = stmt.row(); row != nullptr; row = stmt.row() )
    {
        assert( row.load<int64_t>( 0 ) == m_entityId );
        assert( row.load<IMetadata::EntityType>( 1 ) == m_entityType );
        m_records.emplace_back( row.load<decltype(Record::m_type)>( 2 ),
                          row.load<decltype(Record::m_value)>( 3 ) );
    }
}

bool Metadata::isReady() const
{
    return m_nbMeta != 0;
}

IMetadata& Metadata::get( uint32_t type ) const
{
    assert( isReady() == true );

    auto it = std::find_if( begin( m_records ), end( m_records ), [type](const Record& r ) {
        return r.m_type == type;
    });
    if ( it == end( m_records ) )
    {
        // Create an unset meta for the given type˙No DB entity will be created until
        // the meta is actually set.
        m_records.emplace_back( type );
        return *( m_records.rbegin() );
    }
    return *it;
}

bool Metadata::set( uint32_t type, const std::string& value )
{
    assert( isReady() == true );

    auto it = std::find_if( begin( m_records ), end( m_records ), [type]( const Record& r ) {
        return r.m_type == type;
    });
    if ( it != end( m_records ) )
        (*it).set( value );
    else
        m_records.emplace_back( type, value );
    try
    {
        static const std::string req = "INSERT OR REPLACE INTO " + Metadata::Table::Name +
                "(id_media, entity_type, type, value) VALUES(?, ?, ?, ?)";
        return sqlite::Tools::executeInsert( m_ml->getConn(), req, m_entityId, m_entityType,
                                             type, value );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to update media metadata: ", ex.what() );
        return false;
    }
}

bool Metadata::set( uint32_t type, int64_t value )
{
    auto str = std::to_string( value );
    return set( type, str );
}

bool Metadata::unset( uint32_t type )
{
    assert( isReady() == true );
    auto it = std::find_if( begin( m_records ), end( m_records ), [type]( const Record& r ) {
        return r.m_type == type;
    });
    if ( it != end( m_records ) )
    {
        static const std::string req = "DELETE FROM " + Metadata::Table::Name +
                " WHERE id_media = ? AND entity_type = ? AND type = ?";
        (*it).unset();
        return sqlite::Tools::executeDelete( m_ml->getConn(), req, m_entityId,
                                             m_entityType, type );
    }
    return true;
}

void Metadata::unset( sqlite::Connection* dbConn, IMetadata::EntityType entityType, uint32_t type )
{
    static const std::string req = "DELETE FROM " + Metadata::Table::Name +
            " WHERE entity_type = ? AND type = ? ";
    sqlite::Tools::executeDelete( dbConn, req, entityType, type );
}

void Metadata::createTable(sqlite::Connection* connection)
{
    const std::string reqs[] = {
        #include "database/tables/Metadata_v14.sql"
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( connection, req );
}

void Metadata::Record::set( const std::string& value )
{
    m_value = value;
    m_isSet = true;
}

uint32_t Metadata::Record::type() const
{
    return m_type;
}

}
