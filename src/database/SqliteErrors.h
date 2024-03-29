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
 * This is the general type for all sqlite error
 */
class Exception : public std::runtime_error
{
public:
    Exception( const char* req, const char* errMsg, int extendedCode );

    Exception( const std::string& msg, int errCode );

    int code() const;

    /**
     * @brief requiresDbReset returns true if the error is critical enough
     *          to denote a need for a database reset, for instacne if sqlite
     *          reports the database as corrupted.
     */
    virtual bool requiresDbReset() const;

private:
    int m_errorCode;
};

class ConstraintViolation : public Exception
{
public:
    ConstraintViolation( const char* req, const char* err, int errCode );
};

class ConstraintCheck : public ConstraintViolation
{
public:
    ConstraintCheck( const char* req, const char* err, int errCode );
};

class ConstraintForeignKey : public ConstraintViolation
{
public:
    ConstraintForeignKey( const char* req, const char* err, int errCode );
};

class ConstraintNotNull : public ConstraintViolation
{
public:
    ConstraintNotNull( const char* req, const char* err, int errCode );
};

class ConstraintPrimaryKey : public ConstraintViolation
{
public:
    ConstraintPrimaryKey( const char* req, const char* err, int errCode );
};

class ConstraintRowId : public ConstraintViolation
{
public:
    ConstraintRowId( const char* req, const char* err, int errCode );
};

class ConstraintUnique : public ConstraintViolation
{
public:
    ConstraintUnique( const char* req, const char* err, int errCode );
};

/*
 * /!\ Warning /!\
 * This is not a generic error in the sense of the exception types
 * hierarchy. It's the SQLITE_ERROR counterpart.
 */
class GenericError : public Exception
{
public:
    GenericError( const char* req, const char* errMsg, int extendedCode );
};

class ErrorMissingColSeq : public GenericError
{
public:
    ErrorMissingColSeq( const char* req, const char* errMsg, int extendedCode );
};

class ErrorRetry : public GenericError
{
public:
    ErrorRetry( const char* req, const char* errMsg, int extendedCode );
};

class ErrorSnapshot : public GenericError
{
public:
    ErrorSnapshot( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseBusy : public Exception
{
public:
    DatabaseBusy( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseBusyRecovery : public DatabaseBusy
{
public:
    DatabaseBusyRecovery( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseBusySnapshot: public DatabaseBusy
{
public:
    DatabaseBusySnapshot( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseLocked : public Exception
{
public:
    DatabaseLocked( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseLockedSharedCache : public DatabaseLocked
{
public:
    DatabaseLockedSharedCache( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseLockedVtab : public DatabaseLocked
{
public:
    DatabaseLockedVtab( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseReadOnly : public Exception
{
public:
    DatabaseReadOnly( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseReadOnlyRecovery : public DatabaseReadOnly
{
public:
    DatabaseReadOnlyRecovery( const char* req, const char* errMsg, int extendedCode );
};


class DatabaseReadOnlyCantLock : public DatabaseReadOnly
{
public:
    DatabaseReadOnlyCantLock( const char* req, const char* errMsg, int extendedCode );
};


class DatabaseReadOnlyRollback : public DatabaseReadOnly
{
public:
    DatabaseReadOnlyRollback( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseReadOnlyDbMoved : public DatabaseReadOnly
{
public:
    DatabaseReadOnlyDbMoved( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseReadOnlyCantInit : public DatabaseReadOnly
{
public:
    DatabaseReadOnlyCantInit( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseReadOnlyDirectory : public DatabaseReadOnly
{
public:
    DatabaseReadOnlyDirectory( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIOErr : public Exception
{
public:
    DatabaseIOErr( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrAccess : public DatabaseIOErr
{
public:
    DatabaseIoErrAccess( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrRead : public DatabaseIOErr
{
public:
    DatabaseIoErrRead( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrShortRead : public DatabaseIOErr
{
public:
    DatabaseIoErrShortRead( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrWrite : public DatabaseIOErr
{
public:
    DatabaseIoErrWrite( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrFsync : public DatabaseIOErr
{
public:
    DatabaseIoErrFsync( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrDirClose : public DatabaseIOErr
{
public:
    DatabaseIoErrDirClose( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrDirFsync : public DatabaseIOErr
{
public:
    DatabaseIoErrDirFsync( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrTruncate : public DatabaseIOErr
{
public:
    DatabaseIoErrTruncate( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrCheckReservedLock : public DatabaseIOErr
{
public:
    DatabaseIoErrCheckReservedLock( const char* req, const char* errMsg,
                                    int extendedCode );
};

class DatabaseIoErrUnlock : public DatabaseIOErr
{
public:
    DatabaseIoErrUnlock( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrRdLock : public DatabaseIOErr
{
public:
    DatabaseIoErrRdLock( const char* req, const char* errMsg, int extendedCode );
};


class DatabaseIoErrDelete: public DatabaseIOErr
{
public:
    DatabaseIoErrDelete( const char* req, const char* errMsg, int extendedCode );
};


class DatabaseIoErrDeleteNoEnt : public DatabaseIOErr
{
public:
    DatabaseIoErrDeleteNoEnt( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrLock : public DatabaseIOErr
{
public:
    DatabaseIoErrLock( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrClose : public DatabaseIOErr
{
public:
    DatabaseIoErrClose( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrShmOpen : public DatabaseIOErr
{
public:
    DatabaseIoErrShmOpen( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrShmSize: public DatabaseIOErr
{
public:
    DatabaseIoErrShmSize( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrShMmap : public DatabaseIOErr
{
public:
    DatabaseIoErrShMmap( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrFstat : public DatabaseIOErr
{
public:
    DatabaseIoErrFstat( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrSeek : public DatabaseIOErr
{
public:
    DatabaseIoErrSeek( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrGetTempPath : public DatabaseIOErr
{
public:
    DatabaseIoErrGetTempPath( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseIoErrMmap : public DatabaseIOErr
{
public:
    DatabaseIoErrMmap( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseCorrupt : public Exception
{
public:
    DatabaseCorrupt( const char* req, const char* errMsg, int extendedCode );

    virtual bool requiresDbReset() const override;
};

class DatabaseFull: public Exception
{
public:
    DatabaseFull( const char* req, const char* errMsg, int extendedCode );
};

class ProtocolError : public Exception
{
public:
    ProtocolError( const char* req, const char* errMsg, int extendedCode );
};

class DatabaseSchemaChanged : public Exception
{
public:
    DatabaseSchemaChanged( const char* req, const char* errMsg, int extendedCode );
};

class TypeMismatch : public Exception
{
public:
    TypeMismatch( const char* req, const char* errMsg, int extendedCode );
};

class LibMisuse : public Exception
{
public:
    LibMisuse( const char* req, const char* errMsg, int extendedCode );
};

class ColumnOutOfRange : public Exception
{
public:
    ColumnOutOfRange( const char* req, const char* errMsg, int extendedCode );

    ColumnOutOfRange( unsigned int idx, unsigned int nbColumns );
};

static inline bool isInnocuous( int errCode )
{
    switch ( errCode )
    {
    case SQLITE_IOERR:
    case SQLITE_NOMEM:
    case SQLITE_BUSY:
    case SQLITE_READONLY:
    case SQLITE_FULL:
        return true;
    }
    return false;
}

static inline bool isInnocuous( const Exception& ex )
{
    return isInnocuous( ex.code() );
}

void mapToException( const char* reqStr, const char* errMsg, int extRes );

} // namespace errors
} // namespace sqlite

} // namespace medialibrary
