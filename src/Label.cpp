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

#include "Label.h"
#include "Media.h"
#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"
#include "utils/Enums.h"

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
    static const std::string req = "FROM " + Media::Table::Name + " m "
            "INNER JOIN " + FileRelationTable::Name + " lfr ON lfr.entity_id = m.id_media "
            "WHERE lfr.label_id = ? "
            "AND lfr.entity_type = " + utils::enum_to_string( EntityType::Media );
    return make_query<Media, IMedia>( m_ml, "m.*", req, "", m_id ).build();
}

LabelPtr Label::create( MediaLibraryPtr ml, const std::string& name )
{
    auto self = std::make_shared<Label>( ml, name );
    const char* req = "INSERT INTO Label VALUES(NULL, ?)";
    if ( insert( ml, self, req, self->m_name ) == false )
        return nullptr;
    return self;
}

std::string Label::schema( const std::string& tableName, uint32_t dbModel )
{
    if ( tableName == FileRelationTable::Name )
    {
        if ( dbModel < 37 )
        {
            return "CREATE TABLE " + FileRelationTable::Name +
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
        return "CREATE TABLE " + FileRelationTable::Name +
               "("
                   "label_id INTEGER,"
                   "entity_id INTEGER,"
                   "entity_type INTEGER,"
                   "PRIMARY KEY(label_id,entity_id,entity_type),"
                   "FOREIGN KEY(label_id) "
                       "REFERENCES " + Table::Name + "(id_label) ON DELETE CASCADE"
               ")";
    }
    assert( tableName == Table::Name );
    return "CREATE TABLE " + Table::Name +
    "("
        "id_label INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT UNIQUE ON CONFLICT FAIL"
    ")";
}

std::string Label::trigger( Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
    case Triggers::DeleteFts:
        return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
               " BEFORE DELETE ON " + Table::Name +
               " BEGIN"
               " UPDATE " + Media::FtsTable::Name +
                    " SET labels = TRIM(REPLACE(labels, old.name, ''))"
                    " WHERE labels MATCH old.name;"
               " END";
    case Triggers::DeleteMediaLabel:
        return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
               " AFTER DELETE ON " + Media::Table::Name +
               " BEGIN"
                   " DELETE FROM " + FileRelationTable::Name +
                       " WHERE entity_type = " +
                           utils::enum_to_string( EntityType::Media ) +
                       " AND entity_id = old.id_media;"
               " END";
    }
    return "<invalid request>";
}

std::string Label::triggerName( Triggers trigger, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( dbModel );
    switch ( trigger )
    {
    case Triggers::DeleteFts:
        return "delete_label_fts";
    case Triggers::DeleteMediaLabel:
        assert( dbModel >= 37 );
        return "label_delete_media";
    }
    return "<invalid trigger>";
}

std::string Label::index( Label::Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::MediaId:
            assert( dbModel >= 34 );
            assert( dbModel < 37 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + FileRelationTable::Name + "(media_id)";
    }
    return "<invalid request>";
}

std::string Label::indexName( Label::Indexes index, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( dbModel );
    switch ( index )
    {
        case Indexes::MediaId:
            assert( dbModel >= 34 );
            assert( dbModel < 37 );
            return "label_rel_media_id_idx";
    }
    return "<invalid request>";
}

bool Label::checkDbModel( MediaLibraryPtr ml )
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );

    auto checkTrigger = []( Triggers t ){
        return sqlite::Tools::checkTriggerStatement(
              trigger( t, Settings::DbModelVersion ),
              triggerName( t, Settings::DbModelVersion ) );
    };

    return sqlite::Tools::checkTableSchema(
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) &&
           sqlite::Tools::checkTableSchema(
                                       schema( FileRelationTable::Name, Settings::DbModelVersion ),
                                       FileRelationTable::Name ) &&
           checkTrigger( Triggers::DeleteFts ) &&
           checkTrigger( Triggers::DeleteMediaLabel );
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
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::DeleteFts, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::DeleteMediaLabel, Settings::DbModelVersion ) );
}

}
