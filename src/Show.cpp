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

#include "Show.h"

#include "Media.h"
#include "ShowEpisode.h"
#include "MediaLibrary.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"

namespace medialibrary
{

const std::string Show::Table::Name = "Show";
const std::string Show::Table::PrimaryKeyColumn = "id_show";
int64_t Show::* const Show::Table::PrimaryKey = &Show::m_id;
const std::string Show::FtsTable::Name = "ShowFts";

Show::Show( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_title( row.extract<decltype(m_title)>() )
    , m_nbEpisodes( row.extract<decltype(m_nbEpisodes)>() )
    , m_releaseDate( row.extract<decltype(m_releaseDate)>() )
    , m_shortSummary( row.extract<decltype(m_shortSummary)>() )
    , m_artworkMrl( row.extract<decltype(m_artworkMrl)>() )
    , m_tvdbId( row.extract<decltype(m_tvdbId)>() )
    // Don't load is_present
{
    assert( row.nbColumns() == 8 );
}

Show::Show( MediaLibraryPtr ml, const std::string& name )
    : m_ml( ml )
    , m_id( 0 )
    , m_title( name )
    , m_nbEpisodes( 0 )
    , m_releaseDate( 0 )
{
}

int64_t Show::id() const
{
    return m_id;
}

const std::string& Show::title() const
{
    return m_title;
}

time_t Show::releaseDate() const
{
    return m_releaseDate;
}

bool Show::setReleaseDate( time_t date )
{
    static const std::string req = "UPDATE " + Show::Table::Name
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
    static const std::string req = "UPDATE " + Show::Table::Name
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
    static const std::string req = "UPDATE " + Show::Table::Name
            + " SET artwork_mrl = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, artworkMrl, m_id ) == false )
        return false;
    m_artworkMrl = artworkMrl;
    return true;
}

const std::string& Show::tvdbId() const
{
    return m_tvdbId;
}

bool Show::setTvdbId( const std::string& tvdbId )
{
    static const std::string req = "UPDATE " + Show::Table::Name
            + " SET tvdb_id = ? WHERE id_show = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, tvdbId, m_id ) == false )
        return false;
    m_tvdbId = tvdbId;
    return true;
}

std::shared_ptr<ShowEpisode> Show::addEpisode( Media& media, uint32_t seasonId,
                                               uint32_t episodeId )
{
    auto episode = ShowEpisode::create( m_ml, media.id(), seasonId, episodeId, m_id );
    media.setShowEpisode( episode );
    media.save();
    m_nbEpisodes++;
    return episode;
}

Query<IMedia> Show::episodes( const QueryParameters* params ) const
{
    std::string req = "FROM " + Media::Table::Name + " med "
            " INNER JOIN " + ShowEpisode::Table::Name + " ep ON ep.media_id = med.id_media "
            + " WHERE ep.show_id = ?"
            " AND med.is_present != 0";
    std::string orderBy = " ORDER BY ";
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    switch ( sort )
    {
        case SortingCriteria::Alpha:
            orderBy += "med.name";
            if ( desc == true )
                orderBy += " DESC";
            break;
        default:
            LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default" );
            /* fall-through */
        case SortingCriteria::Default:
            if ( desc == true )
                orderBy += "ep.season_number DESC, ep.episode_number DESC";
            else
                orderBy += "ep.season_number, ep.episode_number";
            break;

    }
    return make_query<Media, IMedia>( m_ml, "med.*", std::move( req ),
                                      std::move( orderBy), m_id );
}

Query<IMedia> Show::searchEpisodes( const std::string& pattern,
                                    const QueryParameters* params ) const
{
    return Media::searchShowEpisodes( m_ml, pattern, m_id, params );
}

uint32_t Show::nbSeasons() const
{
    return 0;
}

uint32_t Show::nbEpisodes() const
{
    return m_nbEpisodes;
}

void Show::createTable( sqlite::Connection* dbConnection )
{
    const std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
        schema( FtsTable::Name, Settings::DbModelVersion )
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConnection, req );
}

void Show::createTriggers( sqlite::Connection* dbConnection, uint32_t dbModelVersion )
{
    const std::string insertTrigger = "CREATE TRIGGER IF NOT EXISTS insert_show_fts"
            " AFTER INSERT ON " + Show::Table::Name +
            " BEGIN"
            " INSERT INTO " + Show::FtsTable::Name + "(rowid,title) VALUES(new.id_show, new.title);"
            " END";
    const std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS delete_show_fts"
            " BEFORE DELETE ON " + Show::Table::Name +
            " BEGIN"
            " DELETE FROM " + Show::FtsTable::Name + " WHERE rowid = old.id_show;"
            " END";
    sqlite::Tools::executeRequest( dbConnection, insertTrigger );
    sqlite::Tools::executeRequest( dbConnection, deleteTrigger );

    if ( dbModelVersion < 23 )
        return;

    const std::string incrementNbEpisodeTrigger = "CREATE TRIGGER IF NOT EXISTS"
            " show_increment_nb_episode AFTER INSERT ON " + ShowEpisode::Table::Name +
            " BEGIN"
            " UPDATE " + Table::Name +
                " SET nb_episodes = nb_episodes + 1, is_present = is_present + 1"
                " WHERE id_show = new.show_id;"
            " END";
    const std::string decrementNbEpisodeTrigger = "CREATE TRIGGER IF NOT EXISTS"
            " show_decrement_nb_episode AFTER DELETE ON " + ShowEpisode::Table::Name +
            " BEGIN"
            " UPDATE " + Table::Name +
                " SET nb_episodes = nb_episodes - 1, is_present = is_present - 1"
                " WHERE id_show = old.show_id;"
            " END";
    const std::string updateIsPresentTrigger = "CREATE TRIGGER IF NOT EXISTS"
            " show_update_is_present AFTER UPDATE OF "
                "is_present ON " + Media::Table::Name +
            " WHEN new.subtype = " +
                std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                                    IMedia::SubType::ShowEpisode ) ) +
            " AND new.is_present != old.is_present"
            " BEGIN "
            " UPDATE " + Table::Name + " SET is_present=is_present +"
                " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)"
                " WHERE id_show = (SELECT show_id FROM " + ShowEpisode::Table::Name +
                    " WHERE media_id = new.id_media);"
            " END";
    sqlite::Tools::executeRequest( dbConnection, incrementNbEpisodeTrigger );
    sqlite::Tools::executeRequest( dbConnection, decrementNbEpisodeTrigger );
    sqlite::Tools::executeRequest( dbConnection, updateIsPresentTrigger );
}

