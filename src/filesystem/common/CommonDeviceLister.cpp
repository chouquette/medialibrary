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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "CommonDeviceLister.h"

#include "logging/Logger.h"

#include <algorithm>

#include <cassert>

namespace medialibrary
{
namespace fs
{

void CommonDeviceLister::refresh()
{
    /* We need the device lister to be started before refreshing anything */
    assert( m_cb != nullptr );

    auto newDeviceList = devices();
    for ( auto knownDeviceIt = begin( m_knownDevices );
          knownDeviceIt != end( m_knownDevices ); )
    {
        auto& knownDevice = *knownDeviceIt;
        auto newDeviceIt = std::find_if( begin( newDeviceList ), end( newDeviceList ),
                                         [&knownDevice]( const Device& d ) {
            return d.uuid == knownDevice.uuid;
        });
        if ( newDeviceIt == end( newDeviceList ) )
        {
            /* A previously known device was removed */
            for ( const auto& mountpoint : knownDevice.mountpoints )
                m_cb->onDeviceUnmounted( knownDevice.uuid, mountpoint );
            knownDeviceIt = m_knownDevices.erase( knownDeviceIt );
            continue;
        }
        /* The device still exists, check its mountpoint */
        auto& newDevice = *newDeviceIt;
        for ( auto knownMountpointIt = begin( knownDevice.mountpoints );
              knownMountpointIt != end( knownDevice.mountpoints ); )
        {
            const auto& knownMountpoint = *knownMountpointIt;
            auto newMountpointIt = std::find( begin( newDevice.mountpoints ),
                                              end( newDevice.mountpoints ),
                                              knownMountpoint );
            if ( newMountpointIt == end( newDevice.mountpoints ) )
            {
                /* The device still exists but lost a mountpoint */
                m_cb->onDeviceUnmounted( knownDevice.uuid, knownMountpoint );
                knownMountpointIt = knownDevice.mountpoints.erase( knownMountpointIt );
                continue;
            }
            /*
             * The mountpoint is still there, remove it from the list of current
             * mountpoints to detect new ones at the end of the loop
             */
            newDevice.mountpoints.erase( newMountpointIt );
            ++knownMountpointIt;
        }
        for ( auto newMountpointIt = begin( newDevice.mountpoints );
              newMountpointIt != end( newDevice.mountpoints ); ++newMountpointIt )
        {
            knownDevice.mountpoints.push_back( std::move( *newMountpointIt ) );
            m_cb->onDeviceMounted( knownDevice.uuid,
                                   *knownDevice.mountpoints.rbegin(),
                                   knownDevice.removable );
        }
        newDeviceList.erase( newDeviceIt );
        ++knownDeviceIt;
    }
    /*
     * Devices leftover in newDevices are entirely new devices which we now need
     * to signal about, and propagate to the list of known devices for later refresh
     */
    for ( auto& newDevice : newDeviceList )
    {
        m_knownDevices.push_back( std::move( newDevice ) );
        const auto& insertedNewDevice = *m_knownDevices.rbegin();
        for ( const auto& mp : insertedNewDevice.mountpoints )
        {
            m_cb->onDeviceMounted( insertedNewDevice.uuid, mp,
                                   insertedNewDevice.removable );
        }
    }
}

bool CommonDeviceLister::start( IDeviceListerCb* cb )
{
    m_cb = cb;
    return true;
}

void CommonDeviceLister::stop()
{
}

}
}

