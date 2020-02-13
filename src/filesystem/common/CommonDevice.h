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

#include "medialibrary/filesystem/IDevice.h"
#include "compat/Mutex.h"
#include <vector>

namespace medialibrary
{
namespace fs
{

class CommonDevice : public IDevice
{
public:
    CommonDevice( const std::string& uuid, const std::string& mountpoint,
                  std::string scheme, bool isRemovable );
    virtual const std::string& uuid() const override;
    virtual const std::string& scheme() const override;
    virtual bool isRemovable() const override;
    virtual bool isPresent() const override;
    virtual const std::string& mountpoint() const override;
    virtual void addMountpoint( std::string mountpoint ) override;
    virtual void removeMountpoint( const std::string& mountpoint ) override;
    virtual std::tuple<bool, std::string>
    matchesMountpoint( const std::string& mrl ) const override;
    virtual std::string relativeMrl( const std::string& absoluteMrl ) const override;
    virtual std::string absoluteMrl( const std::string& relativeMrl ) const override;

private:
    std::tuple<bool, std::string>
        matchesMountpointLocked( const std::string& mrl ) const;

private:
    std::string m_uuid;
    std::vector<std::string> m_mountpoints;
    std::string m_scheme;
    mutable compat::Mutex m_mutex;
    bool m_removable;
};

}
}
