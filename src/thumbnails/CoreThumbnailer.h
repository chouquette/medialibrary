/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Types.h"
#include "medialibrary/IThumbnailer.h"
#include "compat/Mutex.h"

#include <vlcpp/vlc.hpp>

namespace medialibrary
{

class CoreThumbnailer : public IThumbnailer
{
public:
    CoreThumbnailer();

    virtual bool generate( const IMedia& media, const std::string& mrl,
                           uint32_t desiredWidth, uint32_t desiredHeight,
                           float position, const std::string& dest ) override;
    virtual void stop() override;

private:
    compat::Mutex m_mutex;
    VLC::Media m_vlcMedia;
    VLC::Media::ThumbnailRequest* m_request;
};

}
