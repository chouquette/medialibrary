#ifndef SHOWEPISODE_H
#define SHOWEPISODE_H

class Show;

#include <string>
#include <sqlite3.h>

#include "IShowEpisode.h"

class ShowEpisode : public IShowEpisode
{
    public:
        ShowEpisode(sqlite3* dbConnection, sqlite3_stmt* stmt);

        virtual const std::string& artworkUrl();
        virtual unsigned int episodeNumber();
        virtual time_t lastSyncDate();
        virtual const std::string& name();
        virtual unsigned int seasonNuber();
        virtual const std::string& shortSummary();
        virtual const std::string& tvdbId();
        virtual IShow*show();

        static bool createTable( sqlite3* dbConnection );

    private:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_artworkUrl;
        unsigned int m_episodeNumber;
        time_t m_lastSyncDate;
        std::string m_name;
        unsigned int m_seasonNumber;
        std::string m_shortSummary;
        std::string m_tvdbId;
        unsigned int m_showId;
        Show* m_show;
};

#endif // SHOWEPISODE_H
