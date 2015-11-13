/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "ShowEpisode.h"
#include "database/SqliteTools.h"
#include "Show.h"
#include "Media.h"

const std::string policy::ShowEpisodeTable::Name = "ShowEpisode";
const std::string policy::ShowEpisodeTable::PrimaryKeyColumn = "show_id";
unsigned int ShowEpisode::* const policy::ShowEpisodeTable::PrimaryKey = &ShowEpisode::m_id;

ShowEpisode::ShowEpisode(DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_artworkUrl
        >> m_episodeNumber
        >> m_lastSyncDate
        >> m_name
        >> m_seasonNumber
        >> m_shortSummary
        >> m_tvdbId
        >> m_showId;
}

ShowEpisode::ShowEpisode( const std::string& name, unsigned int episodeNumber, unsigned int showId )
    : m_id( 0 )
    , m_episodeNumber( episodeNumber )
    , m_name( name )
    , m_seasonNumber( 0 )
    , m_showId( showId )
{
}

unsigned int ShowEpisode::id() const
{
    return m_id;
}

const std::string& ShowEpisode::artworkUrl() const
{
    return m_artworkUrl;
}

bool ShowEpisode::setArtworkUrl( const std::string& artworkUrl )
{
    static const std::string req = "UPDATE " + policy::ShowEpisodeTable::Name
            + " SET artwork_url = ? WHERE id_episode = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artworkUrl, m_id ) == false )
        return false;
    m_artworkUrl = artworkUrl;
    return true;
}

unsigned int ShowEpisode::episodeNumber() const
{
    return m_episodeNumber;
}

time_t ShowEpisode::lastSyncDate() const
{
    return m_lastSyncDate;
}

const std::string& ShowEpisode::name() const
{
    return m_name;
}

unsigned int ShowEpisode::seasonNumber() const
{
    return m_seasonNumber;
}

bool ShowEpisode::setSeasonNumber( unsigned int seasonNumber )
{
    static const std::string req = "UPDATE " + policy::ShowEpisodeTable::Name
            + " SET season_number = ? WHERE id_episode = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, seasonNumber, m_id ) == false )
        return false;
    m_seasonNumber = seasonNumber;
    return true;
}

const std::string& ShowEpisode::shortSummary() const
{
    return m_shortSummary;
}

bool ShowEpisode::setShortSummary( const std::string& summary )
{
    static const std::string req = "UPDATE " + policy::ShowEpisodeTable::Name
            + " SET episode_summary = ? WHERE id_episode = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, summary, m_id ) == false )
        return false;
    m_shortSummary = summary;
    return true;
}

const std::string& ShowEpisode::tvdbId() const
{
    return m_tvdbId;
}

bool ShowEpisode::setTvdbId( const std::string& tvdbId )
{
    static const std::string req = "UPDATE " + policy::ShowEpisodeTable::Name
            + " SET tvdb_id = ? WHERE id_episode = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, tvdbId, m_id ) == false )
        return false;
    m_tvdbId = tvdbId;
    return true;
}

std::shared_ptr<IShow> ShowEpisode::show()
{
    if ( m_show == nullptr && m_showId != 0 )
    {
        m_show = Show::fetch( m_dbConnection, m_showId );
    }
    return m_show;
}

std::vector<MediaPtr> ShowEpisode::files()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name
            + " WHERE show_episode_id = ?";
    return Media::fetchAll<IMedia>( m_dbConnection, req, m_id );
}

bool ShowEpisode::createTable( DBConnection dbConnection )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::ShowEpisodeTable::Name
            + "("
                "id_episode INTEGER PRIMARY KEY AUTOINCREMENT,"
                "artwork_url TEXT,"
                "episode_number UNSIGNED INT,"
                "last_sync_date UNSIGNED INT,"
                "title TEXT,"
                "season_number UNSIGNED INT,"
                "episode_summary TEXT,"
                "tvdb_id TEXT,"
                "show_id UNSIGNED INT,"
                "FOREIGN KEY(show_id) REFERENCES " + policy::ShowTable::Name + "(id_show)"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

std::shared_ptr<ShowEpisode> ShowEpisode::create( DBConnection dbConnection, const std::string& title, unsigned int episodeNumber, unsigned int showId )
{
    auto episode = std::make_shared<ShowEpisode>( title, episodeNumber, showId );
    static const std::string req = "INSERT INTO " + policy::ShowEpisodeTable::Name
            + "(episode_number, title, show_id) VALUES(? , ?, ?)";
    if ( insert( dbConnection, episode, req, episodeNumber, title, showId ) == false )
        return nullptr;
    episode->m_dbConnection = dbConnection;
    return episode;
}
