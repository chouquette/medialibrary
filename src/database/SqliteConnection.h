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

#include <functional>
#include <memory>
#include <sqlite3.h>
#include <unordered_map>
#include <string>

#include "utils/SWMRLock.h"
#include "compat/Mutex.h"
#include "compat/Thread.h"
#include "utils/StringKey.h"

/*
 * Conditionally open a read context if no context is currently opened.
 * The first macro parameter is the context instance name, the 2nd a pointer to
 * a sqlite::Connection instance.
 * The resulting object must not be used as it may be default constructed if
 * a context was already opened.
 */
#define OPEN_READ_CONTEXT( name, dbConn ) \
    sqlite::Connection::ReadContext name; \
    if ( sqlite::Connection::Context::isOpened( \
            sqlite::Connection::Context::Type::Read ) == false ) { \
        name = dbConn->acquireReadContext(); \
    }

/*
 * Conditionally open a write context if no context is currently opened.
 * The first macro parameter is the context instance name, the 2nd a pointer to
 * a sqlite::Connection instance.
 * The resulting object must not be used as it may be default constructed if
 * a context was already opened.
 */
#define OPEN_WRITE_CONTEXT( name, dbConn ) \
    sqlite::Connection::WriteContext name; \
    if ( sqlite::Connection::Context::isOpened( \
            sqlite::Connection::Context::Type::Write ) == false ) { \
        name = dbConn->acquireWriteContext(); \
    }

namespace medialibrary
{

namespace sqlite
{

class Transaction;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using Handle = sqlite3*;

    /**
     * @brief Represents a generic acquired context, which can be read or write
     *
     * This base class allows the caller to acquire a connection handle, which is
     * otherwise inaccessible.
     */
    class Context
    {
    public:
        enum class Type : uint8_t
        {
            None,
            Read,
            Write,
        };

        Context() noexcept = default;
        ~Context();

        /**
         * @brief handle Returns a connection handle for the calling thread
         *
         * It is invalid to call this function if no context is opened
         */
        static Handle handle();
        /**
         * @brief isOpened Returns true if the calling thread has acquired a context
         * @param t The type of context to check for.
         * @return true if a context is opened, false otherwise
         */
        static bool isOpened( Type t );

    protected:
        void connect( Connection* c, Type t );
        void releaseHandle();

        Context( const Context& ) = delete;
        Context& operator=( const Context& ) = delete;
        Context( Context&& ctx ) noexcept;
        Context& operator=( Context&& ctx ) noexcept;

    private:
        static thread_local Handle m_handle;
        static thread_local Type m_type;
        bool m_owning = false;
    };

    /**
     * @brief The ReadContext class represents a read context to the database
     *
     * Performing writes to the database with such a context is undefined
     */
    class ReadContext final : public Context
    {
    public:
        ReadContext() = default;
        ReadContext( Connection* c );
        ReadContext( const ReadContext& ) = delete;
        ReadContext& operator=( const ReadContext& ) = delete;
        ReadContext( ReadContext&& ) = default;
        ReadContext& operator=( ReadContext&& ) = default;
    private:
        std::unique_lock<utils::ReadLocker> m_lock;
    };

    /**
     * @brief The WriteContext class represents an opened write context
     *
     * Performing writes or read to the database with such a context is valid
     */
    class WriteContext final : public Context
    {
    public:
        WriteContext() = default;
        WriteContext( Connection* c );

        void unlock();

        WriteContext( const WriteContext& ) = delete;
        WriteContext& operator=( const WriteContext& ) = delete;
        WriteContext( WriteContext&& ) = default;
        WriteContext& operator=( WriteContext&& ) = default;

    private:
        std::unique_lock<utils::WriteLocker> m_lock;
    };

    /**
     * @brief The PriorityContext class represents a priority context
     *
     * After acquiring such a context, the calling thread has priority over the
     * others if it tries to acquire a read or write context.
     * This is *not* a read/write context in itself, and doesn't expose a handle
     * to the database
     */
    class PriorityContext final : public Context
    {
    public:
        PriorityContext() = default;
        PriorityContext( Connection* c );

        PriorityContext( const PriorityContext& ) = delete;
        PriorityContext& operator=( const PriorityContext& ) = delete;
        PriorityContext( PriorityContext&& ) = default;
        PriorityContext& operator=( PriorityContext&& ) = default;

    private:
        std::unique_lock<utils::PriorityLocker> m_lock;
    };

    enum class HookReason
    {
        Insert,
        Delete,
        Update
    };

    struct DisableForeignKeyContext
    {
        explicit DisableForeignKeyContext( Connection* conn );
        ~DisableForeignKeyContext();
    private:
        Connection* m_conn;
    };

