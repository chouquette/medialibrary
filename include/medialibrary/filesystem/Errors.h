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

#include <stdexcept>

namespace medialibrary
{

namespace fs
{

namespace errors
{

class Exception : public std::runtime_error
{
public:
    Exception( const std::string& str )
        : std::runtime_error( str )
    {
    }
};

class UnknownScheme : public Exception
{
public:
    UnknownScheme( const std::string& scheme )
        : Exception( "No filesystem factory found for scheme " + scheme )
        , m_scheme( scheme )
    {
    }

    const std::string& scheme() const
    {
        return m_scheme;
    }

private:
    std::string m_scheme;
};

class DeviceRemoved : public Exception
{
public:
    DeviceRemoved() noexcept
        : Exception( "The device containing this file/folder was removed" )
    {
    }
};

}

}

}
