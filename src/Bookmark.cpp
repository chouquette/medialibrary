/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Bookmark.h"
#include "Media.h"
#include "database/SqliteQuery.h"
#include "utils/ModificationsNotifier.h"

namespace medialibrary
{

const std::string Bookmark::Table::Name = "Bookmark";
const std::string Bookmark::Table::PrimaryKeyColumn = "id_bookmark";
int64_t Bookmark::* const Bookmark::Table::PrimaryKey = &Bookmark::m_id;

Bookmark::Bookmark( MediaLibraryPtr ml, int64_t time, int64_t mediaId )
    : m_ml( ml )
    , m_id( 0 )
    , m_time( time )
    , m_mediaId( mediaId )
    , m_creationDate( ::time( nullptr ) )
    , m_type( Type::Simple )
{
}

Bookmark::Bookmark( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_time( row.extract<decltype(m_time)>() )
    , m_name( row.extract<decltype(m_name)>() )
    , m_description( row.extract<decltype(m_description)>() )
    , m_mediaId( row.extract<decltype(m_mediaId)>() )
    , m_creationDate( row.extract<decltype(m_creationDate)>() )
    , m_type( row.extract<decltype(m_type)>() )
{
    assert( row.hasRemainingColumns() == false );
}

int64_t Bookmark::id() const
{
    return m_id;
}

int64_t Bookmark::mediaId() const
{
    return m_mediaId;
}

int64_t Bookmark::time() const
{
    return m_time;
}

const std::string&Bookmark::name() const
{
    return m_name;
}

bool Bookmark::setName( std::string name )
{
    return setNameAndDescription( std::move( name ), m_description );
}

const std::string&Bookmark::description() const
{
    return m_description;
}

time_t Bookmark::creationDate() const
{
    return m_creationDate;
}

IBookmark::Type Bookmark::type() const
{
    return m_type;
}

bool Bookmark::setDescription( std::string description )
{
    return setNameAndDescription( m_name, std::move( description ) );
}

bool Bookmark::setNameAndDescription( std::string name, std::string desc )
{
    if ( m_name == name && m_description == desc )
        return true;
    const std::string req = "UPDATE " + Table::Name +
            " SET name = ?, description = ? WHERE id_bookmark = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, name,
                                       desc, m_id ) == false )
        return false;
    m_name = std::move( name );
    m_description = std::move( desc );
    return true;
}

bool Bookmark::move( int64_t newTime )
{
    const std::string req = "UPDATE " + Table::Name +
            " SET time = ? WHERE id_bookmark = ?";
    try
    {
        if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, newTime,
                                           m_id ) == false )
            return false;
    }
    catch ( const sqlite::errors::ConstraintViolation& )
    {
        return false;
    }
    m_time = newTime;
    return true;
}

void Bookmark::createTable(sqlite::Connection* dbConn)
{
    const std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );
}

void Bookmark::createIndexes(sqlite::Connection* dbConnection)
{
    sqlite::Tools::executeRequest( dbConnection, index( Indexes::MediaId,
                                                        Settings::DbModelVersion ) );
}

