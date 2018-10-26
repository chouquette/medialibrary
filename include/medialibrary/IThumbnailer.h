/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
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
     * @param media The media to generate a thumbnail for
     * @param mrl The mrl to the main file for this media
     *
     * This method is expected to be blocking until the generation is successful
     * If successful, the implementation must call IMedia::setThumbnail and pass
     * the mrl to the generated thumbnail.
     */
    virtual bool generate( MediaPtr media, const std::string& mrl ) = 0;
};

}
