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

#ifndef SQLITECONNECTION_H
#define SQLITECONNECTION_H

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
}

class SqliteConnection
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
    using UpdateHookCb = std::function<void(HookReason, int64_t)>;

    explicit SqliteConnection( const std::string& dbPath );
    explicit SqliteConnection( const std::string& dbPath, bool enableForeignKeys );
    ~SqliteConnection();
    // Returns the current thread's connection
    // This will initiate a connection if required
    Handle getConn();
    std::unique_ptr<sqlite::Transaction> newTransaction();
    ReadContext acquireReadContext();
    WriteContext acquireWriteContext();
    std::string getDBPath();

    void registerUpdateHook( const std::string& table, UpdateHookCb cb );

private:
    static void updateHook( void* data, int reason, const char* database,
                            const char* table, sqlite_int64 rowId );

private:
    using ConnPtr = std::unique_ptr<sqlite3, int(*)(sqlite3*)>;
    const std::string m_dbPath;
    compat::Mutex m_connMutex;
    std::unordered_map<compat::Thread::id, ConnPtr> m_conns;
    utils::SWMRLock m_contextLock;
    utils::ReadLocker m_readLock;
    utils::WriteLocker m_writeLock;
    std::unordered_map<std::string, UpdateHookCb> m_hooks;
    bool m_enableForeignKeys;
};

}

#endif // SQLITECONNECTION_H