    /**
     * @brief The WeakDbContext struct exposes a context with foreign key &
     * recursive triggers disabled.
     *
     * This is useful for some migrations which will delete & recreate some
     * entities but for which we don't want those deletions to propagate
     */
    struct WeakDbContext
    {
        explicit WeakDbContext( Connection* conn );
        ~WeakDbContext();
        WeakDbContext( const WeakDbContext& ) = delete;
        WeakDbContext( WeakDbContext&& ) = delete;
        WeakDbContext& operator=( const WeakDbContext& ) = delete;
        WeakDbContext& operator=( WeakDbContext&& ) = delete;
    private:
        Connection* m_conn;
        DisableForeignKeyContext m_fkeyCtx;
    };

    using UpdateHookCb = std::function<void(HookReason, int64_t)>;

    /**
     * @brief newTransaction Creates a transaction and acquires a write context
     * @return A transaction object
     *
     * This is safe to call recursively, only the first returned transaction
     * object will actually perform operations, the later will be noops, but will
     * not deadlock trying to acquire a second write context.
     */
    std::unique_ptr<sqlite::Transaction> newTransaction();
    /**
     * @brief acquireReadContext Acquires a read context
     * @return A ReadContext object
     *
     * This is not safe to be called recursively.
     * If the caller might already hold a read context, OPEN_READ_CONTEXT can be used
     */
    ReadContext acquireReadContext();
    /**
     * @brief acquireWriteContext Acquires a write context
     * @return A WriteContext object
     *
     * This is not safe to be called recursively.
     * If the caller might already hold a read context, OPEN_WRITE_CONTEXT can be used
     */
    WriteContext acquireWriteContext();
    /**
     * @brief acquirePriorityContext Acquires a priority context
     * @return A priority context object
     */
    PriorityContext acquirePriorityContext();

    /**
     * @brief registerUpdateHook Register a hook on the provided table
     * @param table The table on which to hook
     * @param cb The callback to invoke when an event occurs on the given table
     *
     * This must be called after initializing the Connection object and before
     * other threads have connected to the database.
     * Once registered, hooks will be invoked for all connection, regardless of
     * the thread that executed the request.
     */
    void registerUpdateHook( const std::string& table, UpdateHookCb cb );
    bool checkSchemaIntegrity();
    bool checkForeignKeysIntegrity();
    const std::string& dbPath() const;
    /**
     * @brief flushAll Closes all connections for all threads
     */
    void flushAll();

    static std::shared_ptr<Connection> connect( const std::string& dbPath );

protected:
    explicit Connection( const std::string& dbPath );
    ~Connection();

private:
    Connection( const Connection& ) = delete;
    Connection( Connection&& ) = delete;
    Connection& operator=( const Connection& ) = delete;
    Connection& operator=( Connection&& ) = delete;

    void setPragma( Handle conn, const std::string& pragmaName,
                    const std::string& value );

    /**
     * @brief setForeignKeyEnabled Enables/disables foreign key for the sqlite
     *        connection for the current thread.
     *
     * This will not change existing connection in other threads
     */
    void setForeignKeyEnabled( bool value );
    /**
     * @brief setRecursiveTriggersEnabled Enables/disables recursive trigger for
     *          the connection for the current thread.
     *
     * This will not change existing connection in other threads
     */
    void setRecursiveTriggersEnabled( bool value );

    // Returns the current thread's connection
    // This will initiate a connection if required
    Handle handle();

    static void updateHook( void* data, int reason, const char* database,
                            const char* table, sqlite_int64 rowId );

protected:
    /* This is protected in order to be accessible from test code */
    static int collateFilename( void* data, int lhsSize, const void* lhs,
                                int rhsSize, const void* rhs );
private:
    struct ThreadSpecificConnection
    {
        explicit ThreadSpecificConnection( std::shared_ptr<Connection> conn );
        ~ThreadSpecificConnection();

    private:
        std::weak_ptr<Connection> m_weakConnection;
    };

    /*
     * Wrapper object to sqlite_config calls, to ensure we call those only once
     * per process
     * Not doing so will result in sqlite_misuse and invocation of the log callback
     * for every subsequent calls to sqlite3_config
     */
    struct SqliteConfigurator
    {
        SqliteConfigurator();
        SqliteConfigurator( const SqliteConfigurator& ) = delete;
        SqliteConfigurator& operator=( const SqliteConfigurator& ) = delete;
        SqliteConfigurator( SqliteConfigurator&& ) = delete;
        SqliteConfigurator& operator=( SqliteConfigurator&& ) = delete;
    };

    using ConnPtr = std::unique_ptr<sqlite3, int(*)(sqlite3*)>;
    std::string m_dbPath;
    compat::Mutex m_connMutex;
    std::unordered_map<compat::Thread::id, ConnPtr> m_conns;
    utils::SWMRLock m_contextLock;
    utils::ReadLocker m_readLock;
    utils::WriteLocker m_writeLock;
    utils::PriorityLocker m_priorityLock;
    std::unordered_map<utils::StringKey, UpdateHookCb> m_hooks;
};

}

}
