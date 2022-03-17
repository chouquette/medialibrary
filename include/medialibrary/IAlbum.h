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

#include "IMediaLibrary.h"

namespace medialibrary
{

class IAlbum
{
public:
    virtual ~IAlbum() = default;
    virtual int64_t id() const = 0;
    virtual const std::string& title() const = 0;
    /**
     * @brief releaseYear returns the release year, or 0 if unknown.
     */
    virtual unsigned int releaseYear() const = 0;
    virtual const std::string& shortSummary() const = 0;
    /**
     * @brief thumbnailStatus Returns this album current thumbnail status.
     */
    virtual ThumbnailStatus thumbnailStatus( ThumbnailSizeType sizeType ) const = 0;
    virtual const std::string& thumbnailMrl( ThumbnailSizeType sizeType ) const = 0;
    /**
     * @brief tracks fetches album tracks from the database
     */
    virtual Query<IMedia> tracks( const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief tracks fetches album tracks, filtered by genre
     * @param genre A musical genre. Only tracks of this genre will be returned
     * @return the requested tracks
     */
    virtual Query<IMedia> tracks( GenrePtr genre, const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief albumArtist Returns the album main artist (generally tagged as album-artist)
     * This can be an artist that doesn't appear on the album, and is solely dependent
     * on the most present AlbumArtist tag for all of this album's tracks
     */
    virtual ArtistPtr albumArtist() const = 0;
    /**
     * @brief artists Returns a Query object representing all artists appearing
     *                on at least one track for this album.
     * Artists are sorted by name.
     * @param params the query parameters
     */
    virtual Query<IArtist> artists( const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief nbTracks Returns the amount of track in this album.
     * The value is cached, and doesn't require fetching anything.
     */
    virtual uint32_t nbTracks() const = 0;
    /**
     * @brief nbPresentTracks Returns the number of present tracks in this album
     */
    virtual uint32_t nbPresentTracks() const = 0;
    /**
     * @brief nbDiscs Returns the total number of discs for this album.
     * Defaults to 1
     */
    virtual uint32_t nbDiscs() const = 0;
    /**
     * @brief duration Returns the total album duration in milliseconds
     */
    virtual int64_t duration() const = 0;
    /**
     * @brief isUnknownAlbum returns true is this is an unknown album
     */
    virtual bool isUnknownAlbum() const = 0;

    virtual Query<IMedia> searchTracks( const std::string& pattern,
                                        const QueryParameters* params = nullptr ) const = 0;
};

}
