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
#include <exception>

namespace sqlite
{
namespace errors
{

class ConstraintViolation : public std::exception
{
public:
    ConstraintViolation( const std::string& req, const std::string& err )
    {
        m_reason = std::string( "Request <" ) + req + "> aborted due to "
                "constraint violation (" + err + ")";
    }

    virtual const char* what() const noexcept override
    {
        return m_reason.c_str();
    }
private:
    std::string m_reason;
};

class ColumnOutOfRange : public std::exception
{
public:
    ColumnOutOfRange( unsigned int idx, unsigned int nbColumns )
    {
        m_reason = "Attempting to extract column at index " + std::to_string( idx ) +
                " from a request with " + std::to_string( nbColumns ) + "columns";
    }

    virtual const char* what() const noexcept override
    {
        return m_reason.c_str();
    }

private:
    std::string m_reason;
};

} // namespace errors
} // namespace sqlite
