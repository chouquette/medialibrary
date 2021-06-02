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
#include "utils/Url.h"
#include <vector>

namespace medialibrary
{
namespace fs
{

class CommonDevice : public IDevice
{
public:
    CommonDevice( const std::string& uuid, const std::string& mountpoint,
                  std::string scheme, bool isRemovable, bool isNetwork );
    virtual const std::string& uuid() const override;
    virtual const std::string& scheme() const override;
    virtual bool isRemovable() const override;
    virtual bool isPresent() const override;
    virtual bool isNetwork() const override;
    virtual std::vector<std::string> mountpoints() const override;
    virtual void addMountpoint( std::string mountpoint ) override;
    virtual void removeMountpoint( const std::string& mountpoint ) override;
    virtual std::tuple<bool, std::string>
    matchesMountpoint( const std::string& mrl ) const override;
    virtual std::string relativeMrl( const std::string& absoluteMrl ) const override;
    virtual std::string absoluteMrl( const std::string& relativeMrl ) const override;

private:
    struct Mountpoint
    {
        explicit Mountpoint( std::string m )
            : mrl( std::move( m ) )
            , url( utils::url::split( mrl ) )
        {
        }
        bool operator==( const Mountpoint& mrl ) const;
        std::string mrl;
        utils::url::parts url;
    };
    std::string m_uuid;
    std::vector<Mountpoint> m_mountpoints;
    std::string m_scheme;
    mutable compat::Mutex m_mutex;
    bool m_removable;
    bool m_isNetwork;
};

}
}
