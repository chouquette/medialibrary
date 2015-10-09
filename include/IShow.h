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

#ifndef ISHOW_H
#define ISHOW_H

#include "IMediaLibrary.h"

class IShow
{
    public:
        virtual ~IShow() {}
        virtual unsigned int id() const = 0;
        virtual const std::string& name() const = 0;
        virtual time_t releaseDate() const = 0;
        virtual bool setReleaseDate( time_t date ) = 0;
        virtual const std::string& shortSummary() const = 0;
        virtual bool setShortSummary( const std::string& summary ) = 0;
        virtual const std::string& artworkUrl() const = 0;
        virtual bool setArtworkUrl( const std::string& artworkUrl ) = 0;
        virtual const std::string& tvdbId() = 0;
        virtual bool setTvdbId( const std::string& id ) = 0;
        virtual ShowEpisodePtr addEpisode( const std::string& title, unsigned int episodeNumber ) = 0;
        virtual std::vector<ShowEpisodePtr> episodes() = 0;
        virtual bool destroy() = 0;
};

#endif // ISHOW_H
