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

#include <string>

namespace medialibrary
{

class IFile
{
public:
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
    };

    virtual ~IFile() = default;
    virtual int64_t id() const = 0;
    virtual const std::string& mrl() const = 0;
    virtual Type type() const = 0;
    virtual unsigned int lastModificationDate() const = 0;
    virtual unsigned int size() const = 0;
    virtual bool isRemovable() const = 0;
    ///
    /// \brief isExternal returns true if this stream isn't managed by the medialibrary
    ///
    virtual bool isExternal() const = 0;
};

}
