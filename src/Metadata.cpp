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

const std::string policy::MediaMetadataTable::Name = "MediaMetadata";

MediaMetadata::Record::Record( IMedia::MetadataType t, std::string v )
    : m_type( t )
    , m_value( std::move( v ) )
    , m_isSet( true )
{
}

MediaMetadata::Record::Record( IMedia::MetadataType t )
    : m_type( t )
    , m_isSet( false )
{
}

bool MediaMetadata::Record::isSet() const
{
    return m_isSet;
}

int64_t MediaMetadata::Record::integer() const
{
    return atoll( m_value.c_str() );
}

const std::string& MediaMetadata::Record::str() const
{
    return m_value;
}

MediaMetadata::MediaMetadata( MediaLibraryPtr ml )
    : m_ml( ml )
    , m_nbMeta( 0 )
    , m_entityId( 0 )
{
}

void MediaMetadata::init( int64_t entityId, uint32_t nbMeta )
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
    static const std::string req = "SELECT * FROM " + policy::MediaMetadataTable::Name +
            " WHERE id_media = ?";
    auto conn = m_ml->getConn();
    auto ctx = conn->acquireReadContext();
    sqlite::Statement stmt( conn->handle(), req );
    stmt.execute( m_entityId );
    for ( sqlite::Row row = stmt.row(); row != nullptr; row = stmt.row() )
    {
        assert( row.load<int64_t>( 0 ) == m_entityId );
        m_records.emplace_back( row.load<decltype(Record::m_type)>( 1 ),
                          row.load<decltype(Record::m_value)>( 2 ) );
    }
}

bool MediaMetadata::isReady() const
{
    return m_nbMeta != 0;
}

IMediaMetadata&MediaMetadata::get( IMedia::MetadataType type ) const
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

bool MediaMetadata::set( IMedia::MetadataType type, const std::string& value )
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
        static const std::string req = "INSERT OR REPLACE INTO " + policy::MediaMetadataTable::Name +
                "(id_media, type, value) VALUES(?, ?, ?)";
        return sqlite::Tools::executeInsert( m_ml->getConn(), req, m_entityId, type, value );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to update media metadata: ", ex.what() );
        return false;
    }
}

bool MediaMetadata::set( IMedia::MetadataType type, int64_t value )
{
    auto str = std::to_string( value );
    return set( type, str );
}

void MediaMetadata::createTable(sqlite::Connection* connection)
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::MediaMetadataTable::Name + "("
            "id_media INTEGER,"
            "type INTEGER,"
            "value TEXT,"
            "PRIMARY KEY (id_media, type)"
            ")";
    sqlite::Tools::executeRequest( connection, req );
}

void MediaMetadata::Record::set( const std::string& value )
{
    m_value = value;
    m_isSet = true;
}

}
