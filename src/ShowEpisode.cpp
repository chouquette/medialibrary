#include "ShowEpisode.h"
#include "SqliteTools.h"
#include "Show.h"

ShowEpisode::ShowEpisode( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 1 );
    m_artworkUrl = (const char*)sqlite3_column_text( stmt, 2 );
    m_episodeNumber = sqlite3_column_int( stmt, 3 );
    m_lastSyncDate = sqlite3_column_int( stmt, 4 );
    m_name = (const char*)sqlite3_column_text( stmt, 5 );
    m_seasonNumber = sqlite3_column_int( stmt, 6 );
    m_shortSummary = (const char*)sqlite3_column_text( stmt, 7 );
    m_tvdbId = (const char*)sqlite3_column_text( stmt, 8 );
    m_showId = sqlite3_column_int( stmt, 9 );
}


const std::string&ShowEpisode::artworkUrl()
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

IShow* ShowEpisode::show()
{
    if ( m_show == NULL && m_showId != 0 )
    {
        const char* req = "SELECT * FROM Show WHERE id_show = ?";
        m_show = SqliteTools::fetchOne<Show>( m_dbConnection, req, m_showId );
    }
    return m_show;
}

bool ShowEpisode::createTable(sqlite3* dbConnection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS ("
                "id_episode INTEGER PRIMARY KEY AUTO INCREMENT,"
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
    SqliteTools::createTable( dbConnection, req );
}
