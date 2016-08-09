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

#include "filesystem/IDevice.h"

namespace medialibrary
{
namespace fs
{

class CommonDevice : public IDevice
{
public:
    CommonDevice( const std::string& uuid, const std::string& mountpoint, bool isRemovable );
    virtual const std::string& uuid() const override;
    virtual bool isRemovable() const override;
    virtual bool isPresent() const override;
    virtual const std::string& mountpoint() const override;

private:
    std::string m_uuid;
    std::string m_mountpoint;
    bool m_present;
    bool m_removable;
};

}
}
