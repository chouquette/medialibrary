/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright © 2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "LibvlcCacher.h"

#include "utils/VLCInstance.h"
#include "utils/Defer.h"
#include "logging/Logger.h"

#include <vlcpp/vlc.hpp>

namespace medialibrary
{

LibvlcCacher::LibvlcCacher()
    : m_currentMp( nullptr )
{
}

bool LibvlcCacher::cache( const std::string& inputMrl,
                          const std::string& outputPath )
{
    LOG_INFO( "Caching ", inputMrl, " to ", outputPath );

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    VLC::Media m{ inputMrl, VLC::Media::FromLocation };
    m.addOption( ":demux=dump" );
    m.addOption( ":demuxdump-file=" + outputPath );
    VLC::MediaPlayer mp{ VLCInstance::get(), m };
#else
    VLC::Media m{ VLCInstance::get(), inputMrl, VLC::Media::FromLocation };
    m.addOption( ":demux=dump" );
    m.addOption( ":demuxdump-file=" + outputPath );
    VLC::MediaPlayer mp{ m };
#endif

    std::unique_lock<compat::Mutex> lock{ m_mutex };
    m_currentMp = &mp;
    auto done = false;
    auto success = false;

    auto d = utils::make_defer([this]() {
        /* /!\ The lock is already held /!\ */
        stop();
        m_currentMp = nullptr;
    });
    auto em = mp.eventManager();
    em.onStopped([this, &done, &success](){
        std::unique_lock<compat::Mutex> lock{ m_mutex };
        done = true;
        success = true;
        m_cond.notify_one();
    });
    em.onEncounteredError([this, &done](){
        std::unique_lock<compat::Mutex> lock{ m_mutex };
        done = true;
        m_cond.notify_one();
    });

    mp.play();

    m_cond.wait( lock, [&done]() {
        return done;
    });

    return success;
}

void LibvlcCacher::interrupt()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    stop();
}

void LibvlcCacher::stop()
{
    if ( m_currentMp != nullptr )
    {
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        m_currentMp->stopAsync();
#else
        m_currentMp->stop();
#endif
    }
}

}
