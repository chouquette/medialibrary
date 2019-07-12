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

#include <string>

namespace medialibrary
{

class IImageCompressor
{
public:
    virtual ~IImageCompressor() = default;
    /**
     * @return
     */
    virtual const char* extension() const = 0;
    /**
     * @return Returns the desired input fourcc for this compressor
     */
    virtual const char* fourCC() const = 0;
    virtual uint32_t bpp() const = 0;
    virtual bool compress( const uint8_t* buffer, const std::string& output,
                           uint32_t inputWidth, uint32_t inputHeight,
                           uint32_t outputWidth, uint32_t outputHeight,
                           uint32_t hOffset, uint32_t vOffset ) = 0;
};

}
