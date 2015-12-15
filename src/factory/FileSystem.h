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

#include "factory/IFileSystem.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace factory
{
    class FileSystemFactory : public IFileSystem
    {
    public:
        virtual std::shared_ptr<fs::IDirectory> createDirectory( const std::string& path ) override;
        virtual std::unique_ptr<fs::IFile> createFile( const std::string& fileName ) override;
        virtual std::shared_ptr<fs::IDevice> createDevice( const std::string& uuid ) override;

    private:
        std::unordered_map<std::string, std::shared_ptr<fs::IDirectory>> m_dirs;
        std::mutex m_mutex;
    };
}
