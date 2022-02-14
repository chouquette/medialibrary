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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "SqliteConnection.h"

#include "database/SqliteTools.h"

#include <cstring>

#define DEBUG_SQLITE_TRIGGERS 0

namespace medialibrary
{
namespace sqlite
{

thread_local Connection::Handle Connection::Context::m_handle;
thread_local Connection::Context::Type Connection::Context::m_type;

Connection::Connection( const std::string& dbPath )
    : m_dbPath( dbPath )
    , m_readLock( m_contextLock )
    , m_writeLock( m_contextLock )
    , m_priorityLock( m_contextLock )
{
    /* Indirect call to sqlite3_config */
    static SqliteConfigurator config;
}

Connection::~Connection()
{
    sqlite::Statement::FlushStatementCache();
}

Connection::Handle Connection::handle()
{
    /**
     * We need to have a single sqlite connection per thread, but we also need
     * to be able to destroy those when the SqliteConnection wrapper instance
     * gets destroyed (otherwise we can't re-instantiate the media library during
     * the application runtime. That can't work for unit test and can be quite
     * a limitation for the application using the medialib)
     * Being able to refer to all per-thread connection means we can't use
     * thread local directly, however, we need to know if a thread goes away, to
     * avoid re-using an old connection on a new thread with the same id, as this
     * would result in a "Database is locked error"
     * In order to solve this, we use a single map to store all connection,
     * indexed by std::thread::id (well, compat::thread::id). Additionaly, in
     * order to know when a thread gets terminated, we store a thread_local
     * object to signal back when a thread returns, and to remove the now
     * unusable connection.
     * Also note that we need to flush any potentially cached compiled statement
     * when a thread gets terminated, as it would become unusable as well.
     *
     * \sa sqlite::Connection::ThreadSpecificConnection::~ThreadSpecificConnection
     * \sa sqlite::Statement::StatementsCache
     */
    std::unique_lock<compat::Mutex> lock( m_connMutex );
    auto it = m_conns.find( compat::this_thread::get_id() );
    if ( it != end( m_conns ) )
        return it->second.get();
    sqlite3* dbConnection;
    auto flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX;
    if ( m_conns.empty() == true )
        flags |= SQLITE_OPEN_CREATE;
    auto res = sqlite3_open_v2( m_dbPath.c_str(), &dbConnection, flags, nullptr );
    ConnPtr dbConn( dbConnection, &sqlite3_close );
    if ( res != SQLITE_OK )
    {
        int err = sqlite3_system_errno(dbConnection);
        LOG_ERROR( "Failed to connect to database. OS error: ", err );
        errors::mapToException( "<connecting to db>", "", res );
    }
    /*
     * Fetch the absolute path to the database. If for whatever reason we were
     * to change directories during the runtime, we'd end up having a connection
     * to a different database in case the provided path is relative.
     * See #262
     */
    if ( m_conns.empty() == true )
    {
        m_dbPath = sqlite3_db_filename( dbConnection, nullptr );
        LOG_DEBUG( "Fetched absolute database path from sqlite: ", m_dbPath );
    }

    res = sqlite3_extended_result_codes( dbConnection, 1 );
    if ( res != SQLITE_OK )
        errors::mapToException( "<enabling extended errors>", "", res );
    // Don't use public wrapper, they need to be able to call getConn, which
    // would result from a recursive call and a deadlock from here.
    setPragma( dbConnection, "foreign_keys", "1" );
    setPragma( dbConnection, "recursive_triggers", "1" );
#ifdef __ANDROID__
    // https://github.com/mozilla/mentat/issues/505
    // Should solve `Failed to run request <DELETE FROM File WHERE id_file = ?>: disk I/O error(6410)`
    setPragma( dbConnection, "temp_store", "2" );
#endif
#if DEBUG_SQLITE_TRIGGERS
    sqlite3_trace_v2( dbConnection, SQLITE_TRACE_STMT | SQLITE_TRACE_CLOSE,
                      [](unsigned int t, void* , void* p, void* x) {
        if ( t == SQLITE_TRACE_STMT )
        {
            const char* str = static_cast<const char*>( x );
            LOG_DEBUG( "Executed: ", str );
        }
        else if ( t == SQLITE_TRACE_CLOSE )
        {
            LOG_DEBUG( "Connection ", p, " was closed" );
        }
        return 0;
    }, nullptr );
#endif
    m_conns.emplace( compat::this_thread::get_id(), std::move( dbConn ) );
    sqlite3_update_hook( dbConnection, &updateHook, this );
#if !defined(_WIN32) || defined(__clang__)
    /*
     * Thread local object with destructors don't behave properly when built
     * with g++.
     * See #264
     */
    static thread_local ThreadSpecificConnection tsc( shared_from_this() );
#endif
    return dbConnection;
}

std::unique_ptr<sqlite::Transaction> Connection::newTransaction()
{
    if ( sqlite::Transaction::isInProgress() == false )
        return std::unique_ptr<sqlite::Transaction>{ new sqlite::ActualTransaction( this ) };
    return std::unique_ptr<sqlite::Transaction>{ new sqlite::NoopTransaction() };
}

Connection::ReadContext Connection::acquireReadContext()
{
    assert( Context::isOpened( Context::Type::Read ) == false );
    return ReadContext{ this };
}

Connection::WriteContext Connection::acquireWriteContext()
{
    assert( Context::isOpened( Context::Type::Write ) == false );
    return WriteContext{ this };
}

Connection::PriorityContext Connection::acquirePriorityContext()
{
    assert( Context::isOpened( Context::Type::Priority ) == false );
    return PriorityContext{ this };
}

void Connection::setPragma( Connection::Handle conn, const std::string& pragmaName,
                            const std::string& value )
{
    std::string reqBase = std::string{ "PRAGMA " } + pragmaName;
    std::string reqSet = reqBase + " = " + value;

    sqlite::Statement stmt( conn, reqSet );
    stmt.execute();
    if ( stmt.row() != nullptr )
        throw std::runtime_error( "Failed to enable/disable " + pragmaName );

    sqlite::Statement stmtCheck( conn, reqBase );
    stmtCheck.execute();
    auto resultRow = stmtCheck.row();
    std::string resultValue;
    resultRow >> resultValue;
    if( resultValue != value )
        throw std::runtime_error( "PRAGMA " + pragmaName + " value mismatch" );
}

void Connection::setForeignKeyEnabled( bool value )
{
    // Changing this pragma during a transaction is a no-op (silently ignored by
    // sqlite), so ensure we're doing something usefull here:
    assert( sqlite::Transaction::isInProgress() == false );
    // Ensure no transaction will be started during the pragma change
    auto ctx = acquireWriteContext();
    setPragma( ctx.handle(), "foreign_keys", value ? "1" : "0" );
}

void Connection::setRecursiveTriggersEnabled( bool value )
{
    // Ensure no request will run while we change this setting
    auto ctx = acquireWriteContext();
    setPragma( ctx.handle(), "recursive_triggers", value == true ? "1" : "0" );
}

void Connection::registerUpdateHook( const std::string& table, Connection::UpdateHookCb cb )
{
    m_hooks.emplace( table, std::move( cb ) );
}

bool Connection::checkSchemaIntegrity()
{
    auto conn = handle();
    std::string req = std::string{ "PRAGMA integrity_check" };

    sqlite::Statement stmt( conn, req );
    stmt.execute();
    auto row = stmt.row();
    if ( row.load<std::string>( 0 ) == "ok" )
    {
        row = stmt.row();
        assert( row == nullptr );
        return true;
    }
    do
    {
        LOG_ERROR( "Error string from integrity_check: ", row.load<std::string>( 0 ) );
        row = stmt.row();
    }
    while ( row != nullptr );
    return false;
}

bool Connection::checkForeignKeysIntegrity()
{
    auto conn = handle();
    std::string req = std::string{ "PRAGMA foreign_key_check" };

    sqlite::Statement stmt( conn, req );
    stmt.execute();
    auto row = stmt.row();
    if ( row == nullptr )
        return true;
    do
    {
        auto table = row.extract<std::string>();
        auto rowid = row.extract<int64_t>();
        auto targetTable = row.extract<std::string>();
        auto idx = row.extract<int64_t>();
        LOG_ERROR( "Foreign Key error: In table ", table, " rowid: ", rowid,
                   " referring to table ", targetTable, " at index ", idx );
        row = stmt.row();
    }
    while ( row != nullptr );
    return false;
}

const std::string&Connection::dbPath() const
{
    return m_dbPath;
}

void Connection::flushAll()
{
    Statement::FlushStatementCache();
    std::unique_lock<compat::Mutex> lock( m_connMutex );
    m_conns.clear();
}

std::shared_ptr<Connection> Connection::connect( const std::string& dbPath )
{
    // Use a wrapper to allow make_shared to use the private Connection ctor
    struct SqliteConnectionWrapper : public Connection
    {
        explicit SqliteConnectionWrapper( const std::string& p ) : Connection( p ) {}
    };
    return std::make_shared<SqliteConnectionWrapper>( dbPath );
}

void Connection::updateHook( void* data, int reason, const char*,
                             const char* table, sqlite_int64 rowId )
{
    const auto self = reinterpret_cast<Connection*>( data );
    auto it = self->m_hooks.find( table );
    if ( it == end( self->m_hooks ) )
        return;
    switch ( reason )
    {
    case SQLITE_INSERT:
        it->second( HookReason::Insert, rowId );
        break;
    case SQLITE_UPDATE:
        it->second( HookReason::Update, rowId );
        break;
    case SQLITE_DELETE:
        it->second( HookReason::Delete, rowId );
        break;
    }
}

#if DEBUG_SQLITE_TRIGGERS
namespace
{
void logCallback( void*, int c, const char *str )
{
    LOG_DEBUG( "Sqlite error; code: ", c, " msg: ", str);
}
}
#endif

Connection::WeakDbContext::WeakDbContext( Connection* conn )
    : m_conn( conn )
    , m_fkeyCtx( m_conn )
{
    m_conn->setRecursiveTriggersEnabled( false );
}

Connection::WeakDbContext::~WeakDbContext()
{
    m_conn->setRecursiveTriggersEnabled( true );
}

Connection::ThreadSpecificConnection::ThreadSpecificConnection(
        std::shared_ptr<Connection> conn )
    : m_weakConnection( std::move( conn ) )
{
}

Connection::ThreadSpecificConnection::~ThreadSpecificConnection()
{
    auto conn =  m_weakConnection.lock();
    if ( conn == nullptr )
        return;
    std::unique_lock<compat::Mutex> lock( conn->m_connMutex );
    auto it = conn->m_conns.find( compat::this_thread::get_id() );
    if ( it != end( conn->m_conns ) )
    {
        // Ensure those cached statements will not be used if another thread
        // with the same ID gets created
        sqlite::Statement::FlushConnectionStatementCache( it->second.get() );
        // And ensure we won't use the same connection if a thread with the same
        // ID gets used in the future.
        conn->m_conns.erase( it );
    }
}

Connection::SqliteConfigurator::SqliteConfigurator()
{
    if ( sqlite3_threadsafe() == 0 )
        throw std::runtime_error( "SQLite isn't built with threadsafe mode" );
    if ( sqlite3_config( SQLITE_CONFIG_MULTITHREAD ) == SQLITE_ERROR )
        throw std::runtime_error( "Failed to enable sqlite multithreaded mode" );
#if DEBUG_SQLITE_TRIGGERS
    sqlite3_config( SQLITE_CONFIG_LOG, &logCallback, nullptr );
#endif
}

Connection::DisableForeignKeyContext::DisableForeignKeyContext( Connection* conn )
    : m_conn( conn )
{
    m_conn->setForeignKeyEnabled( false );
}

Connection::DisableForeignKeyContext::~DisableForeignKeyContext()
{
    m_conn->setForeignKeyEnabled( true );
}

Connection::Context::~Context()
{
    releaseHandle();
}

Connection::Handle Connection::Context::handle()
{
    assert( m_handle != nullptr );
    return m_handle;
}

bool Connection::Context::isOpened( Type t )
{
    if ( m_handle == nullptr )
        return false;
    switch ( m_type )
    {
    case Type::None:
        assert( !"Context type can't be none if a handle is available" );
        return false;
    case Type::Write:
    {
        /*
         * If a write context is already opened, it has an exclusive access and can
         * be used to execute read requests
         */
        return true;
    }
    case Type::Read:
    {
        /*
         * We can't open a write context if a read context is created, nor can we
         * create a read context if a write context is created.
         * The only configuration we support is to recursively create a context of the
         * same type.
         */
        assert( t == Type::Read );
        return true;
    }
    case Type::Priority:
        if ( t == Type::Read || t == Type::Write )
            return true;
        assert( !"Recursive acquisition of priority context is not supported."
                " Please fix the calling code" );
        return false;
    default:
        assert( !"Invalid context type provided" );
        return false;
    }
}

void Connection::Context::connect( Connection* c, Type t )
{
    if ( m_handle != nullptr )
    {
        /*
         * Priority contexts are acquired outside of the media library code so
         * we have no control over potential recursive acquisition.
         * We could mostly ignore those, but if the media library code needs to
         * open a transaction while a priority context is held, we still need the
         * transaction code to behave properly and open itself without
         * acquiring a new context.
         */
        assert( m_type == Type::Priority );
        return;
    }
    assert( m_handle == nullptr );
    m_handle = c->handle();
    m_type = t;
    assert( m_handle != nullptr );
    m_owning = true;
}

void Connection::Context::releaseHandle()
{
    /*
     * We don't want to unset the current thread's context when destroying
     * a default constructed Context
     */
    if ( m_owning == false )
        return;
    assert( m_handle != nullptr );
    m_handle = nullptr;
    m_type = Type::None;
    m_owning = false;
}

Connection::Context::Context( Context&& ctx ) noexcept
{
    *this = std::move( ctx );
}

Connection::Context& Connection::Context::operator=( Context&& ctx ) noexcept
{
    m_owning = ctx.m_owning;
    ctx.m_owning = false;
    return *this;
}

Connection::ReadContext::ReadContext( Connection* c )
    : m_lock( c->m_readLock )
{
    connect( c, Type::Read );
}

Connection::WriteContext::WriteContext( Connection* c )
    : m_lock( c->m_writeLock )
{
    connect( c, Type::Write );
}

void Connection::WriteContext::unlock()
{
    m_lock.unlock();
    releaseHandle();
}

Connection::PriorityContext::PriorityContext( Connection* c )
    : m_lock( c->m_priorityLock )
{
    connect( c, Type::Priority );
}

}

}
