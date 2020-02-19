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
#include "medialibrary/filesystem/IDevice.h"

#include <vlcpp/vlc.hpp>

namespace medialibrary
{

namespace fs
{
namespace libvlc
{

class DeviceLister : public IDeviceLister
{
public:
    DeviceLister( const std::string& protocol, const std::string& sdName );
    virtual ~DeviceLister() = default;

    virtual void refresh() override;
    virtual bool start( IDeviceListerCb* cb ) override;
    virtual void stop() override;

private:
    void onDeviceAdded( VLC::MediaPtr media );
    void onDeviceRemoved( VLC::MediaPtr media );

private:
    std::string m_protocol;
    VLC::MediaDiscoverer m_discoverer;
    std::shared_ptr<VLC::MediaList> m_mediaList;
    IDeviceListerCb* m_cb;
};

}
}
}
