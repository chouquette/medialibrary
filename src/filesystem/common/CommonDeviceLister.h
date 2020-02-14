/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2020 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include <vector>
#include <string>

namespace medialibrary
{
namespace fs
{

class CommonDeviceLister : public IDeviceLister
{
protected:
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

public:
    virtual ~CommonDeviceLister() = default;

    virtual bool start( IDeviceListerCb* ) override;
    virtual void stop() override;
    virtual void refresh() override;

protected:
    virtual std::vector<Device> devices() const = 0;

private:
    IDeviceListerCb* m_cb = nullptr;
    std::vector<Device> m_knownDevices;
};

}
}
