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

#include "IMediaLibrary.h"

namespace medialibrary
{

class IAudioTrack
{
public:
    virtual ~IAudioTrack() = default;
    virtual int64_t id() const = 0;
    virtual const std::string& codec() const = 0;
    /**
     * @return The bitrate in bits per second
     */
    virtual unsigned int bitrate() const = 0;
    virtual unsigned int sampleRate() const = 0;
    virtual unsigned int nbChannels() const = 0;
    virtual const std::string& language() const = 0;
    virtual const std::string& description() const = 0;
    virtual bool isInAttachedFile() const = 0;
};

}
