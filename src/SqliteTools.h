#ifndef SQLITETOOLS_H
#define SQLITETOOLS_H

#include <sqlite3.h>
#include <string>

class SqliteTools
{
    public:
        static bool CreateTable(sqlite3* db, const char* request );
};

#endif // SQLITETOOLS_H
