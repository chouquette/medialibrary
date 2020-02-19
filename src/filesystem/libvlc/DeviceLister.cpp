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

#include "filesystem/libvlc/DeviceLister.h"
#include "utils/VLCInstance.h"
#include "utils/Filename.h"
#include "filesystem/libvlc/Device.h"

#include "logging/Logger.h"

namespace medialibrary
{
namespace fs
{
namespace libvlc
{

DeviceLister::DeviceLister( const std::string &protocol,
                                          const std::string &sdName )
    : m_protocol( protocol )
    , m_discoverer( VLCInstance::get(), sdName )
    , m_mediaList( m_discoverer.mediaList() )
    , m_cb( nullptr )
{
    auto& em = m_mediaList->eventManager();
    em.onItemAdded( [this]( VLC::MediaPtr m, int ) { onDeviceAdded( std::move( m ) ); } );
    em.onItemDeleted( [this]( VLC::MediaPtr m, int ) { onDeviceRemoved( std::move( m ) ); } );
}

void DeviceLister::refresh()
{
    /*
     * We are continuously refreshing through libvlc's discoverer so there's
     * nothing particular to do here
     */
}

bool DeviceLister::start( IDeviceListerCb *cb )
{
    m_cb = cb;
    return m_discoverer.start();
}

void DeviceLister::stop()
{
    m_discoverer.stop();
}

void DeviceLister::onDeviceAdded( VLC::MediaPtr media )
{
    const auto& mrl = media->mrl();
    assert( utils::file::scheme( mrl ) == m_protocol );

    auto uuid = utils::file::stripScheme( mrl );
    LOG_ERROR( "Mountpoint added: ", mrl, " from device ", uuid );
    m_cb->onDeviceMounted( uuid, utils::file::toFolderPath( mrl ), true );
}

void DeviceLister::onDeviceRemoved( VLC::MediaPtr media )
{
    const auto& mrl = media->mrl();
    assert( utils::file::scheme( mrl ) == m_protocol );

    auto uuid = utils::file::stripScheme( mrl );
    LOG_ERROR( "Mountpoint removed: ", mrl, " from device ", uuid );

    m_cb->onDeviceUnmounted( uuid, utils::file::toFolderPath( mrl ) );
}


}
}
}
