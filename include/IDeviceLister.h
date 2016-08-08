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

#include <tuple>
#include <vector>

namespace medialibrary
{

class IDeviceListerCb
{
public:
    virtual ~IDeviceListerCb() = default;
    /**
     * @brief onDevicePlugged Shall be invoked when a known device gets plugged
     * @param uuid The device UUID
     * @param mountpoint The device new mountpoint
     */
    virtual void onDevicePlugged( const std::string& uuid, const std::string& mountpoint ) = 0;
    /**
     * @brief onDeviceUnplugged Shall be invoked when a known device gets unplugged
     * @param uuid The device UUID
     */
    virtual void onDeviceUnplugged( const std::string& uuid ) = 0;
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
     * @return
     */
    virtual std::vector<std::tuple<std::string, std::string, bool>> devices() const = 0;
};
}
