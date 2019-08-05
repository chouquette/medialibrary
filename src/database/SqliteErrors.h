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
class Runtime : public Generic
{
public:
    Runtime( const char* req, const char* errMsg, int extendedCode )
        : Generic( std::string( "Failed to run request [" ) + req + "]: " + errMsg +
                   "(" + std::to_string( extendedCode ) + ")" )
        , m_errorCode( extendedCode )
    {
    }

    Runtime( const std::string& msg, int errCode )
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

class ConstraintViolation : public Runtime
{
public:
    ConstraintViolation( const char* req, const char* err, int errCode )
        : Runtime( std::string( "Request [" ) + req + "] aborted due to "
                    "constraint violation (" + err + ")", errCode )
    {
    }
};

class GenericError : public Runtime
{
public:
    GenericError( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class DatabaseBusy : public Runtime
{
public:
    DatabaseBusy( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class DatabaseLocked : public Runtime
{
public:
    DatabaseLocked( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class DatabaseReadOnly : public Runtime
{
public:
    DatabaseReadOnly( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIOErr : public Runtime
{
public:
    DatabaseIOErr( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrAccess : public DatabaseIOErr
{
public:
    DatabaseIoErrAccess( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrRead : public DatabaseIOErr
{
public:
    DatabaseIoErrRead( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrShortRead : public DatabaseIOErr
{
public:
    DatabaseIoErrShortRead( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrWrite : public DatabaseIOErr
{
public:
    DatabaseIoErrWrite( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};


class DatabaseIoErrFsync : public DatabaseIOErr
{
public:
    DatabaseIoErrFsync( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrDirClose : public DatabaseIOErr
{
public:
    DatabaseIoErrDirClose( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrDirFsync : public DatabaseIOErr
{
public:
    DatabaseIoErrDirFsync( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrTruncate : public DatabaseIOErr
{
public:
    DatabaseIoErrTruncate( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrCheckReservedLock : public DatabaseIOErr
{
public:
    DatabaseIoErrCheckReservedLock( const char* req, const char* errMsg,
                                    int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrUnlock : public DatabaseIOErr
{
public:
    DatabaseIoErrUnlock( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrRdLock : public DatabaseIOErr
{
public:
    DatabaseIoErrRdLock( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};


class DatabaseIoErrDelete: public DatabaseIOErr
{
public:
    DatabaseIoErrDelete( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};


class DatabaseIoErrDeleteNoEnt : public DatabaseIOErr
{
public:
    DatabaseIoErrDeleteNoEnt( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrLock : public DatabaseIOErr
{
public:
    DatabaseIoErrLock( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrClose : public DatabaseIOErr
{
public:
    DatabaseIoErrClose( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrShmOpen : public DatabaseIOErr
{
public:
    DatabaseIoErrShmOpen( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrShmSize: public DatabaseIOErr
{
public:
    DatabaseIoErrShmSize( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrShMmap : public DatabaseIOErr
{
public:
    DatabaseIoErrShMmap( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrFstat : public DatabaseIOErr
{
public:
    DatabaseIoErrFstat( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrSeek : public DatabaseIOErr
{
public:
    DatabaseIoErrSeek( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrGetTempPath : public DatabaseIOErr
{
public:
    DatabaseIoErrGetTempPath( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseIoErrMmap : public DatabaseIOErr
{
public:
    DatabaseIoErrMmap( const char* req, const char* errMsg, int extendedCode )
        : DatabaseIOErr( req, errMsg, extendedCode )
    {
    }
};

class DatabaseCorrupt : public Runtime
{
public:
    DatabaseCorrupt( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class DatabaseFull: public Runtime
{
public:
    DatabaseFull( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class ProtocolError : public Runtime
{
public:
    ProtocolError( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class DatabaseSchemaChanged : public Runtime
{
public:
    DatabaseSchemaChanged( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class TypeMismatch : public Runtime
{
public:
    TypeMismatch( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
    {
    }
};

class LibMisuse : public Runtime
{
public:
    LibMisuse( const char* req, const char* errMsg, int extendedCode )
        : Runtime( req, errMsg, extendedCode )
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

    ColumnOutOfRange( const char* req )
        : Generic( std::string{ "Failed to bind to " } + req +
                   ": Parameter out of range" )
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

static inline bool isInnocuous( const Runtime& ex )
{
    return isInnocuous( ex.code() );
}

static inline void mapToException( const char* reqStr, const char* errMsg, int extRes )
{
    auto res = extRes & 0xFF;
    switch ( res )
    {
        case SQLITE_CONSTRAINT:
            throw errors::ConstraintViolation( reqStr, errMsg, extRes );
        case SQLITE_BUSY:
            throw errors::DatabaseBusy( reqStr, errMsg, extRes );
        case SQLITE_LOCKED:
            throw errors::DatabaseLocked( reqStr, errMsg, extRes );
        case SQLITE_READONLY:
            throw errors::DatabaseReadOnly( reqStr, errMsg, extRes );
        case SQLITE_IOERR:
        {
            switch ( extRes )
            {
                case SQLITE_IOERR_READ:
                    throw DatabaseIoErrRead( reqStr, errMsg, extRes );
                case SQLITE_IOERR_SHORT_READ:
                    throw DatabaseIoErrShortRead( reqStr, errMsg, extRes );
                case SQLITE_IOERR_WRITE:
                    throw DatabaseIoErrWrite( reqStr, errMsg, extRes );
                case SQLITE_IOERR_FSYNC:
                    throw DatabaseIoErrFsync( reqStr, errMsg, extRes );
                case SQLITE_IOERR_DIR_FSYNC:
                    throw DatabaseIoErrDirFsync( reqStr, errMsg, extRes );
                case SQLITE_IOERR_TRUNCATE:
                    throw DatabaseIoErrTruncate( reqStr, errMsg, extRes );
                case SQLITE_IOERR_LOCK:
                    throw DatabaseIoErrLock( reqStr, errMsg, extRes );
                case SQLITE_IOERR_ACCESS:
                    throw DatabaseIoErrAccess( reqStr, errMsg, extRes );
                case SQLITE_IOERR_CHECKRESERVEDLOCK:
                    throw DatabaseIoErrCheckReservedLock( reqStr, errMsg, extRes );
                case SQLITE_IOERR_CLOSE:
                    throw DatabaseIoErrClose( reqStr, errMsg, extRes );
                case SQLITE_IOERR_SHMOPEN:
                    throw DatabaseIoErrShmOpen( reqStr, errMsg, extRes );
                case SQLITE_IOERR_SHMMAP:
                    throw DatabaseIoErrShMmap( reqStr, errMsg, extRes );
                case SQLITE_IOERR_SEEK:
                    throw DatabaseIoErrSeek( reqStr, errMsg, extRes );
                case SQLITE_IOERR_MMAP:
                    throw DatabaseIoErrMmap( reqStr, errMsg, extRes );
                case SQLITE_IOERR_FSTAT:
                    throw DatabaseIoErrFstat( reqStr, errMsg, extRes );
                case SQLITE_IOERR_UNLOCK:
                    throw DatabaseIoErrUnlock( reqStr, errMsg, extRes );
                case SQLITE_IOERR_RDLOCK:
                    throw DatabaseIoErrRdLock( reqStr, errMsg, extRes );
                case SQLITE_IOERR_DELETE:
                    throw DatabaseIoErrDelete( reqStr, errMsg, extRes );
                case SQLITE_IOERR_DELETE_NOENT:
                    throw DatabaseIoErrDeleteNoEnt( reqStr, errMsg, extRes );
                case SQLITE_IOERR_DIR_CLOSE:
                    throw DatabaseIoErrDirClose( reqStr, errMsg, extRes );
                case SQLITE_IOERR_SHMSIZE:
                    throw DatabaseIoErrShmSize( reqStr, errMsg, extRes );
                case SQLITE_IOERR_GETTEMPPATH:
                    throw DatabaseIoErrGetTempPath( reqStr, errMsg, extRes );

                // This code is expected to be converted to SQLITE_NOMEM
                case SQLITE_IOERR_NOMEM:
                // This code is only used on cygwin
                case SQLITE_IOERR_CONVPATH:
                // These error codes are undocumented
                case SQLITE_IOERR_VNODE:
                case SQLITE_IOERR_AUTH:
                case SQLITE_IOERR_COMMIT_ATOMIC:
                case SQLITE_IOERR_BEGIN_ATOMIC:
                case SQLITE_IOERR_ROLLBACK_ATOMIC:
                // The remaining error code are not used by sqlite anymore
                case SQLITE_IOERR_BLOCKED:
                case SQLITE_IOERR_SHMLOCK:
                default:
                    throw errors::DatabaseIOErr( reqStr, errMsg, extRes );
            }
        }
        case SQLITE_CORRUPT:
            throw errors::DatabaseCorrupt( reqStr, errMsg, extRes );
        case SQLITE_FULL:
            throw errors::DatabaseFull( reqStr, errMsg, extRes );
        case SQLITE_PROTOCOL:
            throw errors::ProtocolError( reqStr, errMsg, extRes );
        case SQLITE_MISMATCH:
            throw errors::TypeMismatch( reqStr, errMsg, extRes );
        case SQLITE_MISUSE:
            throw errors::LibMisuse( reqStr, errMsg, extRes );
        case SQLITE_ERROR:
            throw errors::GenericError( reqStr, errMsg, extRes );
        default:
            throw errors::Runtime( reqStr, errMsg, extRes );
    }
}

} // namespace errors
} // namespace sqlite

} // namespace medialibrary
