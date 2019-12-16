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

#include "Movie.h"
#include "Media.h"
#include "database/SqliteTools.h"

namespace medialibrary
{

const std::string Movie::Table::Name = "Movie";
const std::string Movie::Table::PrimaryKeyColumn = "id_movie";
int64_t Movie::* const Movie::Table::PrimaryKey = &Movie::m_id;

Movie::Movie(MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_mediaId( row.extract<decltype(m_mediaId)>() )
    , m_summary( row.extract<decltype(m_summary)>() )
    , m_imdbId( row.extract<decltype(m_imdbId)>() )
{
    assert( row.hasRemainingColumns() == false );
}

Movie::Movie( MediaLibraryPtr ml, int64_t mediaId )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
{
}

int64_t Movie::id() const
{
    return m_id;
}

const std::string& Movie::shortSummary() const
{
    return m_summary;
}

bool Movie::setShortSummary( const std::string& summary )
{
    static const std::string req = "UPDATE " + Movie::Table::Name
            + " SET summary = ? WHERE id_movie = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, summary, m_id ) == false )
        return false;
    m_summary = summary;
    return true;
}

const std::string& Movie::imdbId() const
{
    return m_imdbId;
}

bool Movie::setImdbId( const std::string& imdbId )
{
    static const std::string req = "UPDATE " + Movie::Table::Name
            + " SET imdb_id = ? WHERE id_movie = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, imdbId, m_id ) == false )
        return false;
    m_imdbId = imdbId;
    return true;
}

void Movie::createTable( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
}

void Movie::createIndexes(sqlite::Connection* dbConnection)
{
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::MediaId, Settings::DbModelVersion ) );
}

std::string Movie::schema( const std::string& tableName, uint32_t )
{
    assert( tableName == Table::Name );
    return "CREATE TABLE " + Table::Name +
    "("
        "id_movie INTEGER PRIMARY KEY AUTOINCREMENT,"
        "media_id UNSIGNED INTEGER NOT NULL,"
        "summary TEXT,"
        "imdb_id TEXT,"
        "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name
            + "(id_media) ON DELETE CASCADE"
    ")";
}

std::string Movie::index( Indexes index, uint32_t )
{
    assert( index == Indexes::MediaId );
    return "CREATE INDEX movie_media_idx ON "
                + Table::Name + "(media_id)";
}

bool Movie::checkDbModel(MediaLibraryPtr ml)
{
    return sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name );
}

std::shared_ptr<Movie> Movie::create(MediaLibraryPtr ml, int64_t mediaId )
{
    auto movie = std::make_shared<Movie>( ml, mediaId );
    static const std::string req = "INSERT INTO " + Movie::Table::Name
            + "(media_id) VALUES(?)";
    if ( insert( ml, movie, req, mediaId ) == false )
        return nullptr;
    return movie;
}

MoviePtr Movie::fromMedia( MediaLibraryPtr ml, int64_t mediaId )
{
    static const std::string req = "SELECT * FROM " + Movie::Table::Name + " WHERE media_id = ?";
    return fetch( ml, req, mediaId );
}

}
