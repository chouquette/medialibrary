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
#include <unordered_map>

#include "filesystem/IDirectory.h"

using namespace medialibrary;

namespace mock
{

class Device;
class File;

class Directory : public fs::IDirectory
{
public:
    Directory( const std::string& path, std::shared_ptr<Device> device );

    virtual const std::string& path() const override;
    virtual const std::vector<std::shared_ptr<fs::IFile>>& files() const override;
    virtual const std::vector<std::shared_ptr<fs::IDirectory>>& dirs() const override;
    virtual std::shared_ptr<fs::IDevice> device() const override;
    void addFile( const std::string& filePath );
    void addFolder( const std::string& folder );
    void removeFile( const std::string& filePath  );
    std::shared_ptr<File> file( const std::string& filePath );
    std::shared_ptr<Directory> directory( const std::string& path );
    void removeFolder( const std::string& path );
    void setMountpointRoot( const std::string& path, std::shared_ptr<Directory> root );
    void invalidateMountpoint( const std::string& path );

private:
    std::string m_path;
    std::unordered_map<std::string, std::shared_ptr<File>> m_files;
    std::unordered_map<std::string, std::shared_ptr<Directory>> m_dirs;
    mutable std::vector<std::shared_ptr<fs::IFile>> m_filePathes;
    mutable std::vector<std::shared_ptr<fs::IDirectory>> m_dirPathes;
    std::weak_ptr<Device> m_device;
};

}

