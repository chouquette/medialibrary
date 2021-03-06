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

#include "filesystem/common/CommonDeviceLister.h"

#include <unordered_map>

namespace medialibrary
{
namespace fs
{

class DeviceLister : public CommonDeviceLister
{
private:
    // Device name / UUID map
    using DeviceMap = std::unordered_map<std::string, std::string>;
    // Device path / Mountpoints map
    using MountpointMap = std::unordered_map<std::string,
                                             std::vector<std::string>>;

    DeviceMap listDevices() const;
    MountpointMap listMountpoints() const;
    std::pair<std::string, std::string> deviceFromDeviceMapper( const std::string& devicePath ) const;
    virtual std::vector<Device> devices() const override;

    bool isRemovable( const std::string& devicePath ) const;

    std::vector<std::string> getAllowedFsTypes() const;

private:
};

}
}
