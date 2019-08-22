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

#pragma once

#include "Types.h"
#include "database/DatabaseHelpers.h"
#include <chrono>

namespace medialibrary
{

class Device;

class Device : public DatabaseHelpers<Device>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Device::*const PrimaryKey;
    };
    Device( MediaLibraryPtr ml, const std::string& uuid, const std::string& scheme,
            bool isRemovable, time_t insertionDate );
    Device( MediaLibraryPtr ml, sqlite::Row& row );
    int64_t id() const;
    const std::string& uuid() const;
    bool isRemovable() const;
    ///
    /// \brief forceNonRemovable This is used only to fix an invalid DB state
    /// \return
    ///
    bool forceNonRemovable();
    bool isPresent() const;
    void setPresent( bool value );
    ///
    /// \brief scheme returns the scheme that was used for this device when it was
    /// originally created. This allows to use the apropriate IFileSystemFactory to find the
    /// recreate a IDevice based on its id or UUID
    /// \return
    ///
    const std::string& scheme() const;
    void updateLastSeen();

    static std::shared_ptr<Device> create( MediaLibraryPtr ml, const std::string& uuid, const std::string& scheme, bool isRemovable );
    static void createTable( sqlite::Connection* connection );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static std::shared_ptr<Device> fromUuid( MediaLibraryPtr ml, const std::string& uuid );
    static void removeOldDevices( MediaLibraryPtr ml, std::chrono::seconds maxLifeTime );
    static std::vector<std::shared_ptr<Device>> fetchByScheme( MediaLibraryPtr ml, const std::string& scheme );

private:
    MediaLibraryPtr m_ml;
    // This is a database ID
    int64_t m_id;
    // This is a unique ID on the system side, in the /dev/disk/by-uuid sense.
    // It can be a name or what not, depending on the OS.
    const std::string m_uuid;
    const std::string m_scheme;
    // Can't be const anymore, but should be if we ever get to remove the
    // removable->non removable device fixup (introduced after vlc-android 3.1.0 rc3)
    bool m_isRemovable;
    bool m_isPresent;
    time_t m_lastSeen;

    friend struct Device::Table;
};

}
