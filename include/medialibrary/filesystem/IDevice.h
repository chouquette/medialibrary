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

#include <string>
#include <tuple>
#include <vector>

namespace medialibrary
{

namespace fs
{

class IDevice
{
public:
    virtual ~IDevice() = default;
    virtual const std::string& uuid() const = 0;
    /**
     * @brief scheme Returns this device scheme
     *
     * Since if the device has no remaining mountpoint, this getter can be used
     * to know what scheme this device is using at any time.
     */
    virtual const std::string& scheme() const = 0;
    virtual bool isRemovable() const = 0;
    virtual bool isPresent() const = 0;
    virtual bool isNetwork() const = 0;
    /**
     * @brief mountpoint Returns a mountpoint of this device.
     *
     * If the device has multiple mountpoints, the result is undetermined
     */
    virtual std::vector<std::string> mountpoints() const = 0;

    virtual void addMountpoint( std::string mountpoint ) = 0;
    virtual void removeMountpoint( const std::string& mountpoint ) = 0;
    /**
     * @brief matchesMountpoint checks if the provided mrl matches this device
     * @param mrl The mrl to probe
     * @return A tuple containing:
     *  - a bool, that reflects the match status (ie. true if the device matches the mrl)
     *  - a string, containing the mountpoint that matched, or an empty string
     *    if the device did not match
     */
    virtual std::tuple<bool, std::string>
    matchesMountpoint( const std::string& mrl ) const = 0;
    /**
     * @brief relativeMrl Returns an mrl relative to the device mountpoint
     * @param absoluteMrl The absolute MRL pointing to a file or folder, including
     *                    the scheme
     * @return An scheme-less MRL
     */
    virtual std::string relativeMrl( const std::string& absoluteMrl ) const = 0;
    /**
     * @brief absoluteMrl Returns an absolute mrl to the provided file or folder
     * @param relativeMrl A relative MRL pointing to a file or folder, *not*
     *                    including the scheme
     * @return An absolute MRL, including the scheme
     *
     * If the device has multiple mountpoints, the resulting mrl is undetermined
     * but guaranteed to yield the same device back when using
     * IFileSystemFactory::createDeviceFromMrl
     */
    virtual std::string absoluteMrl( const std::string& relativeMrl ) const = 0;
};
}

}
