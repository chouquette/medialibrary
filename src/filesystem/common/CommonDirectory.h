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

#include "filesystem/IDirectory.h"
#include "utils/Cache.h"

namespace medialibrary
{
namespace fs
{

class CommonDirectory : public IDirectory
{
public:
    CommonDirectory( const std::string& path );
    virtual const std::string& path() const override;
    virtual const std::vector<std::shared_ptr<IFile>>& files() const override;
    virtual const std::vector<std::shared_ptr<IDirectory>>& dirs() const override;

protected:
    virtual void read() const = 0;
    static std::string toAbsolute( const std::string& path );

protected:
    std::string m_path;
    mutable std::vector<std::shared_ptr<IFile>> m_files;
    mutable std::vector<std::shared_ptr<IDirectory>> m_dirs;
    mutable Cache<std::shared_ptr<IDevice>> m_device;
};

}
}
