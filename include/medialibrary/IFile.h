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

class IFile
{
public:
    /**
     * @brief Describes the type of a file
     * @warning These values are stored in DB. As such, any new value must be
     *          appended, as modifying the existing values would invalidate any
     *          existing db record.
     */
    enum class Type
    {
        /// Unknown type, so far
        Unknown,
        /// The main file of a media.
        Main,
        /// A part of a media (for instance, the first half of a movie)
        Part,
        /// External soundtrack
        Soundtrack,
        /// External subtitles
        Subtitles,
        /// A playlist File
        Playlist,
        /// A disc file. Also considered to be a "main" file
        Disc,
    };

    virtual ~IFile() = default;
    virtual int64_t id() const = 0;
    /**
     * @brief mrl Returns the full mrl for this file.
     * Since we can't compute an mrl for a file or folder that is/was present on
     * a removable storage or network share that is not mounted, a
     * fs::DeviceRemovedException will be thrown when trying to get the mrl of
     * a non present file.
     * You should always account for this exception is isRemovable returns true.
     * If for some reasons we can't compute the MRL, an empty string will be returned
     * @return The folder's mrl
     */
    virtual const std::string& mrl() const = 0;
    virtual Type type() const = 0;
    virtual time_t lastModificationDate() const = 0;
    virtual int64_t size() const = 0;
    virtual bool isRemovable() const = 0;
    ///
    /// \brief isExternal returns true if this stream isn't managed by the medialibrary
    ///
    virtual bool isExternal() const = 0;
    /**
     * @brief isNetwork returns true if this file is on a network location
     *
     * If the file is external, this is a best guess effort.
     */
    virtual bool isNetwork() const = 0;
    /**
     * @brief isMain Returns true if this file is the main file of a media
     *
     * This can be used to have a Disc file considered as the main file
     */
    virtual bool isMain() const = 0;
};

}
