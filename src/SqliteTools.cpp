#include <iostream>

#include "SqliteTools.h"

bool SqliteTools::createTable( sqlite3 *db, const char* request )
{
    sqlite3_stmt* stmt;
    int res = sqlite3_prepare_v2( db, request, -1, &stmt, NULL );
    if ( res != SQLITE_OK )
    {
        std::cerr << "Failed to execute request: " << request << std::endl;
        std::cerr << sqlite3_errmsg( db ) << std::endl;
        return false;
    }
    res = sqlite3_step( stmt );
    while ( res != SQLITE_DONE && res != SQLITE_ERROR )
        res = sqlite3_step( stmt );
    if ( res == SQLITE_ERROR )
    {
        std::cerr << "Failed to execute request: " << request << std::endl;
        std::cerr << sqlite3_errmsg( db ) << std::endl;
    }
    return res == SQLITE_DONE;
}
