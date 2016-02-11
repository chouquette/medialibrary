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

#include "Movie.h"
#include "Media.h"
#include "database/SqliteTools.h"

const std::string policy::MovieTable::Name = "Movie";
const std::string policy::MovieTable::PrimaryKeyColumn = "id_movie";
unsigned int Movie::* const policy::MovieTable::PrimaryKey = &Movie::m_id;

Movie::Movie(MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    row >> m_id
        >> m_mediaId
        >> m_title
        >> m_summary
        >> m_artworkMrl
        >> m_imdbId;
}

Movie::Movie( MediaLibraryPtr ml, unsigned int mediaId, const std::string& title )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_title( title )
{
}

unsigned int Movie::id() const
{
    return m_id;
}

const std::string&Movie::title() const
{
    return m_title;
}

const std::string& Movie::shortSummary() const
{
    return m_summary;
}

bool Movie::setShortSummary( const std::string& summary )
{
    static const std::string req = "UPDATE " + policy::MovieTable::Name
            + " SET summary = ? WHERE id_movie = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, summary, m_id ) == false )
        return false;
    m_summary = summary;
    return true;
}

const std::string&Movie::artworkMrl() const
{
    return m_artworkMrl;
}

bool Movie::setArtworkMrl( const std::string& artworkMrl )
{
    static const std::string req = "UPDATE " + policy::MovieTable::Name
            + " SET artwork_mrl = ? WHERE id_movie = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, artworkMrl, m_id ) == false )
        return false;
    m_artworkMrl = artworkMrl;
    return true;
}

const std::string& Movie::imdbId() const
{
    return m_imdbId;
}

bool Movie::setImdbId( const std::string& imdbId )
{
    static const std::string req = "UPDATE " + policy::MovieTable::Name
            + " SET imdb_id = ? WHERE id_movie = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, imdbId, m_id ) == false )
        return false;
    m_imdbId = imdbId;
    return true;
}

std::vector<MediaPtr> Movie::files()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name
            + " WHERE movie_id = ?";
    return Media::fetchAll<IMedia>( m_ml, req, m_id );
}

bool Movie::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::MovieTable::Name
            + "("
                "id_movie INTEGER PRIMARY KEY AUTOINCREMENT,"
                "media_id UNSIGNED INTEGER NOT NULL,"
                "title TEXT UNIQUE ON CONFLICT FAIL,"
                "summary TEXT,"
                "artwork_mrl TEXT,"
                "imdb_id TEXT,"
                "FOREIGN KEY(media_id) REFERENCES " + policy::MediaTable::Name
                + "(id_media) ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

std::shared_ptr<Movie> Movie::create( MediaLibraryPtr ml, unsigned int mediaId, const std::string& title )
{
    auto movie = std::make_shared<Movie>( ml, mediaId, title );
    static const std::string req = "INSERT INTO " + policy::MovieTable::Name
            + "(media_id, title) VALUES(?, ?)";
    if ( insert( ml, movie, req, mediaId, title ) == false )
        return nullptr;
    return movie;
}

MoviePtr Movie::fromMedia( MediaLibraryPtr ml, unsigned int mediaId )
{
    static const std::string req = "SELECT * FROM " + policy::MovieTable::Name + " WHERE media_id = ?";
    return fetch( ml, req, mediaId );
}
