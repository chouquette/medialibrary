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

#ifndef MOVIE_H
#define MOVIE_H

#include "IMovie.h"
#include <sqlite3.h>
#include "database/Cache.h"

class Movie;

namespace policy
{
struct MovieTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Movie::*const PrimaryKey;
};
}

class Movie : public IMovie, public Cache<Movie, IMovie, policy::MovieTable>
{
    public:
        Movie( DBConnection dbConnection, sqlite3_stmt* stmt );
        Movie( const std::string& title );

        virtual unsigned int id() const override;
        virtual const std::string& title() const override;
        virtual time_t releaseDate() const override;
        bool setReleaseDate(time_t date);
        virtual const std::string& shortSummary() const override;
        bool setShortSummary(const std::string& summary);
        virtual const std::string& artworkUrl() const override;
        bool setArtworkUrl(const std::string& artworkUrl);
        virtual const std::string& imdbId() const override;
        bool setImdbId(const std::string& imdbId);
        virtual std::vector<MediaPtr> files() override;

        bool destroy();

        static bool createTable( DBConnection dbConnection );
        static std::shared_ptr<Movie> create( DBConnection dbConnection, const std::string& title );

    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_title;
        time_t m_releaseDate;
        std::string m_summary;
        std::string m_artworkUrl;
        std::string m_imdbId;

        typedef Cache<Movie, IMovie, policy::MovieTable> _Cache;
        friend struct policy::MovieTable;
};

#endif // MOVIE_H
