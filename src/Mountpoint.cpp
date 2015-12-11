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

#include "Mountpoint.h"

const std::string policy::MountpointTable::Name = "Mountpoint";
const std::string policy::MountpointTable::PrimaryKeyColumn = "id_mountpoint";
unsigned int Mountpoint::* const policy::MountpointTable::PrimaryKey = &Mountpoint::m_id;

Mountpoint::Mountpoint( DBConnection dbConnection, sqlite::Row& row )
    : m_dbConn( dbConnection )
{
    row >> m_id
        >> m_uuid
        >> m_isRemovable
        >> m_isPresent;
    //FIXME: It's probably a bad idea to load "isPresent" for DB. This field should
    //only be here for sqlite triggering purposes
}

Mountpoint::Mountpoint( const std::string& uuid, bool isRemovable )
    : m_uuid( uuid )
    , m_isRemovable( isRemovable )
    // Assume we can't add an unmounted/absent mountpoint
    , m_isPresent( true )
{
}

unsigned int Mountpoint::id() const
{
    return m_id;
}

const std::string&Mountpoint::uuid() const
{
    return m_uuid;
}

bool Mountpoint::isRemovable() const
{
    return m_isRemovable;
}

bool Mountpoint::isPresent() const
{
    return m_isPresent;
}

void Mountpoint::setPresent(bool value)
{
    static const std::string req = "UPDATE " + policy::MountpointTable::Name +
            " SET is_present = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConn, req, value ) == false )
        return;
    m_isPresent = value;
}

std::shared_ptr<Mountpoint> Mountpoint::create(DBConnection dbConnection, const std::string& uuid, bool isRemovable )
{
    static const std::string req = "INSERT INTO " + policy::MountpointTable::Name
            + "(uuid, is_removable, is_present) VALUES(?, ?, ?)";
    auto self = std::make_shared<Mountpoint>( uuid, isRemovable );
    if ( insert( dbConnection, self, req, uuid, isRemovable, self->isPresent() ) == false )
        return nullptr;
    self->m_dbConn = dbConnection;
    return self;
}

bool Mountpoint::createTable(DBConnection connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::MountpointTable::Name + "("
                "id_mountpoint INTEGER PRIMARY KEY AUTOINCREMENT,"
                "uuid TEXT UNIQUE ON CONFLICT FAIL,"
                "is_removable BOOLEAN,"
                "is_present BOOLEAN"
            ")";
    return sqlite::Tools::executeRequest( connection, req );
}

std::shared_ptr<Mountpoint> Mountpoint::fromUuid( DBConnection dbConnection, const std::string& uuid )
{
    static const std::string req = "SELECT * FROM " + policy::MountpointTable::Name +
            " WHERE uuid = ?";
    return fetch( dbConnection, req, uuid );
}

