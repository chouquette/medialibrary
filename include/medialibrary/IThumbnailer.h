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

#include "medialibrary/Types.h"

namespace medialibrary
{

class IThumbnailer
{
public:
    virtual ~IThumbnailer() = default;
    /**
     * @brief request Generate a thumbnail for the provided media
     * @param media A reference to the media a thumbnail is being generated for
     * @param mrl The mrl to the main file for this media
     * @param desiredWidth The desired thumbnail width
     * @param desiredHeight The desired thumbnail height
     * @param position The position at which to generate the thumbnail, in the [0;1] range
     * @param destination The path in which to save the thumbnail
     *
     * This method is expected to be blocking until the generation is
     * successful or stopped.
     * Upon successful return, the <destination> path must contain the generated
     * thumbnail
     */
    virtual bool generate( const IMedia& media, const std::string& mrl,
                           uint32_t desiredWidth, uint32_t desiredHeight,
                           float position, const std::string& destination ) = 0;
    /**
     * @brief stop Stop any ongoing processing as soon as possible
     */
    virtual void stop() = 0;
};

}
