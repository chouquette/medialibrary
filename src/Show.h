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

#ifndef SHOW_H
#define SHOW_H

#include <sqlite3.h>

#include "database/DatabaseHelpers.h"
#include "medialibrary/IMediaLibrary.h"
#include "medialibrary/IShow.h"

namespace medialibrary
{

class Media;
class Show;
class ShowEpisode;

namespace policy
{
struct ShowTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t Show::*const PrimaryKey;
};
}

class Show : public IShow, public DatabaseHelpers<Show, policy::ShowTable>
{
    public:
        Show( MediaLibraryPtr ml, sqlite::Row& row );
        Show( MediaLibraryPtr ml, const std::string& name );

        virtual int64_t id() const override;
        virtual const std::string& name() const override;
        virtual time_t releaseDate() const override;
        bool setReleaseDate( time_t date );
        virtual const std::string& shortSummary() const override;
        bool setShortSummary( const std::string& summary );
        virtual const std::string& artworkMrl() const override;
        bool setArtworkMrl( const std::string& artworkMrl );
        virtual const std::string& tvdbId() const override;
        bool setTvdbId( const std::string& summary );
        std::shared_ptr<ShowEpisode> addEpisode( Media& media, unsigned int episodeNumber );
        virtual Query<IMedia> episodes( const QueryParameters* params ) const override;
        virtual uint32_t nbSeasons() const override;
        virtual uint32_t nbEpisodes() const override;

        static void createTable( sqlite::Connection* dbConnection );
        static std::shared_ptr<Show> create( MediaLibraryPtr ml, const std::string& name );

        static Query<IShow> listAll( MediaLibraryPtr ml, const QueryParameters* params );

    protected:
        MediaLibraryPtr m_ml;

        int64_t m_id;
        std::string m_name;
        time_t m_releaseDate;
        std::string m_shortSummary;
        std::string m_artworkMrl;
        std::string m_tvdbId;

        friend struct policy::ShowTable;
};

}

#endif // SHOW_H
