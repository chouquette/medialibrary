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

#pragma once

#include "Types.h"
#include "database/DatabaseHelpers.h"

class Mountpoint;

namespace policy
{
struct MountpointTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static unsigned int Mountpoint::*const PrimaryKey;
};
}

class Mountpoint : public DatabaseHelpers<Mountpoint, policy::MountpointTable>
{
public:
    Mountpoint( const std::string& uuid, bool isRemovable );
    Mountpoint( DBConnection dbConnection, sqlite::Row& row );
    unsigned int id() const;
    const std::string& uuid() const;
    bool isRemovable() const;
    bool isPresent() const;
    void setPresent( bool value );

    static std::shared_ptr<Mountpoint> create( DBConnection dbConnection, const std::string& uuid, bool isRemovable );
    static bool createTable( DBConnection connection );
    static std::shared_ptr<Mountpoint> fromUuid( DBConnection dbConnection, const std::string& uuid );

private:
    DBConnection m_dbConn;
    // This is a database ID
    unsigned int m_id;
    // This is a unique ID on the system side, in the /dev/disk/by-uuid sense.
    // It can be a name or what not, depending on the OS.
    std::string m_uuid;
    bool m_isRemovable;
    bool m_isPresent;

    friend struct policy::MountpointTable;
};
