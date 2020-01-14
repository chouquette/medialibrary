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
     * @returns true is the device was unknown. false otherwise
     */
    virtual bool onDeviceMounted( const std::string& uuid,
                                  const std::string& mountpoint ) = 0;
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
     * @brief devices Returns a tuple containing:
     * - The device UUID
     * - The device mountpoint
     * - A 'removable' state, being true if the device can be removed, false otherwise.
     */
    virtual std::vector<std::tuple<std::string, std::string, bool>> devices() const = 0;
};
}
