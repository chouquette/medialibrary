/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "CoreThumbnailer.h"

#include "compat/ConditionVariable.h"
#include "compat/Mutex.h"
#include "Media.h"
#include "MediaLibrary.h"
#include "utils/VLCInstance.h"

namespace medialibrary
{

CoreThumbnailer::CoreThumbnailer( MediaLibraryPtr ml )
    : m_ml( ml )
{
}

bool CoreThumbnailer::generate( const std::string& mrl, uint32_t desiredWidth,
                                uint32_t desiredHeight, float position,
                                const std::string& dest )
{
    compat::ConditionVariable cond;
    auto done = false;
    VLC::Picture thumbnail;
    {
        std::unique_lock<compat::Mutex> l{ m_mutex };

        m_vlcMedia = VLC::Media{ VLCInstance::get(), mrl, VLC::Media::FromType::FromLocation };
        auto em = m_vlcMedia.eventManager();

        em.onThumbnailGenerated([this, &cond, &thumbnail, &done]( const VLC::Picture* p ) {
            {
                std::unique_lock<compat::Mutex> l{ m_mutex };
                if( p != nullptr )
                    thumbnail = *p;
                done = true;
            }
            cond.notify_all();
        });
        m_request = m_vlcMedia.thumbnailRequestByPos( position, VLC::Media::ThumbnailSeekSpeed::Fast,
                                                       desiredWidth, desiredHeight, true,
                                                       VLC::Picture::Type::Jpg, 3000 );
        if ( m_request == nullptr )
        {
            m_vlcMedia = VLC::Media{};
            return false;
        }
        cond.wait( l, [&done]() { return done == true; } );

        m_request = nullptr;
        m_vlcMedia = VLC::Media{};
    }
    if ( thumbnail.isValid() == false )
        return false;

    return thumbnail.save( dest );
}

void CoreThumbnailer::stop()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    if ( m_request != nullptr )
        m_vlcMedia.thumbnailCancel( m_request );
}

}
