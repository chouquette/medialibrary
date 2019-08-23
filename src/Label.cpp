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

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "Label.h"
#include "Media.h"
#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"

namespace medialibrary
{

const std::string Label::Table::Name = "Label";
const std::string Label::Table::PrimaryKeyColumn = "id_label";
int64_t Label::* const Label::Table::PrimaryKey = &Label::m_id;
const std::string Label::FileRelationTable::Name = "LabelFileRelation";

Label::Label(MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_name( row.extract<decltype (m_name)>() )
{
    assert( row.hasRemainingColumns() == false );
}

Label::Label( MediaLibraryPtr ml, const std::string& name )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( name )
{
}

int64_t Label::id() const
{
    return m_id;
}

const std::string& Label::name() const
{
    return m_name;
}

Query<IMedia> Label::media()
{
    static const std::string req = "FROM " + Media::Table::Name + " f "
            "INNER JOIN " + FileRelationTable::Name + " lfr ON lfr.media_id = f.id_media "
            "WHERE lfr.label_id = ?";
    return make_query<Media, IMedia>( m_ml, "f.*", req, "", m_id );
}

LabelPtr Label::create( MediaLibraryPtr ml, const std::string& name )
{
    auto self = std::make_shared<Label>( ml, name );
    const char* req = "INSERT INTO Label VALUES(NULL, ?)";
    if ( insert( ml, self, req, self->m_name ) == false )
        return nullptr;
    return self;
}

std::string Label::schema( const std::string& tableName, uint32_t )
{
    if ( tableName == FileRelationTable::Name )
    {
        return "CREATE TABLE IF NOT EXISTS " + FileRelationTable::Name +
        "("
            "label_id INTEGER,"
            "media_id INTEGER,"
            "PRIMARY KEY(label_id,media_id),"
            "FOREIGN KEY(label_id) "
                "REFERENCES " + Table::Name + "(id_label) ON DELETE CASCADE,"
            "FOREIGN KEY(media_id) "
                "REFERENCES " + Media::Table::Name + "(id_media) ON DELETE CASCADE"
        ")";
    }
    assert( tableName == Table::Name );
    return "CREATE TABLE IF NOT EXISTS " + Table::Name +
    "("
        "id_label INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT UNIQUE ON CONFLICT FAIL"
    ")";
}

void Label::createTable( sqlite::Connection* dbConnection )
{
    const std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
        schema( FileRelationTable::Name, Settings::DbModelVersion ),
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConnection, req );
}

void Label::createTriggers( sqlite::Connection* dbConnection )
{
    const std::string ftsTrigger = "CREATE TRIGGER IF NOT EXISTS delete_label_fts "
            "BEFORE DELETE ON " + Label::Table::Name +
            " BEGIN"
            " UPDATE " + Media::Table::Name + "Fts SET labels = TRIM(REPLACE(labels, old.name, ''))"
            " WHERE labels MATCH old.name;"
            " END";
    sqlite::Tools::executeRequest( dbConnection, ftsTrigger );
}

}
