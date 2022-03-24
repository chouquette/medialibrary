/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "SqliteErrors.h"

namespace medialibrary
{
namespace sqlite
{
namespace errors
{

void mapToException( const char* reqStr, const char* errMsg, int extRes )
{
    auto res = extRes & 0xFF;
    switch ( res )
    {
        case SQLITE_CONSTRAINT:
        {
            switch ( extRes )
            {
                case SQLITE_CONSTRAINT_CHECK:
                    throw errors::ConstraintCheck( reqStr, errMsg, extRes );
                case SQLITE_CONSTRAINT_FOREIGNKEY:
                    throw errors::ConstraintForeignKey( reqStr, errMsg, extRes );
                case SQLITE_CONSTRAINT_NOTNULL:
                    throw errors::ConstraintNotNull( reqStr, errMsg, extRes );
                case SQLITE_CONSTRAINT_PRIMARYKEY:
                    throw errors::ConstraintPrimaryKey( reqStr, errMsg, extRes );
                case SQLITE_CONSTRAINT_ROWID:
                    throw errors::ConstraintRowId( reqStr, errMsg, extRes );
                case SQLITE_CONSTRAINT_UNIQUE:
                    throw errors::ConstraintUnique( reqStr, errMsg, extRes );
                default:
                    throw errors::ConstraintViolation( reqStr, errMsg, extRes );
            }
        }
        case SQLITE_BUSY:
        {
            switch ( extRes )
            {
                case SQLITE_BUSY_RECOVERY:
                    throw errors::DatabaseBusyRecovery( reqStr, errMsg, extRes );
                case SQLITE_BUSY_SNAPSHOT:
                    throw errors::DatabaseBusySnapshot( reqStr, errMsg, extRes );
                default:
                    throw errors::DatabaseBusy( reqStr, errMsg, extRes );
            }
        }
        case SQLITE_LOCKED:
        {
            switch ( extRes )
            {
                case SQLITE_LOCKED_SHAREDCACHE:
                    throw errors::DatabaseLockedSharedCache( reqStr, errMsg, extRes );
                case SQLITE_LOCKED_VTAB:
                    throw errors::DatabaseLockedVtab( reqStr, errMsg, extRes );
                default:
                    throw errors::DatabaseLocked( reqStr, errMsg, extRes );
            }
        }
        case SQLITE_READONLY:
        {
            switch ( extRes )
            {
                case SQLITE_READONLY_RECOVERY:
                    throw DatabaseReadOnlyRecovery( reqStr, errMsg, extRes );
                case SQLITE_READONLY_CANTLOCK:
                    throw DatabaseReadOnlyCantLock( reqStr, errMsg, extRes );
                case SQLITE_READONLY_ROLLBACK:
                    throw DatabaseReadOnlyRollback( reqStr, errMsg, extRes );
                case SQLITE_READONLY_DBMOVED:
                    throw DatabaseReadOnlyDbMoved( reqStr, errMsg, extRes );
                case SQLITE_READONLY_CANTINIT:
                    throw DatabaseReadOnlyCantInit( reqStr, errMsg, extRes );
                case SQLITE_READONLY_DIRECTORY:
                    throw DatabaseReadOnlyDirectory( reqStr, errMsg, extRes );
                default:
                    throw errors::DatabaseReadOnly( reqStr, errMsg, extRes );
            }
        }
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
        case SQLITE_RANGE:
            throw errors::ColumnOutOfRange( reqStr, errMsg, extRes );
        case SQLITE_ERROR:
        {
            switch ( extRes )
            {
                case SQLITE_ERROR_MISSING_COLLSEQ:
                    throw errors::ErrorMissingColSeq( reqStr, errMsg, extRes );
                case SQLITE_ERROR_RETRY:
                    throw errors::ErrorRetry( reqStr, errMsg, extRes );
#ifdef SQLITE_ERROR_SNAPSHOT
                case SQLITE_ERROR_SNAPSHOT:
                    throw errors::ErrorSnapshot( reqStr, errMsg, extRes );
#endif
                default:
                    throw errors::GenericError( reqStr, errMsg, extRes );
            }
        }
        default:
            throw errors::Exception( reqStr, errMsg, extRes );
    }
}

Exception::Exception(const char* req, const char* errMsg, int extendedCode)
    : std::runtime_error( std::string( "Failed to run request [" ) + req + "]: " +
                          ( errMsg != nullptr ? errMsg : "" ) +
                          "(" + std::to_string( extendedCode ) + ")" )
    , m_errorCode( extendedCode )
{
}

Exception::Exception(const std::string& msg, int errCode)
    : std::runtime_error( msg )
    , m_errorCode( errCode )
{
}

int Exception::code() const
{
    return m_errorCode & 0xFF;
}

bool Exception::requiresDbReset() const
{
    return false;
}

ConstraintViolation::ConstraintViolation(const char* req, const char* err, int errCode)
    : Exception( std::string( "Request [" ) + req + "] aborted due to "
                                                    "constraint violation (" + err + ")", errCode )
{
}

ConstraintCheck::ConstraintCheck(const char* req, const char* err, int errCode)
    : ConstraintViolation( req, err, errCode )
{
}

ConstraintForeignKey::ConstraintForeignKey(const char* req, const char* err, int errCode)
    : ConstraintViolation( req, err, errCode )
{
}

ConstraintNotNull::ConstraintNotNull(const char* req, const char* err, int errCode)
    : ConstraintViolation( req, err, errCode )
{
}

ConstraintPrimaryKey::ConstraintPrimaryKey(const char* req, const char* err, int errCode)
    : ConstraintViolation( req, err, errCode )
{
}

ConstraintRowId::ConstraintRowId(const char* req, const char* err, int errCode)
    : ConstraintViolation( req, err, errCode )
{
}

ConstraintUnique::ConstraintUnique(const char* req, const char* err, int errCode)
    : ConstraintViolation( req, err, errCode )
{
}

GenericError::GenericError(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

ErrorMissingColSeq::ErrorMissingColSeq(const char* req, const char* errMsg, int extendedCode)
    : GenericError( req, errMsg, extendedCode )
{
}

ErrorRetry::ErrorRetry(const char* req, const char* errMsg, int extendedCode)
    : GenericError( req, errMsg, extendedCode )
{
}

ErrorSnapshot::ErrorSnapshot(const char* req, const char* errMsg, int extendedCode)
    : GenericError( req, errMsg, extendedCode )
{
}

DatabaseBusy::DatabaseBusy(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

DatabaseBusyRecovery::DatabaseBusyRecovery(const char* req, const char* errMsg, int extendedCode)
    : DatabaseBusy( req, errMsg, extendedCode )
{
}

DatabaseBusySnapshot::DatabaseBusySnapshot(const char* req, const char* errMsg, int extendedCode)
    : DatabaseBusy( req, errMsg, extendedCode )
{
}

DatabaseLocked::DatabaseLocked(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

DatabaseLockedSharedCache::DatabaseLockedSharedCache(const char* req, const char* errMsg, int extendedCode)
    : DatabaseLocked( req, errMsg, extendedCode )
{
}

DatabaseLockedVtab::DatabaseLockedVtab(const char* req, const char* errMsg, int extendedCode)
    : DatabaseLocked( req, errMsg, extendedCode )
{
}

DatabaseReadOnly::DatabaseReadOnly(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

DatabaseReadOnlyRecovery::DatabaseReadOnlyRecovery(const char* req, const char* errMsg, int extendedCode)
    : DatabaseReadOnly( req, errMsg, extendedCode )
{
}

DatabaseReadOnlyCantLock::DatabaseReadOnlyCantLock(const char* req, const char* errMsg, int extendedCode)
    : DatabaseReadOnly( req, errMsg, extendedCode )
{
}

DatabaseReadOnlyRollback::DatabaseReadOnlyRollback(const char* req, const char* errMsg, int extendedCode)
    : DatabaseReadOnly( req, errMsg, extendedCode )
{
}

DatabaseReadOnlyDbMoved::DatabaseReadOnlyDbMoved(const char* req, const char* errMsg, int extendedCode)
    : DatabaseReadOnly( req, errMsg, extendedCode )
{
}

DatabaseReadOnlyCantInit::DatabaseReadOnlyCantInit(const char* req, const char* errMsg, int extendedCode)
    : DatabaseReadOnly( req, errMsg, extendedCode )
{
}

DatabaseReadOnlyDirectory::DatabaseReadOnlyDirectory(const char* req, const char* errMsg, int extendedCode)
    : DatabaseReadOnly( req, errMsg, extendedCode )
{
}

DatabaseIOErr::DatabaseIOErr(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

DatabaseIoErrAccess::DatabaseIoErrAccess(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrRead::DatabaseIoErrRead(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrShortRead::DatabaseIoErrShortRead(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrWrite::DatabaseIoErrWrite(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrFsync::DatabaseIoErrFsync(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrDirClose::DatabaseIoErrDirClose(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrDirFsync::DatabaseIoErrDirFsync(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrTruncate::DatabaseIoErrTruncate(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrCheckReservedLock::DatabaseIoErrCheckReservedLock(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrUnlock::DatabaseIoErrUnlock(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrRdLock::DatabaseIoErrRdLock(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrDelete::DatabaseIoErrDelete(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrDeleteNoEnt::DatabaseIoErrDeleteNoEnt(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrLock::DatabaseIoErrLock(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrClose::DatabaseIoErrClose(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrShmOpen::DatabaseIoErrShmOpen(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrShmSize::DatabaseIoErrShmSize(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrShMmap::DatabaseIoErrShMmap(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrFstat::DatabaseIoErrFstat(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrSeek::DatabaseIoErrSeek(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrGetTempPath::DatabaseIoErrGetTempPath(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseIoErrMmap::DatabaseIoErrMmap(const char* req, const char* errMsg, int extendedCode)
    : DatabaseIOErr( req, errMsg, extendedCode )
{
}

DatabaseCorrupt::DatabaseCorrupt(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

bool DatabaseCorrupt::requiresDbReset() const
{
    return true;
}

DatabaseFull::DatabaseFull(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

ProtocolError::ProtocolError(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

DatabaseSchemaChanged::DatabaseSchemaChanged(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

TypeMismatch::TypeMismatch(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

LibMisuse::LibMisuse(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

ColumnOutOfRange::ColumnOutOfRange(const char* req, const char* errMsg, int extendedCode)
    : Exception( req, errMsg, extendedCode )
{
}

ColumnOutOfRange::ColumnOutOfRange(unsigned int idx, unsigned int nbColumns)
    : Exception( "Attempting to extract column at index " + std::to_string( idx ) +
                 " from a request with " + std::to_string( nbColumns ) + " columns",
                 SQLITE_RANGE )
{
}

} // namespace errors
} // namespace sqlite
} // namespace medialibrary
