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
    if ( sqlite::Connection::Context::isOpened() == false ) { \
        name = dbConn->acquireReadContext(); \
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

    class Context
    {
    public:
        Context() noexcept = default;
        ~Context();

        static Handle handle();
        static bool isOpened();

    protected:
        void connect( Connection* c );
        void releaseHandle();

        Context( const Context& ) = delete;
        Context& operator=( const Context& ) = delete;
        Context( Context&& ctx ) noexcept;
        Context& operator=( Context&& ctx ) noexcept;

    private:
        static thread_local Handle m_handle;
        bool m_owning = false;
    };

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

    std::unique_ptr<sqlite::Transaction> newTransaction();
    ReadContext acquireReadContext();
    WriteContext acquireWriteContext();
    PriorityContext acquirePriorityContext();

    void registerUpdateHook( const std::string& table, UpdateHookCb cb );
    bool checkSchemaIntegrity();
    bool checkForeignKeysIntegrity();
    const std::string& dbPath() const;
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
