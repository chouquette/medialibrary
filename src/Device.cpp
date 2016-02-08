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

#include "Device.h"

const std::string policy::DeviceTable::Name = "Device";
const std::string policy::DeviceTable::PrimaryKeyColumn = "id_device";
unsigned int Device::* const policy::DeviceTable::PrimaryKey = &Device::m_id;

Device::Device(MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    row >> m_id
        >> m_uuid
        >> m_isRemovable
        >> m_isPresent;
    //FIXME: It's probably a bad idea to load "isPresent" for DB. This field should
    //only be here for sqlite triggering purposes
}

Device::Device( const std::string& uuid, bool isRemovable )
    : m_uuid( uuid )
    , m_isRemovable( isRemovable )
    // Assume we can't add an absent device
    , m_isPresent( true )
{
}

unsigned int Device::id() const
{
    return m_id;
}

const std::string&Device::uuid() const
{
    return m_uuid;
}

bool Device::isRemovable() const
{
    return m_isRemovable;
}

bool Device::isPresent() const
{
    return m_isPresent;
}

void Device::setPresent(bool value)
{
    static const std::string req = "UPDATE " + policy::DeviceTable::Name +
            " SET is_present = ? WHERE id_device = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, value, m_id ) == false )
        return;
    m_isPresent = value;
}

std::shared_ptr<Device> Device::create( MediaLibraryPtr ml, const std::string& uuid, bool isRemovable )
{
    static const std::string req = "INSERT INTO " + policy::DeviceTable::Name
            + "(uuid, is_removable, is_present) VALUES(?, ?, ?)";
    auto self = std::make_shared<Device>( uuid, isRemovable );
    if ( insert( ml, self, req, uuid, isRemovable, self->isPresent() ) == false )
        return nullptr;
    self->m_ml = ml;
    return self;
}

bool Device::createTable(DBConnection connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::DeviceTable::Name + "("
                "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
                "uuid TEXT UNIQUE ON CONFLICT FAIL,"
                "is_removable BOOLEAN,"
                "is_present BOOLEAN"
            ")";
    return sqlite::Tools::executeRequest( connection, req );
}

std::shared_ptr<Device> Device::fromUuid( MediaLibraryPtr ml, const std::string& uuid )
{
    static const std::string req = "SELECT * FROM " + policy::DeviceTable::Name +
            " WHERE uuid = ?";
    return fetch( ml, req, uuid );
}

