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

#include "IMediaLibrary.h"

class IAlbum
{
public:
    virtual ~IAlbum() = default;
    virtual unsigned int id() const = 0;
    virtual const std::string& title() const = 0;
    /**
     * @brief releaseYear returns the release year, or 0 if unknown.
     * The release date of an album is considered unknown if multiple tracks
     * of the same album have different release dates
     */
    virtual unsigned int releaseYear() const = 0;
    virtual const std::string& shortSummary() const = 0;
    virtual const std::string& artworkMrl() const = 0;
    /**
     * @brief tracks fetches album tracks from the database
     */
    virtual std::vector<std::shared_ptr<IMedia>> tracks() const = 0;
    /**
     * @brief albumArtist Returns the album main artist (generally tagged as album-artist)
     */
    virtual ArtistPtr albumArtist() const = 0;
    /**
     * @brief artists Returns a vector of all additional artists appearing on the album.
     */
    virtual std::vector<ArtistPtr> artists() const = 0;
    /**
     * @brief nbTracks Returns the amount of track in this album.
     * The value is cached, and doesn't require fetching anything.
     */
    virtual uint32_t nbTracks() const = 0;
};
