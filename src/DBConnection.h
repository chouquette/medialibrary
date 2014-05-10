#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <sqlite3.h>
#include <string>

class DBConnection
{
    public:
        DBConnection();
        bool connect( const std::string& dbPath );
};

#endif // DBCONNECTION_H
