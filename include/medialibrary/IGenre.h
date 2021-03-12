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

#include "IMediaLibrary.h"

namespace medialibrary
{

class IGenre
{
public:
    enum class TracksIncluded : uint8_t
    {
        /// Include all present tracks in the listing
        All,
        /// Only include tracks with a thumbnail
        WithThumbnailOnly,
    };

    virtual ~IGenre() = default;
    virtual int64_t id() const = 0;
    virtual const std::string& name() const = 0;
    virtual uint32_t nbTracks() const = 0;
    virtual uint32_t nbPresentTracks() const = 0;
    virtual Query<IArtist> artists( const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IArtist> searchArtists( const std::string& pattern,
                                         const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief tracks Returns the tracks associated with this genre
     * @param included A TracksIncluded flag to specify which tracks to return
     * @param params Some query parameters, or nullptr for the default.
     *
     * This function supports sorting by:
     * - Duration
     * - InsertionDate
     * - ReleaseDate
     * - Alpha
     *
     * The default sort is to group tracks by their artist, album, disc number,
     * track number, and finally file name in case of ambiguous results.
     * Sort is ascending by default.
     */
    virtual Query<IMedia> tracks( TracksIncluded included,
                                  const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IMedia> searchTracks( const std::string& pattern,
                                  const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IAlbum> albums( const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IAlbum> searchAlbums( const std::string& pattern,
                                        const QueryParameters* params = nullptr ) const = 0;

    /**
     * @brief thumbnailMrl Returns this genre thumbnail mrl
     * @param sizeType The target thumbnail size type
     * @return An mrl to the thumbnail if any, or an empty string
     */
    virtual const std::string& thumbnailMrl( ThumbnailSizeType sizeType ) const = 0;
    /**
     * @brief hasThumbnail Returns true if this genre has a thumbnail available
     * @param sizeType The probed thumbnail size type
     * @return true if a thumbnail is available, false otherwise
     */
    virtual bool hasThumbnail( ThumbnailSizeType sizeType ) const = 0;
    /**
     * @brief setThumbnail Set a thumbnail for this genre
     * @param mrl The thumbnail mrl
     * @param sizeType The thumbnail size type
     * @param takeOwnership If true, the medialibrary will copy the thumbnail in
     *                      its thumbnail directory and will manage its lifetime
     * @return true if the thumbnail was successfully overriden, false otherwise.
     */
    virtual bool setThumbnail( const std::string& mrl, ThumbnailSizeType sizeType,
                               bool takeOwnership ) = 0;
};

}
