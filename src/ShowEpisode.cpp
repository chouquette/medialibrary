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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "ShowEpisode.h"
#include "database/SqliteTools.h"
#include "Show.h"
#include "Media.h"

namespace medialibrary
{

const std::string policy::ShowEpisodeTable::Name = "ShowEpisode";
const std::string policy::ShowEpisodeTable::PrimaryKeyColumn = "show_id";
int64_t ShowEpisode::* const policy::ShowEpisodeTable::PrimaryKey = &ShowEpisode::m_id;

ShowEpisode::ShowEpisode( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    row >> m_id
        >> m_mediaId
        >> m_artworkMrl
        >> m_episodeNumber
        >> m_name
        >> m_seasonNumber
        >> m_shortSummary
        >> m_tvdbId
        >> m_showId;
}

ShowEpisode::ShowEpisode( MediaLibraryPtr ml, int64_t mediaId, const std::string& name, unsigned int episodeNumber, int64_t showId )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_episodeNumber( episodeNumber )
    , m_name( name )
    , m_seasonNumber( 0 )
    , m_showId( showId )
{
}

int64_t ShowEpisode::id() const
{
    return m_id;
}

const std::string& ShowEpisode::artworkMrl() const
{
    return m_artworkMrl;
}

bool ShowEpisode::setArtworkMrl( const std::string& artworkMrl )
{
    static const std::string req = "UPDATE " + policy::ShowEpisodeTable::Name
            + " SET artwork_mrl = ? WHERE id_episode = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, artworkMrl, m_id ) == false )
        return false;
    m_artworkMrl = artworkMrl;
    return true;
}

unsigned int ShowEpisode::episodeNumber() const
{
    return m_episodeNumber;
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
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, seasonNumber, m_id ) == false )
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
    static const std::string req = "UPDATE " + policy::ShowEpisodeTable::Name
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

std::vector<MediaPtr> ShowEpisode::files()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name
            + " WHERE show_episode_id = ?";
    return Media::fetchAll<IMedia>( m_ml, req, m_id );
}

void ShowEpisode::createTable( sqlite::Connection* dbConnection )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::ShowEpisodeTable::Name
            + "("
                "id_episode INTEGER PRIMARY KEY AUTOINCREMENT,"
                "media_id UNSIGNED INTEGER NOT NULL,"
                "artwork_mrl TEXT,"
                "episode_number UNSIGNED INT,"
                "title TEXT,"
                "season_number UNSIGNED INT,"
                "episode_summary TEXT,"
                "tvdb_id TEXT,"
                "show_id UNSIGNED INT,"
                "FOREIGN KEY(media_id) REFERENCES " + policy::MediaTable::Name
                    + "(id_media) ON DELETE CASCADE,"
                "FOREIGN KEY(show_id) REFERENCES " + policy::ShowTable::Name
                    + "(id_show) ON DELETE CASCADE"
            ")";
    const std::string indexReq = "CREATE INDEX IF NOT EXISTS show_episode_media_show_idx ON " +
            policy::ShowEpisodeTable::Name + "(media_id, show_id)";
    sqlite::Tools::executeRequest( dbConnection, req );
    sqlite::Tools::executeRequest( dbConnection, indexReq );
}

std::shared_ptr<ShowEpisode> ShowEpisode::create( MediaLibraryPtr ml, int64_t mediaId, const std::string& title, unsigned int episodeNumber, int64_t showId )
{
    auto episode = std::make_shared<ShowEpisode>( ml, mediaId, title, episodeNumber, showId );
    static const std::string req = "INSERT INTO " + policy::ShowEpisodeTable::Name
            + "(media_id, episode_number, title, show_id) VALUES(?, ? , ?, ?)";
    if ( insert( ml, episode, req, mediaId, episodeNumber, title, showId ) == false )
        return nullptr;
    return episode;
}

ShowEpisodePtr ShowEpisode::fromMedia( MediaLibraryPtr ml, int64_t mediaId )
{
    static const std::string req = "SELECT * FROM " + policy::ShowEpisodeTable::Name + " WHERE media_id = ?";
    return fetch( ml, req, mediaId );
}

}
