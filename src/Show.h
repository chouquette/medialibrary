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

#pragma once

#include "database/DatabaseHelpers.h"
#include "medialibrary/IShow.h"

namespace medialibrary
{

class Media;
class ShowEpisode;

class Show : public IShow, public DatabaseHelpers<Show>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Show::*const PrimaryKey;
    };
    struct FtsTable
    {
        static const std::string Name;
    };

    Show( MediaLibraryPtr ml, sqlite::Row& row );
    Show( MediaLibraryPtr ml, const std::string& title );

    virtual int64_t id() const override;
    virtual const std::string& title() const override;
    virtual time_t releaseDate() const override;
    bool setReleaseDate( time_t date );
    virtual const std::string& shortSummary() const override;
    bool setShortSummary( const std::string& summary );
    virtual const std::string& artworkMrl() const override;
    bool setArtworkMrl( const std::string& artworkMrl );
    virtual const std::string& tvdbId() const override;
    bool setTvdbId( const std::string& summary );
    std::shared_ptr<ShowEpisode> addEpisode( Media& media, uint32_t seasonId,
                                             uint32_t episodeId );
    virtual Query<IMedia> episodes( const QueryParameters* params ) const override;
    virtual Query<IMedia> searchEpisodes( const std::string& pattern,
                                          const QueryParameters* params = nullptr ) const override;
    virtual uint32_t nbSeasons() const override;
    virtual uint32_t nbEpisodes() const override;

    static void createTable( sqlite::Connection* dbConnection );
    static void createTriggers(sqlite::Connection* dbConnection , uint32_t dbModelVersion);
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static std::shared_ptr<Show> create( MediaLibraryPtr ml, const std::string& title );

    static Query<IShow> listAll( MediaLibraryPtr ml, const QueryParameters* params );
    static Query<IShow> search( MediaLibraryPtr ml, const std::string& pattern,
                                const QueryParameters* params );
    static bool createUnknownShow( sqlite::Connection* dbConn );

private:
    static std::string orderBy( const QueryParameters* params );

protected:
    MediaLibraryPtr m_ml;

    int64_t m_id;
    const std::string m_title;
    uint32_t m_nbEpisodes;
    time_t m_releaseDate;
    std::string m_shortSummary;
    std::string m_artworkMrl;
    std::string m_tvdbId;

    friend struct Show::Table;
};

}
