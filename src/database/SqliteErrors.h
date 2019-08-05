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

/**
 * These errors happen before the request gets executed. Usually because of a
 * syntax error, or an invalid bind index
 */
class Generic : public std::runtime_error
{
public:
    Generic( const char* req, const char* msg, int extendedCode )
        : std::runtime_error( std::string( "Failed to compile/prepare request [" ) + req
                                           + "]: " + msg + "(" + std::to_string( extendedCode ) + ")" )
    {
    }
    Generic( const std::string& msg )
        : std::runtime_error( msg )
    {
    }
};

/**
 * This is the general type for runtime error, ie. when the request is being
 * executed.
 */
class GenericExecution : public Generic
{
public:
    GenericExecution( const char* req, const char* errMsg, int extendedCode )
        : Generic( std::string( "Failed to run request [" ) + req + "]: " + errMsg +
                   "(" + std::to_string( extendedCode ) + ")" )
        , m_errorCode( extendedCode )
    {
    }

    GenericExecution( const std::string& msg, int errCode )
        : Generic( msg )
        , m_errorCode( errCode )
    {
    }

    int code() const
    {
        return m_errorCode & 0xFF;
    }

private:
    int m_errorCode;
};

class ConstraintViolation : public Generic
{
public:
    ConstraintViolation( const std::string& req, const std::string& err )
        : Generic( std::string( "Request [" ) + req + "] aborted due to "
                    "constraint violation (" + err + ")" )
    {
    }
};

class GenericError : public GenericExecution
{
public:
    GenericError( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class DatabaseBusy : public GenericExecution
{
public:
    DatabaseBusy( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class DatabaseLocked : public GenericExecution
{
public:
    DatabaseLocked( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class DatabaseReadOnly : public GenericExecution
{
public:
    DatabaseReadOnly( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIOErr : public GenericExecution
{
public:
    DatabaseIOErr( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class DatabaseCorrupt : public GenericExecution
{
public:
    DatabaseCorrupt( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class DatabaseFull: public GenericExecution
{
public:
    DatabaseFull( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class ProtocolError : public GenericExecution
{
public:
    ProtocolError( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class DatabaseSchemaChanged : public GenericExecution
{
public:
    DatabaseSchemaChanged( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class TypeMismatch : public GenericExecution
{
public:
    TypeMismatch( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
    {
    }
};

class LibMisuse : public GenericExecution
{
public:
    LibMisuse( const char* req, const char* errMsg, int extendedCode )
        : GenericExecution( req, errMsg, extendedCode )
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

static inline bool isInnocuous( int errCode )
{
    switch ( errCode )
    {
    case SQLITE_IOERR:
    case SQLITE_NOMEM:
    case SQLITE_BUSY:
    case SQLITE_READONLY:
        return true;
    }
    return false;
}

static inline bool isInnocuous( const GenericExecution& ex )
{
    return isInnocuous( ex.code() );
}


} // namespace errors
} // namespace sqlite

} // namespace medialibrary
