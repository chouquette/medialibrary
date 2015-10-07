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
public:
    SqliteConnection( const std::string& dbPath );
    // Returns the current thread's connection
    // This will initiate a connection if required
    sqlite3* getConn();
    // Release the current thread's connection
    void release();

private:
    using ConnPtr = std::unique_ptr<sqlite3, int(*)(sqlite3*)>;
    const std::string m_dbPath;
    std::mutex m_connMutex;
    std::unordered_map<std::thread::id, ConnPtr> m_conns;
};

#endif // SQLITECONNECTION_H
