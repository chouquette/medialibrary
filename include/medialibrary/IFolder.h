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

class IFolder
{
public:
    virtual ~IFolder() = default;
    virtual int64_t id() const = 0;
    /**
     * @brief mrl Returns the full mrl for this folder.
     * Caller is responsible for checking isPresent() beforehand, as we
     * can't compute an for a folder that is/was present on a removable storage
     * or network share that has been unplugged
     * @return The folder's mrl
     */
    virtual const std::string& mrl() const = 0;
    virtual bool isPresent() const = 0;
};

}
