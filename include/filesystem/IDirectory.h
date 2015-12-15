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

#include <memory>
#include <string>
#include <vector>

namespace fs
{
    class IFile;
    class IDevice;

    class IDirectory
    {
    public:
        virtual ~IDirectory() = default;
        // Returns the absolute path to this directory
        virtual const std::string& path() const = 0;
        /// Returns a list of absolute files path
        virtual const std::vector<std::string>& files() = 0;
        /// Returns a list of absolute path to this folder subdirectories
        virtual const std::vector<std::string>& dirs() = 0;
        virtual unsigned int lastModificationDate() const = 0;
        virtual std::shared_ptr<IDevice> device() const = 0;
        virtual bool isRemovable() const = 0;
    };
}
