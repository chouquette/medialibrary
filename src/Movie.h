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

#include "medialibrary/IMovie.h"
#include <sqlite3.h>
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class Movie;

namespace policy
{
struct MovieTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t Movie::*const PrimaryKey;
};
}

class Movie : public IMovie, public DatabaseHelpers<Movie, policy::MovieTable>
{
    public:
        Movie( MediaLibraryPtr ml, sqlite::Row& row );
        Movie( MediaLibraryPtr ml, int64_t mediaId, const std::string& title );

        virtual int64_t id() const override;
        virtual const std::string& title() const override;
        virtual const std::string& shortSummary() const override;
        bool setShortSummary(const std::string& summary);
        virtual const std::string& artworkMrl() const override;
        bool setArtworkMrl(const std::string& artworkMrl);
        virtual const std::string& imdbId() const override;
        bool setImdbId(const std::string& imdbId);
        virtual Query<IMedia> files() override;

        static void createTable( sqlite::Connection* dbConnection );
        static std::shared_ptr<Movie> create( MediaLibraryPtr ml, int64_t mediaId, const std::string& title );
        static MoviePtr fromMedia( MediaLibraryPtr ml, int64_t mediaId );

    private:
        MediaLibraryPtr m_ml;
        int64_t m_id;
        int64_t m_mediaId;
        std::string m_title;
        std::string m_summary;
        std::string m_artworkMrl;
        std::string m_imdbId;

        friend struct policy::MovieTable;
};

}

#endif // MOVIE_H
