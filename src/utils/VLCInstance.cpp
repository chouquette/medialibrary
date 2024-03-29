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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef HAVE_LIBVLC
# error This file requires libvlc
#endif

#include "VLCInstance.h"
#include "logging/Logger.h"
#include "vlcpp/vlc.hpp"

#include <mutex>

namespace medialibrary
{

compat::Mutex VLCInstance::s_lock;
VLC::Instance VLCInstance::s_instance;
std::vector<VLCInstanceCb*> VLCInstance::s_cbs;

void VLCInstance::set( libvlc_instance_t* external_instance )
{
    std::lock_guard<compat::Mutex> lock{ s_lock };
    s_instance = VLC::Instance{ external_instance };
    for ( const auto cb : s_cbs )
        cb->onInstanceReplaced( s_instance );
}

bool VLCInstance::isSet()
{
    std::lock_guard<compat::Mutex> lock{ s_lock };
    return s_instance.isValid();
}

void VLCInstance::registerCb( VLCInstanceCb* cb )
{
    std::lock_guard<compat::Mutex> lock{ s_lock };
    s_cbs.push_back( cb );
}

void VLCInstance::unregisterCb( VLCInstanceCb* cb )
{
    std::lock_guard<compat::Mutex> lock{ s_lock };
    auto it = std::find( cbegin( s_cbs ), cend( s_cbs ), cb );
    if ( it == cend( s_cbs ) )
    {
        assert( !"Unregistering unregistered callback" );
        return;
    }
    s_cbs.erase( it );
}

VLC::Instance VLCInstance::get()
{
    std::lock_guard<compat::Mutex> lock{ s_lock };
    if ( s_instance.isValid() == false )
    {
        s_instance = VLC::Instance{ 0, nullptr };
    }
    return s_instance;
}

}
