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

#include <memory>
#include <sqlite3.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>

class SqliteConnection
{
private:
    using RequestContext = std::unique_lock<std::mutex>;
public:
    explicit SqliteConnection( const std::string& dbPath );
    // Returns the current thread's connection
    // This will initiate a connection if required
    sqlite3* getConn();
    // Release the current thread's connection
    void release();
    RequestContext acquireContext();

private:
    using ConnPtr = std::unique_ptr<sqlite3, int(*)(sqlite3*)>;
    const std::string m_dbPath;
    std::mutex m_connMutex;
    std::unordered_map<std::thread::id, ConnPtr> m_conns;
    std::mutex m_contextMutex;
};

#endif // SQLITECONNECTION_H
