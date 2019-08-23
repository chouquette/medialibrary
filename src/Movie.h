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

#ifndef MOVIE_H
#define MOVIE_H

#include "medialibrary/IMovie.h"
#include <sqlite3.h>
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class Movie;

class Movie : public IMovie, public DatabaseHelpers<Movie>
{
    public:
        struct Table
        {
            static const std::string Name;
            static const std::string PrimaryKeyColumn;
            static int64_t Movie::*const PrimaryKey;
        };
        Movie( MediaLibraryPtr ml, sqlite::Row& row );
        Movie( MediaLibraryPtr ml, int64_t mediaId );

        virtual int64_t id() const override;
        virtual const std::string& shortSummary() const override;
        bool setShortSummary(const std::string& summary);
        virtual const std::string& imdbId() const override;
        bool setImdbId(const std::string& imdbId);

        static void createTable( sqlite::Connection* dbConnection );
        static std::string schema( const std::string& tableName, uint32_t dbModel );
        static std::shared_ptr<Movie> create( MediaLibraryPtr ml, int64_t mediaId );
        static MoviePtr fromMedia( MediaLibraryPtr ml, int64_t mediaId );

    private:
        MediaLibraryPtr m_ml;
        int64_t m_id;
        const int64_t m_mediaId;
        std::string m_summary;
        std::string m_imdbId;

        friend struct Movie::Table;
};

}

#endif // MOVIE_H
