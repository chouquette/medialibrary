/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2018-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Chapter.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"
#include "Media.h"

namespace medialibrary
{

const std::string Chapter::Table::Name = "Chapter";
const std::string Chapter::Table::PrimaryKeyColumn = "id_chapter";
int64_t Chapter::* const Chapter::Table::PrimaryKey = &Chapter::m_id;

void Chapter::createTable( sqlite::Connection* dbConn )
{
    return sqlite::Tools::executeRequest( dbConn,
                                          schema( Table::Name, Settings::DbModelVersion ) );
}

std::string Chapter::schema( const std::string& tableName, uint32_t )
{
    assert( tableName == Table::Name );
    return "CREATE TABLE " + Table::Name +
    "("
        + Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
        "offset INTEGER NOT NULL,"
        "duration INTEGER NOT NULL,"
        "name TEXT,"
        "media_id INTEGER,"
        "FOREIGN KEY(media_id) REFERENCES " +
            Media::Table::Name + "(" + Media::Table::PrimaryKeyColumn + ")"
            " ON DELETE CASCADE"
    ")";
}

bool Chapter::checkDbModel( MediaLibraryPtr ml )
{
    return sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name );
}

std::shared_ptr<Chapter> Chapter::create( MediaLibraryPtr ml, int64_t offset,
                                          int64_t duration, std::string name,
                                          int64_t mediaId )
{
    static const std::string req = "INSERT INTO " + Table::Name +
            "(offset, duration, name, media_id) VALUES(?, ?, ?, ?)";
    auto self = std::make_shared<Chapter>( ml, offset, duration,
                                           std::move( name ) );
    if ( insert( ml, self, req, offset, duration, self->m_name, mediaId ) == false )
        return nullptr;
    return self;
}

Query<IChapter> Chapter::fromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                    const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " WHERE media_id = ?";
    std::string orderBy = "ORDER BY ";
    auto desc = params != nullptr ? params->desc : false;
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;

    switch ( sort )
    {
        case SortingCriteria::Alpha:
            orderBy += "name";
            break;
        case SortingCriteria::Duration:
            orderBy += "duration";
            desc = !desc;
            break;
        default:
            LOG_WARN( "Unsupported sorting criteria ",
                      static_cast<std::underlying_type<SortingCriteria>::type>( sort ),
                      " falling back to default (by offset)" );
            /* fall-through */
        case SortingCriteria::Default:
            orderBy += "offset";
            break;
    }
    if ( desc == true )
        orderBy += " DESC";

    return make_query<Chapter, IChapter>( ml, "*", req, orderBy, mediaId );
}

Chapter::Chapter( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_offset( row.extract<decltype(m_offset)>() )
    , m_duration( row.extract<decltype(m_offset)>() )
    , m_name( row.extract<decltype(m_name)>() )
{
    // Simply check that the media_id row is present, until we eventually store it
    assert( row.extract<int64_t>() );
    assert( row.hasRemainingColumns() == false );
}

Chapter::Chapter( MediaLibraryPtr ml, int64_t offset, int64_t duration,
                  std::string name )
    : m_ml( ml )
    , m_id( 0 )
    , m_offset( offset )
    , m_duration( duration )
    , m_name( std::move( name ) )
{
}

const std::string& Chapter::name() const
{
    return m_name;
}

int64_t Chapter::offset() const
{
    return m_offset;
}

int64_t Chapter::duration() const
{
    return m_duration;
}

}
