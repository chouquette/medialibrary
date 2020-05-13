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

#include <tuple>
#include <vector>
#include <string>

namespace medialibrary
{

/**
 * @brief IDeviceListerCb is intended for external device listers to signal device modifications
 *
 * An external device lister shall only be used when the medialibrary can't list
 * the devices itself.
 * The device/folder/file management will still be the medialibrary's responsability
 */
class IDeviceListerCb
{
public:
    virtual ~IDeviceListerCb() = default;
    /**
     * @brief onDeviceMounted Shall be invoked when a known device gets mounted
     * @param uuid The device UUID
     * @param mountpoint The device new mountpoint
     * @param removable The removable state of the mounted device
     */
    virtual void onDeviceMounted( const std::string& uuid,
                                  const std::string& mountpoint,
                                  bool removable ) = 0;
    /**
     * @brief onDeviceUnmounted Shall be invoked when a known device gets unmounted
     * @param uuid The device UUID
     * @param mountpoint The mountpoint the device was mounted on
     */
    virtual void onDeviceUnmounted( const std::string& uuid,
                                    const std::string& mountpoint ) = 0;
};

class IDeviceLister
{
public:
    virtual ~IDeviceLister() = default;
    /**
     * @brief refresh Force a device refresh
     *
     * Implementation that solely rely on callback can implement this as a no-op
     * as long as they are guaranteed to invoke onDeviceMounted &
     * onDeviceUnmounted as soon as the information is available.
     */
    virtual void refresh() = 0;
    /**
     * @brief start Starts watching for new device
     * @param cb An IDeviceListerCb implementation to invoke upon device changes
     * @return true in case of success, false otherwise
     */
    virtual bool start( IDeviceListerCb* cb ) = 0;
    /**
     * @brief stop Stop watching for new device
     */
    virtual void stop() = 0;
};
}
