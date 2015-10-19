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

#include "Show.h"
#include "ShowEpisode.h"
#include "database/SqliteTools.h"

const std::string policy::ShowTable::Name = "Show";
const std::string policy::ShowTable::CacheColumn = "id_show";
unsigned int Show::* const policy::ShowTable::PrimaryKey = &Show::m_id;

Show::Show(DBConnection dbConnection, sqlite3_stmt* stmt)
    : m_dbConnection( dbConnection )
{
    m_id = sqlite::Traits<unsigned int>::Load( stmt, 0 );
    m_name = sqlite::Traits<std::string>::Load( stmt, 1 );
    m_releaseDate = sqlite::Traits<unsigned int>::Load( stmt, 2 );
    m_shortSummary = sqlite::Traits<std::string>::Load( stmt, 3 );
    m_artworkUrl = sqlite::Traits<std::string>::Load( stmt, 4 );
    m_lastSyncDate = sqlite::Traits<unsigned int>::Load( stmt, 5 );
    m_tvdbId = sqlite::Traits<std::string>::Load( stmt, 6 );
}

Show::Show( const std::string& name )
    : m_id( 0 )
    , m_name( name )
    , m_releaseDate( 0 )
    , m_lastSyncDate( 0 )
{
}

unsigned int Show::id() const
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
    static const std::string& req = "UPDATE " + policy::ShowTable::Name
            + " SET release_date = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, date, m_id ) == false )
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
    static const std::string& req = "UPDATE " + policy::ShowTable::Name
            + " SET short_summary = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, summary, m_id ) == false )
        return false;
    m_shortSummary = summary;
    return true;
}

const std::string& Show::artworkUrl() const
{
    return m_artworkUrl;
}

bool Show::setArtworkUrl( const std::string& artworkUrl )
{
    static const std::string& req = "UPDATE " + policy::ShowTable::Name
            + " SET artwork_url = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artworkUrl, m_id ) == false )
        return false;
    m_artworkUrl = artworkUrl;
    return true;
}

time_t Show::lastSyncDate() const
{
    return m_lastSyncDate;
}

const std::string& Show::tvdbId()
{
    return m_tvdbId;
}

bool Show::setTvdbId( const std::string& tvdbId )
{
    static const std::string& req = "UPDATE " + policy::ShowTable::Name
            + " SET tvdb_id = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, tvdbId, m_id ) == false )
        return false;
    m_tvdbId = tvdbId;
    return true;
}

std::shared_ptr<ShowEpisode> Show::addEpisode(const std::string& title, unsigned int episodeNumber)
{
    return ShowEpisode::create( m_dbConnection, title, episodeNumber, m_id );
}

std::vector<ShowEpisodePtr> Show::episodes()
{
    static const std::string req = "SELECT * FROM " + policy::ShowEpisodeTable::Name
            + " WHERE show_id = ?";
    return ShowEpisode::fetchAll( m_dbConnection, req, m_id );
}

bool Show::createTable(DBConnection dbConnection)
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::ShowTable::Name + "("
                        "id_show INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "name TEXT, "
                        "release_date UNSIGNED INTEGER,"
                        "short_summary TEXT,"
                        "artwork_url TEXT,"
                        "last_sync_date UNSIGNED INTEGER,"
                        "tvdb_id TEXT"
                    ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

std::shared_ptr<Show> Show::create(DBConnection dbConnection, const std::string& name )
{
    auto show = std::make_shared<Show>( name );
    static const std::string req = "INSERT INTO " + policy::ShowTable::Name
            + "(name) VALUES(?)";
    if ( _Cache::insert( dbConnection, show, req, name ) == false )
        return nullptr;
    show->m_dbConnection = dbConnection;
    return show;
}
