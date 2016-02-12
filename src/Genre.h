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

#pragma once

#include "IGenre.h"

#include "database/DatabaseHelpers.h"

class Genre;

namespace policy
{
struct GenreTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static unsigned int Genre::*const PrimaryKey;
};
}

class Genre : public IGenre, public DatabaseHelpers<Genre, policy::GenreTable>
{
public:
    Genre( MediaLibraryPtr ml, sqlite::Row& row );
    Genre( MediaLibraryPtr ml, const std::string& name );
    virtual unsigned int id() const;
    virtual const std::string& name() const override;
    virtual std::vector<ArtistPtr> artists( medialibrary::SortingCriteria sort, bool desc ) const override;
    virtual std::vector<AlbumTrackPtr> tracks(medialibrary::SortingCriteria sort, bool desc) const override;
    virtual std::vector<AlbumPtr> albums( medialibrary::SortingCriteria sort, bool desc ) const override;

    static bool createTable( DBConnection dbConn );
    static std::shared_ptr<Genre> create( MediaLibraryPtr ml, const std::string& name );
    static std::shared_ptr<Genre> fromName( MediaLibraryPtr ml, const std::string& name );
    static std::vector<GenrePtr> search( MediaLibraryPtr ml, const std::string& name );
    static std::vector<GenrePtr> listAll( MediaLibraryPtr ml, medialibrary::SortingCriteria sort, bool desc );

private:
    MediaLibraryPtr m_ml;

    unsigned int m_id;
    std::string m_name;

    friend policy::GenreTable;
};
