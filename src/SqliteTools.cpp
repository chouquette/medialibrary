#include <iostream>

#include "SqliteTools.h"

bool SqliteTools::createTable( sqlite3 *db, const char* request )
{
    auto stmt = executeRequest( db, request );
    int res = sqlite3_step( stmt.get() );
    while ( res != SQLITE_DONE && res != SQLITE_ERROR )
        res = sqlite3_step( stmt.get() );
    if ( res == SQLITE_ERROR )
    {
        std::cerr << "Failed to execute request: " << request << std::endl;
        std::cerr << sqlite3_errmsg( db ) << std::endl;
    }
    return res == SQLITE_DONE;
}
