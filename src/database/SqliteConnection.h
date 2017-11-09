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

#include <functional>
#include <memory>
#include <sqlite3.h>
#include "compat/ConditionVariable.h"
#include <unordered_map>
#include <string>

#include "utils/SWMRLock.h"
#include "compat/Mutex.h"
#include "compat/Thread.h"

namespace medialibrary
{

namespace sqlite
{

class Transaction;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using ReadContext = std::unique_lock<utils::ReadLocker>;
    using WriteContext = std::unique_lock<utils::WriteLocker>;
    using Handle = sqlite3*;
    enum class HookReason
    {
        Insert,
        Delete,
        Update
    };
    struct WeakDbContext
    {
        WeakDbContext( Connection* conn );
        ~WeakDbContext();
        WeakDbContext( const WeakDbContext& ) = delete;
        WeakDbContext( WeakDbContext&& ) = delete;
        WeakDbContext& operator=( const WeakDbContext& ) = delete;
        WeakDbContext& operator=( WeakDbContext&& ) = delete;
    private:
        Connection* m_conn;
    };

    using UpdateHookCb = std::function<void(HookReason, int64_t)>;

    // Returns the current thread's connection
    // This will initiate a connection if required
    Handle handle();
    std::unique_ptr<sqlite::Transaction> newTransaction();
    ReadContext acquireReadContext();
    WriteContext acquireWriteContext();
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

    void registerUpdateHook( const std::string& table, UpdateHookCb cb );

    static std::shared_ptr<Connection> connect( const std::string& dbPath );

protected:
    explicit Connection( const std::string& dbPath );
    ~Connection();

private:
    Connection( const Connection& ) = delete;
    Connection( Connection&& ) = delete;
    Connection& operator=( const Connection& ) = delete;
    Connection& operator=( Connection&& ) = delete;

    void setPragmaEnabled( Handle conn, const std::string& pragmaName, bool value );
    static void updateHook( void* data, int reason, const char* database,
                            const char* table, sqlite_int64 rowId );

private:
    struct ThreadSpecificConnection
    {
        ThreadSpecificConnection( std::shared_ptr<Connection> conn );
        ~ThreadSpecificConnection();

    private:
        std::shared_ptr<Connection> m_conn;
    };

    using ConnPtr = std::unique_ptr<sqlite3, int(*)(sqlite3*)>;
    const std::string m_dbPath;
    compat::Mutex m_connMutex;
    std::unordered_map<compat::Thread::id, ConnPtr> m_conns;
    utils::SWMRLock m_contextLock;
    utils::ReadLocker m_readLock;
    utils::WriteLocker m_writeLock;
    std::unordered_map<std::string, UpdateHookCb> m_hooks;
};

}

}
