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

#include "medialibrary/filesystem/IDirectory.h"

namespace medialibrary
{

namespace fs
{

class IFileSystemFactory;

class CommonDirectory : public IDirectory
{
public:
    explicit CommonDirectory( fs::IFileSystemFactory& fsFactory );
    virtual const std::vector<std::shared_ptr<IFile>>& files() const override;
    virtual const std::vector<std::shared_ptr<IDirectory>>& dirs() const override;
    virtual std::shared_ptr<IDevice> device() const override;
    virtual std::shared_ptr<IFile> file( const std::string& mrl ) const override;
    virtual bool contains( const std::string& fileName ) const override;

protected:
    virtual void read() const = 0;

protected:
    mutable std::vector<std::shared_ptr<IFile>> m_files;
    mutable std::vector<std::shared_ptr<IDirectory>> m_dirs;
    mutable std::shared_ptr<IDevice> m_device;
    fs::IFileSystemFactory& m_fsFactory;
};

}
}
