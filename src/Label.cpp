/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "Label.h"
#include "Media.h"
#include "database/SqliteTools.h"

const std::string policy::LabelTable::Name = "Label";
const std::string policy::LabelTable::CacheColumn = "name";
unsigned int Label::* const policy::LabelTable::PrimaryKey = &Label::m_id;

Label::Label( DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_name;
}

Label::Label( const std::string& name )
    : m_id( 0 )
    , m_name( name )
{
}

unsigned int Label::id() const
{
    return m_id;
}

const std::string& Label::name() const
{
    return m_name;
}

std::vector<MediaPtr> Label::files()
{
    static const std::string req = "SELECT f.* FROM " + policy::MediaTable::Name + " f "
            "LEFT JOIN LabelFileRelation lfr ON lfr.id_media = f.id_media "
            "WHERE lfr.id_label = ?";
    return Media::fetchAll( m_dbConnection, req, m_id );
}

LabelPtr Label::create(DBConnection dbConnection, const std::string& name )
{
    auto self = std::make_shared<Label>( name );
    const char* req = "INSERT INTO Label VALUES(NULL, ?)";
    if ( _Cache::insert( dbConnection, self, req, self->m_name ) == false )
        return nullptr;
    self->m_dbConnection = dbConnection;
    return self;
}

bool Label::createTable(DBConnection dbConnection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::LabelTable::Name + "("
                "id_label INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT UNIQUE ON CONFLICT FAIL"
            ")";
    if ( sqlite::Tools::executeRequest( dbConnection, req ) == false )
        return false;
    req = "CREATE TABLE IF NOT EXISTS LabelFileRelation("
                "id_label INTEGER,"
                "id_media INTEGER,"
            "PRIMARY KEY (id_label, id_media),"
            "FOREIGN KEY(id_label) REFERENCES Label(id_label) ON DELETE CASCADE,"
            "FOREIGN KEY(id_media) REFERENCES Media(id_media) ON DELETE CASCADE);";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

const std::string&policy::LabelCachePolicy::key( const ILabel* self )
{
    return self->name();
}

std::string policy::LabelCachePolicy::key(sqlite::Row& row)
{
    return row.load<KeyType>( 1 );
}
