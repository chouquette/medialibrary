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

#include <vlcpp/vlc.hpp>

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
    VLC::Media vlcMedia{ VLCInstance::get(), mrl, VLC::Media::FromType::FromLocation };
    auto em = vlcMedia.eventManager();

    compat::ConditionVariable cond;
    compat::Mutex lock;
    auto done = false;
    VLC::Picture thumbnail;

    em.onThumbnailGenerated([&cond, &lock, &thumbnail, &done]( const VLC::Picture* p ) {
        {
            std::unique_lock<compat::Mutex> l( lock );
            if( p != nullptr )
                thumbnail = *p;
            done = true;
        }
        cond.notify_all();
    });
    {
        std::unique_lock<compat::Mutex> l( lock );
        auto request = vlcMedia.thumbnailRequestByPos( position, VLC::Media::ThumbnailSeekSpeed::Fast,
                                                       desiredWidth, desiredHeight, true,
                                                       VLC::Picture::Type::Jpg, 3000 );
        if ( request == nullptr )
            return false;
        cond.wait( l, [&done]() { return done == true; } );
    }
    if ( thumbnail.isValid() == false )
        return false;

    return thumbnail.save( dest );
}

}
