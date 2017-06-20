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
#include <stdexcept>

#include <sqlite3.h>

namespace medialibrary
{

namespace sqlite
{
namespace errors
{

class Generic : public std::runtime_error
{
public:
    Generic( const char* req, const char* msg, int extendedCode )
        : std::runtime_error( std::string( "Failed to compile/prepare request <" ) + req
                                           + ">: " + msg + "(" + std::to_string( extendedCode ) + ")" )
    {
    }
    Generic( const std::string& msg )
        : std::runtime_error( msg )
    {
    }
};

class GenericExecution : public Generic
{
public:
    GenericExecution( const char* req, const char* errMsg, int errCode, int extendedCode )
        : Generic( std::string( "Failed to run request <" ) + req + ">: " + errMsg +
                   "(" + std::to_string( extendedCode ) + ")" )
        , m_errorCode( errCode )
    {
    }

    GenericExecution( const std::string& msg, int errCode )
        : Generic( msg )
        , m_errorCode( errCode )
    {
    }

    int code() const
    {
        return m_errorCode;
    }

private:
    int m_errorCode;
};

class ConstraintViolation : public Generic
{
public:
    ConstraintViolation( const std::string& req, const std::string& err )
        : Generic( std::string( "Request <" ) + req + "> aborted due to "
                    "constraint violation (" + err + ")" )
    {
    }
};

class ColumnOutOfRange : public Generic
{
public:
    ColumnOutOfRange( unsigned int idx, unsigned int nbColumns )
        : Generic( "Attempting to extract column at index " + std::to_string( idx ) +
                   " from a request with " + std::to_string( nbColumns ) + " columns" )
    {
    }
};

static inline bool isInnocuous( const GenericExecution& ex )
{
    switch ( ex.code() )
    {
    case SQLITE_IOERR:
    case SQLITE_NOMEM:
    case SQLITE_BUSY:
    case SQLITE_READONLY:
        return true;
    }
    return false;
}


} // namespace errors
} // namespace sqlite

} // namespace medialibrary
