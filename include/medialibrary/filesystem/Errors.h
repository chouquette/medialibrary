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
#include <system_error>

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

class UnhandledScheme : public Exception
{
public:
    UnhandledScheme( const std::string& scheme )
        : Exception( "Unhandled MRL scheme: " + scheme )
        , m_scheme( scheme )
    {
    }

    const std::string& scheme() const noexcept
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

class DeviceMapper : public Exception
{
public:
    DeviceMapper( const std::string& str )
        : Exception( "Failed to resolve using device mapper: " + str )
    {
    }
};

class DeviceListing : public Exception
{
public:
    DeviceListing( const std::string& str )
        : Exception( "Failed to list devices: " + str )
    {
    }
};

class NotFound : public Exception
{
public:
    NotFound( const std::string& mrl, const std::string& container )
        : Exception( mrl + " was not found in " + container )
    {
    }
};

class System : public Exception
{
public:
#ifdef _WIN32
    System( unsigned long err, const std::string& msg )
        : Exception( msg + ": " +
                     std::error_code( err, std::generic_category() ).message() )
        , m_errc( err, std::generic_category() )
    {
    }
#endif
    System( int err, const std::string& msg )
        : Exception( msg + ": " +
                     std::error_code( err, std::generic_category() ).message() )
        , m_errc( err, std::generic_category() )
    {
    }

    const std::error_code& code() const noexcept
    {
        return m_errc;
    }

private:
    std::error_code m_errc;
};

}

}

}
