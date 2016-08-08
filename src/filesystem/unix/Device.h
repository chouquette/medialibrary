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

#include "filesystem/IDevice.h"
#include "medialibrary/Types.h"
#include "utils/Cache.h"

#include <memory>
#include <unordered_map>

namespace medialibrary
{

namespace fs
{

class Device : public IDevice
{
    // UUID -> Device instance map
    using DeviceCacheMap = std::unordered_map<std::string, std::shared_ptr<IDevice>>;

public:
    virtual const std::string& uuid() const override;
    virtual bool isRemovable() const override;
    virtual bool isPresent() const override;
    virtual const std::string& mountpoint() const override;

    ///
    /// \brief fromPath Returns the device that contains the given path
    ///
    static std::shared_ptr<IDevice> fromPath( const std::string& path );
    static std::shared_ptr<IDevice> fromUuid( const std::string& uuid );

    static void setDeviceLister( DeviceListerPtr lister );
    static void refreshDeviceCache();

protected:
    Device( const std::string& uuid, const std::string& mountpoint, bool isRemovable );

private:
    static void refreshDeviceCacheLocked();

private:
    static Cache<DeviceCacheMap> DeviceCache;
    static DeviceListerPtr DeviceLister;

private:
    std::string m_uuid;
    std::string m_mountpoint;
    bool m_present;
    bool m_removable;
};

}

}
