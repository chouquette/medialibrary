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

class Device : public DatabaseHelpers<Device>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Device::*const PrimaryKey;
    };
    struct MountpointTable
    {
        static const std::string Name;
    };

    Device(MediaLibraryPtr ml, std::string uuid, std::string scheme,
            bool isRemovable, bool isNetwork );
    Device( MediaLibraryPtr ml, sqlite::Row& row );
    int64_t id() const;
    const std::string& uuid() const;
    bool isRemovable() const;
    bool isPresent() const;
    void setPresent( bool value );
    bool isNetwork() const;
    ///
    /// \brief scheme returns the scheme that was used for this device when it was
    /// originally created. This allows to use the apropriate IFileSystemFactory to find the
    /// recreate a IDevice based on its id or UUID
    /// \return
    ///
    const std::string& scheme() const;
    void updateLastSeen();

    /**
     * @brief addMountpoint Stores a device mountpoint in database
     * @param mrl The mrl to that mountpoint
     * @return true if the insertion succeeded, false otherwise
     *
     * If the mountpoint was already known, the request is ignored
     */
    bool addMountpoint( const std::string& mrl, int64_t seenDate );


    /**
     * @brief cachedMountpoint Fetches a cached mountpoint from the database
     * @param ml A media library instance
     * @return A previously seen mountpoint for this device
     *
     * This is only valid for network devices.
     */
    std::string cachedMountpoint() const;

    static std::shared_ptr<Device> create( MediaLibraryPtr ml, std::string uuid,
                                           std::string scheme, bool isRemovable,
                                           bool isNetwork );
    static void createTable( sqlite::Connection* connection );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static std::shared_ptr<Device> fromUuid( MediaLibraryPtr ml, const std::string& uuid,
                                             const std::string& scheme );
    static void removeOldDevices( MediaLibraryPtr ml, std::chrono::seconds maxLifeTime );
    static std::vector<std::shared_ptr<Device>> fetchByScheme( MediaLibraryPtr ml, const std::string& scheme );
    static bool deleteRemovable( MediaLibraryPtr ml );
    /**
     * @brief markNetworkDeviceMissing Will mark all network devices as missing
     * @param ml A media library instance
     * @return true if the request successfully executed. False otherwise
     *
     * This is used to have a coherent state before the device lister are started.
     * We don't know what devices are present, so we mark them all as missing and
     * wait for the device lister to signal their presence
     */
    static bool markNetworkAsDeviceMissing( MediaLibraryPtr ml );
    /**
     * @brief fromMountpoint Returns a device matching the given mountpoint
     * @param ml A media library instance
     * @param mrl An mrl
     * @return A device instance if a match was found, nullptr otherwise.
     *
     * This will look at the mountpoints that were stored in database
     */
    static std::tuple<int64_t, std::string> fromMountpoint( MediaLibraryPtr ml,
                                                   const std::string& mrl );

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
    bool m_isNetwork;

    friend struct Device::Table;
};

}
