#ifndef SHOW_H
#define SHOW_H

#include <sqlite3.h>

#include "IShow.h"

class Show : public IShow
{
    public:
        Show( sqlite3* dbConnection, sqlite3_stmt* stmt );
        Show( sqlite3* dbConnection );

        virtual const std::string& name();
        virtual unsigned int releaseYear();
        virtual const std::string& shortSummary();
        virtual const std::string& artworkUrl();
        virtual time_t lastSyncDate();
        virtual const std::string& tvdbId();

        static bool createTable( sqlite3* dbConnection );

    protected:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_name;
        unsigned int m_releaseYear;
        std::string m_shortSummary;
        std::string m_artworkUrl;
        time_t m_lastSyncDate;
        std::string m_tvdbId;
};

#endif // SHOW_H