std::string Show::schema( const std::string& tableName, uint32_t dbModelVersion )
{
    if ( tableName == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE " + FtsTable::Name +
               " USING FTS3(title)";
    }
    assert( tableName == Table::Name );
    if ( dbModelVersion < 23 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
           "id_show INTEGER PRIMARY KEY AUTOINCREMENT,"
           "title TEXT,"
           "release_date UNSIGNED INTEGER,"
           "short_summary TEXT,"
           "artwork_mrl TEXT,"
           "tvdb_id TEXT"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "("
       "id_show INTEGER PRIMARY KEY AUTOINCREMENT,"
       "title TEXT,"
       "nb_episodes UNSIGNED INTEGER NOT NULL DEFAULT 0,"
       "release_date UNSIGNED INTEGER,"
       "short_summary TEXT,"
       "artwork_mrl TEXT,"
       "tvdb_id TEXT,"
       "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 "
            "CHECK(is_present <= nb_episodes)"
    ")";
}

bool Show::checkDbModel(MediaLibraryPtr ml)
{
    return sqlite::Tools::checkSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) &&
           sqlite::Tools::checkSchema( ml->getConn(),
                                       schema( FtsTable::Name, Settings::DbModelVersion ),
                                       FtsTable::Name );
}

std::shared_ptr<Show> Show::create( MediaLibraryPtr ml, const std::string& name )
{
    auto show = std::make_shared<Show>( ml, name );
    static const std::string req = "INSERT INTO " + Show::Table::Name
            + "(title) VALUES(?)";
    if ( insert( ml, show, req, name ) == false )
        return nullptr;
    return show;
}

Query<IShow> Show::listAll( MediaLibraryPtr ml, const QueryParameters* params )
{
    std::string req = "FROM " + Show::Table::Name + " WHERE is_present != 0";
    return make_query<Show, IShow>( ml, "*", std::move( req ), orderBy( params ) );
}

std::string Show::orderBy( const QueryParameters* params )
{
    std::string req = " ORDER BY ";
    SortingCriteria sort = params != nullptr ? params->sort : SortingCriteria::Default;
    switch ( sort )
    {
        case SortingCriteria::ReleaseDate:
            req += "release_date";
            break;
        case SortingCriteria::Default:
        case SortingCriteria::Alpha:
        default:
            req += "title";
            break;

    }
    if ( params != nullptr && params->desc == true )
        req += " DESC";
    return req;
}

Query<IShow> Show::search( MediaLibraryPtr ml, const std::string& pattern,
                           const QueryParameters* params )
{
    std::string req = "FROM " + Show::Table::Name + " WHERE id_show IN"
            "(SELECT rowid FROM " + Show::FtsTable::Name + " WHERE " +
            Show::FtsTable::Name + " MATCH ?) AND is_present != 0";
    return make_query<Show, IShow>( ml, "*", std::move( req ),
                                    orderBy( params ),
                                    sqlite::Tools::sanitizePattern( pattern ) );
}

bool Show::createUnknownShow( sqlite::Connection* dbConn )
{
    const std::string req = "INSERT INTO " + Table::Name + " (id_show) VALUES(?)";
    return sqlite::Tools::executeInsert( dbConn, req, UnknownShowID );
}

}
