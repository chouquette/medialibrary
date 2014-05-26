#ifndef SHOW_H
#define SHOW_H

#include <sqlite3.h>

#include "Cache.h"
#include "IMediaLibrary.h"
#include "IShow.h"

class Show;

namespace policy
{
struct ShowTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Show::*const PrimaryKey;
};
}

class Show : public IShow, public Cache<Show, IShow, policy::ShowTable>
{
    public:
        Show( DBConnection dbConnection, sqlite3_stmt* stmt );
        Show( const std::string& name );

        virtual unsigned int id() const;
        virtual const std::string& name() const;
        virtual time_t releaseDate() const;
        virtual bool setReleaseDate( time_t date );
        virtual const std::string& shortSummary() const;
        virtual bool setShortSummary( const std::string& summary );
        virtual const std::string& artworkUrl() const;
        virtual bool setArtworkUrl( const std::string& artworkUrl );
        virtual time_t lastSyncDate() const;
        virtual const std::string& tvdbId();
        virtual bool setTvdbId( const std::string& summary );
        virtual ShowEpisodePtr addEpisode( const std::string& title, unsigned int episodeNumber );
        virtual bool episodes( std::vector<ShowEpisodePtr>& episodes );
        virtual bool destroy();

        static bool createTable( DBConnection dbConnection );
        static ShowPtr create( DBConnection dbConnection, const std::string& name );

    protected:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_name;
        time_t m_releaseDate;
        std::string m_shortSummary;
        std::string m_artworkUrl;
        time_t m_lastSyncDate;
        std::string m_tvdbId;

        friend class Cache<Show, IShow, policy::ShowTable>;
        friend struct policy::ShowTable;
        typedef Cache<Show, IShow, policy::ShowTable> _Cache;
};

#endif // SHOW_H
