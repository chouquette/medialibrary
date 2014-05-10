#include "DBConnection.h"

DBConnection::DBConnection()
{
}

bool DBConnection::connect(const std::string& dbPath )
{
    sqlite3* stmt;
    int res = sqlite3_open( dbPath.c_str(), &stmt );
    return res == SQLITE_OK;
}
