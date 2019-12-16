/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "ShowEpisode.h"
#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"
#include "Show.h"
#include "Media.h"

namespace medialibrary
{

const std::string ShowEpisode::Table::Name = "ShowEpisode";
const std::string ShowEpisode::Table::PrimaryKeyColumn = "id_episode";
int64_t ShowEpisode::* const ShowEpisode::Table::PrimaryKey = &ShowEpisode::m_id;

ShowEpisode::ShowEpisode( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_mediaId( row.extract<decltype(m_mediaId)>() )
    , m_episodeId( row.extract<decltype(m_episodeId)>() )
    , m_seasonId( row.extract<decltype(m_seasonId)>() )
    , m_shortSummary( row.extract<decltype(m_shortSummary)>() )
    , m_tvdbId( row.extract<decltype(m_tvdbId)>() )
    , m_showId( row.extract<decltype(m_showId)>() )
{
    assert( row.hasRemainingColumns() == false );
}

ShowEpisode::ShowEpisode( MediaLibraryPtr ml, int64_t mediaId, uint32_t seasonId,
                          uint32_t episodeNumber, int64_t showId )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_episodeId( episodeNumber )
    , m_seasonId( seasonId )
    , m_showId( showId )
{
}

int64_t ShowEpisode::id() const
{
    return m_id;
}

unsigned int ShowEpisode::episodeId() const
{
    return m_episodeId;
}

unsigned int ShowEpisode::seasonId() const
{
    return m_seasonId;
}

const std::string& ShowEpisode::shortSummary() const
{
    return m_shortSummary;
}

bool ShowEpisode::setShortSummary( const std::string& summary )
{
    static const std::string req = "UPDATE " + ShowEpisode::Table::Name
            + " SET episode_summary = ? WHERE id_episode = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, summary, m_id ) == false )
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
    static const std::string req = "UPDATE " + ShowEpisode::Table::Name
            + " SET tvdb_id = ? WHERE id_episode = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, tvdbId, m_id ) == false )
        return false;
    m_tvdbId = tvdbId;
    return true;
}

std::shared_ptr<IShow> ShowEpisode::show()
{
    if ( m_show == nullptr && m_showId != 0 )
    {
        m_show = Show::fetch( m_ml, m_showId );
    }
    return m_show;
}

void ShowEpisode::createTable( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
}

void ShowEpisode::createIndexes( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::MediaIdShowId, Settings::DbModelVersion ) );
}

std::string ShowEpisode::schema( const std::string& tableName, uint32_t )
{
    assert( tableName == Table::Name );
    return "CREATE TABLE " + Table::Name +
    "("
        "id_episode INTEGER PRIMARY KEY AUTOINCREMENT,"
        "media_id UNSIGNED INTEGER NOT NULL,"
        "episode_number UNSIGNED INT,"
        "season_number UNSIGNED INT,"
        "episode_summary TEXT,"
        "tvdb_id TEXT,"
        "show_id UNSIGNED INT,"
        "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name
            + "(id_media) ON DELETE CASCADE,"
        "FOREIGN KEY(show_id) REFERENCES " + Show::Table::Name
            + "(id_show) ON DELETE CASCADE"
    ")";
}

std::string ShowEpisode::index( Indexes index, uint32_t )
{
    assert( index == Indexes::MediaIdShowId );
    return "CREATE INDEX show_episode_media_show_idx ON " +
               Table::Name + "(media_id, show_id)";
}

bool ShowEpisode::checkDbModel(MediaLibraryPtr ml)
{
    return sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name );
}

std::shared_ptr<ShowEpisode> ShowEpisode::create( MediaLibraryPtr ml,
                                                  int64_t mediaId,
                                                  uint32_t seasonId,
                                                  uint32_t episodeId,
                                                  int64_t showId )
{
    auto episode = std::make_shared<ShowEpisode>( ml, mediaId, seasonId, episodeId,
                                                  showId );
    static const std::string req = "INSERT INTO " + ShowEpisode::Table::Name
            + "(media_id, episode_number, season_number, show_id) VALUES(?, ?, ?, ?)";
    if ( insert( ml, episode, req, mediaId, episodeId, seasonId, showId ) == false )
        return nullptr;
    return episode;
}

ShowEpisodePtr ShowEpisode::fromMedia( MediaLibraryPtr ml, int64_t mediaId )
{
    static const std::string req = "SELECT * FROM " + ShowEpisode::Table::Name + " WHERE media_id = ?";
    return fetch( ml, req, mediaId );
}

}
