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

namespace medialibrary
{

class Device;

namespace policy
{
struct DeviceTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t Device::*const PrimaryKey;
};
}

class Device : public DatabaseHelpers<Device, policy::DeviceTable>
{
public:
    Device( MediaLibraryPtr ml, const std::string& uuid, const std::string& scheme, bool isRemovable );
    Device( MediaLibraryPtr ml, sqlite::Row& row );
    int64_t id() const;
    const std::string& uuid() const;
    bool isRemovable() const;
    bool isPresent() const;
    void setPresent( bool value );
    ///
    /// \brief scheme returns the scheme that was used for this device when it was
    /// originally created. This allows to use the apropriate IFileSystemFactory to find the
    /// recreate a IDevice based on its id or UUID
    /// \return
    ///
    const std::string& scheme() const;

    static std::shared_ptr<Device> create( MediaLibraryPtr ml, const std::string& uuid, const std::string& scheme, bool isRemovable );
    static bool createTable( DBConnection connection );
    static std::shared_ptr<Device> fromUuid( MediaLibraryPtr ml, const std::string& uuid );

private:
    MediaLibraryPtr m_ml;
    // This is a database ID
    int64_t m_id;
    // This is a unique ID on the system side, in the /dev/disk/by-uuid sense.
    // It can be a name or what not, depending on the OS.
    std::string m_uuid;
    std::string m_scheme;
    bool m_isRemovable;
    bool m_isPresent;

    friend struct policy::DeviceTable;
};

}
