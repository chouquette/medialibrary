#include "ShowEpisode.h"
#include "SqliteTools.h"
#include "Show.h"

const std::string policy::ShowEpisodeTable::Name = "Show";
const std::string policy::ShowEpisodeTable::CacheColumn = "id_show";

ShowEpisode::ShowEpisode( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_artworkUrl = (const char*)sqlite3_column_text( stmt, 1 );
    m_episodeNumber = sqlite3_column_int( stmt, 2 );
    m_lastSyncDate = sqlite3_column_int( stmt, 3 );
    m_name = (const char*)sqlite3_column_text( stmt, 4 );
    m_seasonNumber = sqlite3_column_int( stmt, 5 );
    m_shortSummary = (const char*)sqlite3_column_text( stmt, 6 );
    m_tvdbId = (const char*)sqlite3_column_text( stmt, 7 );
    m_showId = sqlite3_column_int( stmt, 8 );
}


const std::string& ShowEpisode::artworkUrl()
{
    return m_artworkUrl;
}

unsigned int ShowEpisode::episodeNumber()
{
    return m_episodeNumber;
}

time_t ShowEpisode::lastSyncDate()
{
    return m_lastSyncDate;
}

const std::string&ShowEpisode::name()
{
    return m_name;
}

unsigned int ShowEpisode::seasonNuber()
{
    return m_seasonNumber;
}

const std::string&ShowEpisode::shortSummary()
{
    return m_shortSummary;
}

const std::string&ShowEpisode::tvdbId()
{
    return m_tvdbId;
}

std::shared_ptr<IShow> ShowEpisode::show()
{
    if ( m_show == nullptr && m_showId != 0 )
    {
        m_show = Show::fetch( m_dbConnection, m_showId );
    }
    return m_show;
}

bool ShowEpisode::createTable(sqlite3* dbConnection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS ShowEpisode("
                "id_episode INTEGER PRIMARY KEY AUTOINCREMENT,"
                "artwork_url TEXT,"
                "episode_number UNSIGNED INT,"
                "last_sync_date UNSIGNED INT,"
                "name TEXT,"
                "season_number UNSIGNED INT,"
                "episode_summary TEXT,"
                "tvdb_id TEXT,"
                "show_id UNSIGNED INT,"
                "FOREIGN KEY(show_id) REFERENCES Show(id_show)"
            ")";
    return SqliteTools::createTable( dbConnection, req );
}