std::string Bookmark::schema( const std::string& tableName, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( tableName );

    assert( dbModel >= 17 );
    assert( tableName == Table::Name );
    if ( dbModel < 25 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_bookmark INTEGER PRIMARY KEY AUTOINCREMENT,"
            "time UNSIGNED INTEGER NOT NULL,"
            "name TEXT,"
            "description TEXT,"
            "media_id UNSIGNED INTEGER NOT NULL,"
            "FOREIGN KEY(media_id) REFERENCES " +
                Media::Table::Name + "(id_media),"
            "UNIQUE(time,media_id) ON CONFLICT FAIL"
        ")";
    }
    if ( dbModel < 35 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_bookmark INTEGER PRIMARY KEY AUTOINCREMENT,"
            "time UNSIGNED INTEGER NOT NULL,"
            "name TEXT,"
            "description TEXT,"
            "media_id UNSIGNED INTEGER NOT NULL,"
            "creation_date UNSIGNED INTEGER NOT NULL,"
            "type UNSIGNED INTEGER NOT NULL,"
            "FOREIGN KEY(media_id) REFERENCES " +
                Media::Table::Name + "(id_media),"
            "UNIQUE(time,media_id) ON CONFLICT FAIL"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "("
        "id_bookmark INTEGER PRIMARY KEY AUTOINCREMENT,"
        "time UNSIGNED INTEGER NOT NULL,"
        "name TEXT,"
        "description TEXT,"
        "media_id UNSIGNED INTEGER NOT NULL,"
        "creation_date UNSIGNED INTEGER NOT NULL,"
        "type UNSIGNED INTEGER NOT NULL,"
        "FOREIGN KEY(media_id) REFERENCES " +
            Media::Table::Name + "(id_media) ON DELETE CASCADE,"
        "UNIQUE(time,media_id) ON CONFLICT FAIL"
    ")";
}

std::string Bookmark::index( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::MediaId:
            return "CREATE INDEX " + indexName( index, dbModel ) + " ON " +
                    Table::Name + "(media_id)";
    }
    return "<invalid request>";
}

std::string Bookmark::indexName( Indexes index, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( dbModel );

    switch ( index )
    {
        case Indexes::MediaId:
            assert( dbModel >= 34 );
            return "bookmark_media_id_idx";
    }
    return "<invalid request>";
}

bool Bookmark::checkDbModel( MediaLibraryPtr ml )
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );

    return sqlite::Tools::checkTableSchema(
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) &&
           sqlite::Tools::checkIndexStatement(
                 index( Indexes::MediaId, Settings::DbModelVersion ),
                 indexName( Indexes::MediaId, Settings::DbModelVersion ) );
}

std::shared_ptr<Bookmark> Bookmark::create( MediaLibraryPtr ml, int64_t time,
                                            int64_t mediaId )
{
    auto self = std::make_shared<Bookmark>( ml, time, mediaId );
    const std::string req = "INSERT INTO " + Table::Name +
            "(time, media_id, creation_date, type) VALUES(?, ?, ?, ?)";
    try
    {
        if ( insert( ml, self, req, time, mediaId, self->creationDate(),
                     self->type() ) == false )
            return nullptr;
    }
    catch ( const sqlite::errors::ConstraintUnique& )
    {
        return nullptr;
    }
    auto notifier = ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyBookmarkCreation( self );
    return self;
}

bool Bookmark::remove( MediaLibraryPtr ml, int64_t time, int64_t mediaId )
{
    const std::string req = "DELETE FROM " + Table::Name +
            " WHERE time = ? AND media_id = ?";
    return sqlite::Tools::executeDelete( ml->getConn(), req, time, mediaId );
}

Query<IBookmark> Bookmark::fromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                      const QueryParameters* params )
{
    const std::string req = "FROM " + Table::Name + " WHERE media_id = ?";
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    std::string orderBy = " ORDER BY ";
    switch ( sort )
    {
        case SortingCriteria::Alpha:
            orderBy += "name";
            break;
        case SortingCriteria::InsertionDate:
            orderBy += "creation_date";
            break;
        default:
            LOG_INFO( "Unsupported sorting criteria, falling back to default" );
            /* fall-through */
        case SortingCriteria::Default:
            orderBy += "time";
            break;
    }
    if ( params != nullptr && params->desc == true )
        orderBy += " DESC";
    return make_query<Bookmark, IBookmark>( ml, "*", req, orderBy, mediaId ).build();
}

std::shared_ptr<Bookmark> Bookmark::fromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                               int64_t time )
{
    const std::string req = "SELECT * FROM " + Table::Name +
            " WHERE time = ? AND media_id = ?";
    return fetch( ml, req, time, mediaId );
}

bool Bookmark::removeAll( MediaLibraryPtr ml, int64_t mediaId )
{
    const std::string req = "DELETE FROM " + Table::Name + " WHERE media_id = ?";
    return sqlite::Tools::executeDelete( ml->getConn(), req, mediaId );
}

}
