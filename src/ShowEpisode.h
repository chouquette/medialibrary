#ifndef SHOWEPISODE_H
#define SHOWEPISODE_H

class Show;
class ShowEpisode;

#include <string>
#include <sqlite3.h>

#include "IShowEpisode.h"
#include "Cache.h"

namespace policy
{
struct ShowEpisodeTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int ShowEpisode::*const PrimaryKey;
};
}

class ShowEpisode : public IShowEpisode, public Cache<ShowEpisode, IShowEpisode, policy::ShowEpisodeTable>
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
        virtual std::shared_ptr<IShow> show();

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
        std::shared_ptr<Show> m_show;

        friend class Cache<ShowEpisode, IShowEpisode, policy::ShowEpisodeTable>;
        friend class policy::ShowEpisodeTable;
};

#endif // SHOWEPISODE_H
