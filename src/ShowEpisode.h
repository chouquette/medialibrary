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

#include "medialibrary/IMediaLibrary.h"
#include "medialibrary/IShowEpisode.h"
#include "database/DatabaseHelpers.h"

namespace policy
{
struct ShowEpisodeTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t ShowEpisode::*const PrimaryKey;
};
}

class ShowEpisode : public IShowEpisode, public DatabaseHelpers<ShowEpisode, policy::ShowEpisodeTable>
{
    public:
        ShowEpisode( MediaLibraryPtr ml, sqlite::Row& row );
        ShowEpisode( MediaLibraryPtr ml, int64_t mediaId, const std::string& name, unsigned int episodeNumber, int64_t showId );

        virtual int64_t id() const override;
        virtual const std::string& artworkMrl() const override;
        bool setArtworkMrl( const std::string& artworkMrl );
        virtual unsigned int episodeNumber() const override;
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
        static std::shared_ptr<ShowEpisode> create( MediaLibraryPtr ml, int64_t mediaId, const std::string& title, unsigned int episodeNumber, int64_t showId );
        static ShowEpisodePtr fromMedia( MediaLibraryPtr ml, int64_t mediaId );

    private:
        MediaLibraryPtr m_ml;
        int64_t m_id;
        int64_t m_mediaId;
        std::string m_artworkMrl;
        unsigned int m_episodeNumber;
        std::string m_name;
        unsigned int m_seasonNumber;
        std::string m_shortSummary;
        std::string m_tvdbId;
        int64_t m_showId;
        ShowPtr m_show;

        friend struct policy::ShowEpisodeTable;
};

#endif // SHOWEPISODE_H
