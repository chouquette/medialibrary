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

#include "Types.h"

#include <vector>

class IFolder
{
public:
    virtual ~IFolder() = default;
    virtual unsigned int id() const = 0;
    virtual const std::string& path() = 0;
    // This will only returns the files in this immediate folder
    virtual std::vector<FilePtr> files() = 0;
    virtual std::vector<FolderPtr> folders() = 0;
    virtual unsigned int lastModificationDate() = 0;
    virtual bool isRemovable() = 0;
    virtual FolderPtr parent() = 0;
};
