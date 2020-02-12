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

#include "medialibrary/IDeviceLister.h"

#include <unordered_map>

namespace medialibrary
{
namespace fs
{

class DeviceLister : public IDeviceLister
{
private:
    struct Device
    {
        Device( std::string u, std::vector<std::string> m, bool r )
            : uuid( std::move( u ) )
            , mountpoints( std::move( m ) )
            , removable( r )
        {
        }
        std::string uuid;
        std::vector<std::string> mountpoints;
        bool removable;
    };

    // Device name / UUID map
    using DeviceMap = std::unordered_map<std::string, std::string>;
    // Device path / Mountpoints map
    using MountpointMap = std::unordered_map<std::string,
                                             std::vector<std::string>>;

    DeviceMap listDevices() const;
    MountpointMap listMountpoints() const;
    std::pair<std::string, std::string> deviceFromDeviceMapper( const std::string& devicePath ) const;
    std::vector<Device> devices() const;

    bool isRemovable( const std::string& devicePath ) const;

    std::vector<std::string> getAllowedFsTypes() const;

    virtual void refresh() override;
    virtual bool start( IDeviceListerCb* ) override;
    virtual void stop() override;

private:
    IDeviceListerCb* m_cb = nullptr;
    std::vector<Device> m_knownDevices;
};

}
}
