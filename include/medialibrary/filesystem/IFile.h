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

namespace fs
{
    class IFile
    {
    public:
        enum class LinkedFileType : uint8_t
        {
            ///< This file is not a linked file
            None,
            ///< This is a linked subtitle file
            Subtitles,
            ///< This is a linked soundtrack file
            SoundTrack,
        };
        virtual ~IFile() = default;
        /// Returns the URL encoded filename, including the extension
        virtual const std::string& name() const = 0;
        /// Returns the mrl of this file
        virtual const std::string& mrl() const = 0;
        virtual const std::string& extension() const = 0;
        virtual time_t lastModificationDate() const = 0;
        virtual int64_t size() const = 0;
        virtual bool isNetwork() const = 0;
        /**
         * @brief type Returns the file type, or None if not linked with another file
         */
        virtual LinkedFileType linkedType() const = 0;
        /**
         * @brief linkedWith Return the MRL this file is linked to, or an empty
         *                   string if it's not linked with anything
         *
         * If type() is None, it's invalid to call this function
         */
        virtual const std::string& linkedWith() const = 0;
    };
}

}
