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

#ifndef SHOWEPISODE_H
#define SHOWEPISODE_H

class Show;
class ShowEpisode;

#include <string>
#include <sqlite3.h>

#include "IMediaLibrary.h"
#include "IShowEpisode.h"
#include "database/Cache.h"

namespace policy
{
struct ShowEpisodeTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int ShowEpisode::*const PrimaryKey;
};
}

class ShowEpisode : public IShowEpisode, public Cache<ShowEpisode, IShowEpisode, policy::ShowEpisodeTable>
{
    public:
        ShowEpisode( DBConnection dbConnection, sqlite3_stmt* stmt );
        ShowEpisode(const std::string& name, unsigned int episodeNumber, unsigned int showId );

        virtual unsigned int id() const override;
        virtual const std::string& artworkUrl() const override;
        bool setArtworkUrl( const std::string& artworkUrl );
        virtual unsigned int episodeNumber() const override;
        virtual time_t lastSyncDate() const override;
        virtual const std::string& name() const override;
        unsigned int seasonNumber() const;
        bool setSeasonNumber(unsigned int seasonNumber);
        virtual const std::string& shortSummary() const override;
        bool setShortSummary( const std::string& summary );
        virtual const std::string& tvdbId() const override;
        bool setTvdbId( const std::string& tvdbId );
        virtual std::shared_ptr<IShow> show() override;
        virtual std::vector<MediaPtr> files() override;

        static bool createTable( DBConnection dbConnection );
        static std::shared_ptr<ShowEpisode> create( DBConnection dbConnection, const std::string& title, unsigned int episodeNumber, unsigned int showId );

    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_artworkUrl;
        unsigned int m_episodeNumber;
        time_t m_lastSyncDate;
        std::string m_name;
        unsigned int m_seasonNumber;
        std::string m_shortSummary;
        std::string m_tvdbId;
        unsigned int m_showId;
        ShowPtr m_show;

        friend class Cache<ShowEpisode, IShowEpisode, policy::ShowEpisodeTable>;
        friend struct policy::ShowEpisodeTable;
        typedef Cache<ShowEpisode, IShowEpisode, policy::ShowEpisodeTable> _Cache;
};

#endif // SHOWEPISODE_H
