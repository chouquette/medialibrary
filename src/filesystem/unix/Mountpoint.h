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

#include "filesystem/IMountpoint.h"
#include <memory>
#include <unordered_map>

namespace fs
{

class UnknownMountpoint : public IMountpoint
{
public:
    UnknownMountpoint() : m_uuid( "unknown" ) {}
    virtual const std::string& uuid() const override { return m_uuid; }
    virtual bool isPresent() const override { return true; }
    virtual bool isRemovable() const override { return false; }
private:
    std::string m_uuid;
};

class Mountpoint : public IMountpoint
{
public:
    using MountpointMap = std::unordered_map<std::string, std::shared_ptr<IMountpoint>>;

    virtual const std::string& uuid() const override;
    virtual bool isPresent() const override;
    virtual bool isRemovable() const override;


    static std::shared_ptr<IMountpoint> fromPath( const std::string& path );

protected:
    Mountpoint( const std::string& devicePath );

private:
    static MountpointMap listMountpoints();
    static std::shared_ptr<IMountpoint> unknownMountpoint;

private:
    static const MountpointMap Cache;

private:
    std::string m_device;
    std::string m_uuid;
};

}
