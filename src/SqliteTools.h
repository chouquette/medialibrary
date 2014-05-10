#ifndef SQLITETOOLS_H
#define SQLITETOOLS_H

#include <sqlite3.h>
#include <string>
#include <vector>

class SqliteTools
{
    public:
        static bool CreateTable(sqlite3* db, const char* request );

        template <typename T, typename U>
        static bool fetchAll( sqlite3* dbConnection, const char* req, unsigned int foreignKey, std::vector<U*>*& results)
        {
            results = new std::vector<U*>;
            sqlite3_stmt* stmt;
            int res = sqlite3_prepare_v2( dbConnection, req, -1, &stmt, NULL );
            if ( res != SQLITE_OK )
                return false;
            sqlite3_bind_int( stmt, 1, foreignKey );
            res = sqlite3_step( stmt );
            while ( res == SQLITE_ROW )
            {
                U* l = new T( dbConnection, stmt );
                results->push_back( l );
                res = sqlite3_step( stmt );
            }
            sqlite3_finalize( stmt );
            return true;
        }

        template <typename T>
        static T* fetchOne( sqlite3* dbConnection, const char* req, unsigned int primaryKey )
        {
            T* result = NULL;
            sqlite3_stmt *stmt;
            int res = sqlite3_prepare_v2( dbConnection, req, -1, &stmt, NULL );
            if ( res != SQLITE_OK )
                return result;
            sqlite3_bind_int( stmt, 1, primaryKey );
            if ( sqlite3_step( stmt ) != SQLITE_ROW )
                return result;
            result = new T( dbConnection, stmt );
            sqlite3_finalize( stmt );
            return result;
        }


};

#endif // SQLITETOOLS_H
