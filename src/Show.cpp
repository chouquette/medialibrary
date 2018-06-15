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

#include "Show.h"

#include "Media.h"
#include "ShowEpisode.h"
#include "MediaLibrary.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"

namespace medialibrary
{

const std::string policy::ShowTable::Name = "Show";
const std::string policy::ShowTable::PrimaryKeyColumn = "id_show";
int64_t Show::* const policy::ShowTable::PrimaryKey = &Show::m_id;

Show::Show( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    row >> m_id
        >> m_name
        >> m_releaseDate
        >> m_shortSummary
        >> m_artworkMrl
        >> m_tvdbId;
}

Show::Show( MediaLibraryPtr ml, const std::string& name )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( name )
    , m_releaseDate( 0 )
{
}

int64_t Show::id() const
{
    return m_id;
}

const std::string& Show::name() const
{
    return m_name;
}

time_t Show::releaseDate() const
{
    return m_releaseDate;
}

bool Show::setReleaseDate( time_t date )
{
    static const std::string req = "UPDATE " + policy::ShowTable::Name
            + " SET release_date = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, date, m_id ) == false )
        return false;
    m_releaseDate = date;
    return true;
}

const std::string& Show::shortSummary() const
{
    return m_shortSummary;
}

bool Show::setShortSummary( const std::string& summary )
{
    static const std::string req = "UPDATE " + policy::ShowTable::Name
            + " SET short_summary = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, summary, m_id ) == false )
        return false;
    m_shortSummary = summary;
    return true;
}

const std::string& Show::artworkMrl() const
{
    return m_artworkMrl;
}

bool Show::setArtworkMrl( const std::string& artworkMrl )
{
    static const std::string req = "UPDATE " + policy::ShowTable::Name
            + " SET artwork_mrl = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, artworkMrl, m_id ) == false )
        return false;
    m_artworkMrl = artworkMrl;
    return true;
}

const std::string& Show::tvdbId()
{
    return m_tvdbId;
}

bool Show::setTvdbId( const std::string& tvdbId )
{
    static const std::string req = "UPDATE " + policy::ShowTable::Name
            + " SET tvdb_id = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, tvdbId, m_id ) == false )
        return false;
    m_tvdbId = tvdbId;
    return true;
}

std::shared_ptr<ShowEpisode> Show::addEpisode( Media& media, const std::string& title, unsigned int episodeNumber)
{
    auto episode = ShowEpisode::create( m_ml, media.id(), title, episodeNumber, m_id );
    media.setShowEpisode( episode );
    media.save();
    return episode;
}

Query<IShowEpisode> Show::episodes()
{
    static const std::string req = "FROM " + policy::ShowEpisodeTable::Name
            + " WHERE show_id = ?";
    return make_query<ShowEpisode, IShowEpisode>( m_ml, "*", req, m_id );
}

uint32_t Show::nbSeasons() const
{
    return 0;
}

uint32_t Show::nbEpisodes() const
{
    return 0;
}

void Show::createTable( sqlite::Connection* dbConnection )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::ShowTable::Name + "("
                        "id_show INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "name TEXT, "
                        "release_date UNSIGNED INTEGER,"
                        "short_summary TEXT,"
                        "artwork_mrl TEXT,"
                        "tvdb_id TEXT"
                    ")";
    sqlite::Tools::executeRequest( dbConnection, req );
}

std::shared_ptr<Show> Show::create( MediaLibraryPtr ml, const std::string& name )
{
    auto show = std::make_shared<Show>( ml, name );
    static const std::string req = "INSERT INTO " + policy::ShowTable::Name
            + "(name) VALUES(?)";
    if ( insert( ml, show, req, name ) == false )
        return nullptr;
    return show;
}

}
