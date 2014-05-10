#include <iostream>

#include "SqliteTools.h"

bool SqliteTools::CreateTable( sqlite3 *db, const std::string& request )
{
    sqlite3_stmt* stmt;
    int res = sqlite3_prepare_v2( db, request.c_str(), -1, &stmt, NULL );
    if ( res != SQLITE_OK )
    {
        std::cerr << "Failed to execute request: " << request << std::endl;
        return false;
    }
    res = sqlite3_step( stmt );
    while ( res != SQLITE_DONE && res != SQLITE_ERROR )
        res = sqlite3_step( stmt );
    return res == SQLITE_DONE;
}
