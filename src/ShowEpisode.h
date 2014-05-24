#ifndef SHOWEPISODE_H
#define SHOWEPISODE_H

class Show;
class ShowEpisode;

#include <string>
#include <sqlite3.h>

#include "IMediaLibrary.h"
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
        ShowEpisode(const std::string& name, unsigned int episodeNumber, unsigned int showId );

        virtual unsigned int id() const;
        virtual const std::string& artworkUrl() const;
        virtual bool setArtworkUrl( const std::string& artworkUrl );
        virtual unsigned int episodeNumber() const;
        virtual time_t lastSyncDate() const;
        virtual const std::string& name() const;
        virtual unsigned int seasonNumber() const;
        virtual bool setSeasonNumber(unsigned int seasonNumber);
        virtual const std::string& shortSummary() const;
        virtual bool setShortSummary( const std::string& summary );
        virtual const std::string& tvdbId() const;
        virtual bool setTvdbId( const std::string& tvdbId );
        virtual std::shared_ptr<IShow> show();
        virtual bool files( std::vector<FilePtr>& files );
        virtual bool destroy();

        static bool createTable( sqlite3* dbConnection );
        static ShowEpisodePtr create(sqlite3* dbConnection, const std::string& title, unsigned int episodeNumber, unsigned int showId );

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
        ShowPtr m_show;

        friend class Cache<ShowEpisode, IShowEpisode, policy::ShowEpisodeTable>;
        friend struct policy::ShowEpisodeTable;
        typedef Cache<ShowEpisode, IShowEpisode, policy::ShowEpisodeTable> _Cache;
};

#endif // SHOWEPISODE_H
