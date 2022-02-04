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
    , m_title( row.extract<decltype(m_title)>() )
    , m_shortSummary( row.extract<decltype(m_shortSummary)>() )
    , m_tvdbId( row.extract<decltype(m_tvdbId)>() )
    , m_showId( row.extract<decltype(m_showId)>() )
{
    assert( row.hasRemainingColumns() == false );
}

ShowEpisode::ShowEpisode( MediaLibraryPtr ml, int64_t mediaId, uint32_t seasonId,
                          uint32_t episodeId, std::string title, int64_t showId )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_episodeId( episodeId )
    , m_seasonId( seasonId )
    , m_title( std::move( title ) )
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
    static const std::string req = "UPDATE " + Table::Name
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
    static const std::string req = "UPDATE " + Table::Name
            + " SET tvdb_id = ? WHERE id_episode = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, tvdbId, m_id ) == false )
        return false;
    m_tvdbId = tvdbId;
    return true;
}

const std::string&ShowEpisode::title() const
{
    return m_title;
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
                                   index( Indexes::MediaId, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::ShowId, Settings::DbModelVersion ) );
}

std::string ShowEpisode::schema( const std::string& tableName, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( tableName );

    assert( tableName == Table::Name );
    if ( dbModel <= 23 )
    {
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
    return "CREATE TABLE " + Table::Name +
    "("
        "id_episode INTEGER PRIMARY KEY AUTOINCREMENT,"
        "media_id UNSIGNED INTEGER NOT NULL,"
        "episode_number UNSIGNED INT,"
        "season_number UNSIGNED INT,"
        "episode_title TEXT,"
        "episode_summary TEXT,"
        "tvdb_id TEXT,"
        "show_id UNSIGNED INT,"
        "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name
            + "(id_media) ON DELETE CASCADE,"
        "FOREIGN KEY(show_id) REFERENCES " + Show::Table::Name
            + "(id_show) ON DELETE CASCADE"
    ")";
}

std::string ShowEpisode::index( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::MediaId:
            if ( dbModel < 34 )
            {
                return "CREATE INDEX " + indexName( index, dbModel ) +
                       " ON " + Table::Name + "(media_id, show_id)";
            }
            return "CREATE INDEX " + indexName( index, dbModel ) +
                   " ON " + Table::Name + "(media_id)";
        case Indexes::ShowId:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) +
                   " ON " + Table::Name + "(show_id)";
    }
    return "<invalid request>";
}

std::string ShowEpisode::indexName( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::MediaId:
            if ( dbModel < 34 )
                return "show_episode_media_show_idx";
            return "show_episode_media_idx";
        case Indexes::ShowId:
            return "show_episode_show_id_idx";
    }
    return "<invalid request>";
}

bool ShowEpisode::checkDbModel(MediaLibraryPtr ml)
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );

    return sqlite::Tools::checkTableSchema(
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) &&
           sqlite::Tools::checkIndexStatement(
                index( Indexes::MediaId, Settings::DbModelVersion ),
                indexName( Indexes::MediaId, Settings::DbModelVersion ) ) &&
            sqlite::Tools::checkIndexStatement(
                 index( Indexes::ShowId, Settings::DbModelVersion ),
                 indexName( Indexes::ShowId, Settings::DbModelVersion ) );
}

std::shared_ptr<ShowEpisode> ShowEpisode::create( MediaLibraryPtr ml,
                                                  int64_t mediaId,
                                                  uint32_t seasonId,
                                                  uint32_t episodeId,
                                                  std::string title,
                                                  int64_t showId )
{
    auto episode = std::make_shared<ShowEpisode>( ml, mediaId, seasonId, episodeId,
                                                  std::move( title ), showId );
    static const std::string req = "INSERT INTO " + Table::Name
            + "(media_id, episode_number, season_number, episode_title, show_id)"
            " VALUES(?, ?, ?, ?, ?)";
    if ( insert( ml, episode, req, mediaId, episodeId, seasonId,
                 episode->title(), showId ) == false )
        return nullptr;
    return episode;
}

ShowEpisodePtr ShowEpisode::fromMedia( MediaLibraryPtr ml, int64_t mediaId )
{
    static const std::string req = "SELECT * FROM " + Table::Name + " WHERE media_id = ?";
    return fetch( ml, req, mediaId );
}

bool ShowEpisode::deleteByMediaId( MediaLibraryPtr ml, int64_t mediaId )
{
    const std::string req = "DELETE FROM " + Table::Name + " WHERE media_id = ?";
    return sqlite::Tools::executeDelete( ml->getConn(), req, mediaId );
}

}
