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

#include <string>

#include "medialibrary/IShowEpisode.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class ShowEpisode : public IShowEpisode, public DatabaseHelpers<ShowEpisode>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t ShowEpisode::*const PrimaryKey;
    };
    enum class Indexes : uint8_t
    {
        MediaIdShowId,
    };

    ShowEpisode( MediaLibraryPtr ml, sqlite::Row& row );
    ShowEpisode( MediaLibraryPtr ml, int64_t mediaId, uint32_t seasonId,
                 uint32_t episodeId, std::string title, int64_t showId );

    virtual int64_t id() const override;
    virtual unsigned int episodeId() const override;
    unsigned int seasonId() const override;
    virtual const std::string& shortSummary() const override;
    bool setShortSummary( const std::string& summary );
    virtual const std::string& tvdbId() const override;
    bool setTvdbId( const std::string& tvdbId );
    virtual ShowPtr show() override;
    virtual const std::string& title() const override;

    static void createTable( sqlite::Connection* dbConnection );
    static void createIndexes( sqlite::Connection* dbConnection );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static std::string index( Indexes index, uint32_t dbModel );
    static std::string indexName( Indexes index, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static std::shared_ptr<ShowEpisode> create(MediaLibraryPtr ml, int64_t mediaId,
                                                uint32_t seasonId,
                                                uint32_t episodeId, std::string title,
                                                int64_t showId );
    static ShowEpisodePtr fromMedia( MediaLibraryPtr ml, int64_t mediaId );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    const int64_t m_mediaId;
    const unsigned int m_episodeId;
    const unsigned int m_seasonId;
    std::string m_title;
    std::string m_shortSummary;
    std::string m_tvdbId;
    const int64_t m_showId;
    ShowPtr m_show;

    friend struct ShowEpisode::Table;
};

}
