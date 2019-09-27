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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "VideoGroup.h"
#include "Media.h"
#include "medialibrary/IMediaLibrary.h"
#include "database/SqliteQuery.h"

namespace medialibrary
{

const std::string VideoGroup::Table::Name = "VideoGroup";

VideoGroup::VideoGroup( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_groupPattern( row.extract<decltype(m_groupPattern)>() )
    , m_count( row.extract<decltype(m_count)>() )
    , m_mediaName( row.extract<decltype(m_mediaName)>() )
{
    assert( row.hasRemainingColumns() == false );
}

const std::string& VideoGroup::name() const
{
    if ( m_count == 1 )
        return m_mediaName;
    assert( m_mediaName.empty() == true );
    return m_groupPattern;
}

size_t VideoGroup::count() const
{
    return m_count;
}

Query<IMedia> VideoGroup::media( const QueryParameters* params ) const
{
    return Media::fromVideoGroup( m_ml, m_groupPattern, params );
}

Query<IMedia> VideoGroup::searchMedia( const std::string& pattern,
                                       const QueryParameters* params ) const
{
    if ( pattern.size() < 3 )
        return nullptr;
    return Media::searchFromVideoGroup( m_ml, m_groupPattern, pattern, params );
}

Query<IVideoGroup> VideoGroup::listAll( MediaLibraryPtr ml, const QueryParameters* params )
{
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    std::string req = "SELECT * FROM " + Table::Name;
    const std::string countReq = "SELECT COUNT() FROM " + Table::Name;
    switch ( sort )
    {
        default:
            LOG_INFO( "Unsupported sorting criteria for media groups, falling "
                      "back to default" );
            /* fall-through */
        case SortingCriteria::Alpha:
        case SortingCriteria::Default:
            req += " ORDER BY grp";
            break;
        case SortingCriteria::NbMedia:
        case SortingCriteria::NbVideo:
            req += " ORDER BY cnt";
            break;
    }
    if ( desc == true )
        req += " DESC";
    return make_query_with_count<VideoGroup, IVideoGroup>( ml, countReq, req);
}

VideoGroupPtr VideoGroup::fromName( MediaLibraryPtr ml, const std::string& name )
{
    const std::string req = "SELECT * FROM " + Table::Name +
            " WHERE grp = LOWER(?1) OR media_title = ?1";
    return fetch( ml, req, name );
}

std::string VideoGroup::schema( const std::string& tableName, uint32_t )
{
    assert( tableName == Table::Name );
    return "CREATE VIEW " + Table::Name + " AS"
           " SELECT "
                "LOWER(SUBSTR("
                    "CASE "
                        "WHEN title LIKE 'The %' THEN SUBSTR(title, 5) "
                        "ELSE title "
                    "END, "
                "1, (SELECT video_groups_prefix_length FROM Settings)))"
           " as grp, COUNT() as cnt,"
           " CASE WHEN COUNT() = 1 THEN title ELSE NULL END as media_title"
           " FROM Media "
           " WHERE type = " +
                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                    IMedia::Type::Video ) ) +
           " AND is_present != 0"
           " GROUP BY grp";

}

void VideoGroup::createView( sqlite::Connection* dbConn )
{
    std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );
}

}
