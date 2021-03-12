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

class IArtist
{
public:
    virtual ~IArtist() = default;
    virtual int64_t id() const = 0;
    virtual const std::string& name() const = 0;
    virtual const std::string& shortBio() const = 0;
    /**
     * @brief albums List the albums this artist appears on.
     *
     * This will return all albums by this artist, and all album the artist
     * appeared on, even if they are not the main artist (or AlbumArtist)
     */
    virtual Query<IAlbum> albums( const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IAlbum> searchAlbums( const std::string& pattern,
                                        const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IMedia> tracks( const QueryParameters* params = nullptr ) const = 0;
    virtual Query<IMedia> searchTracks( const std::string& pattern,
                                        const QueryParameters* params = nullptr ) const = 0;
    /**
     * @brief thumbnailStatus returns this artist thumbnail status
     *
     * @param sizeType The targeted thumbnail size
     * @see{medialibrary::ThumbnailStatus}
     */
    virtual ThumbnailStatus thumbnailStatus( ThumbnailSizeType sizeType ) const = 0;
    virtual const std::string& thumbnailMrl( ThumbnailSizeType sizeType ) const = 0;
    /**
     * @brief setThumbnail Assign a thumbnail to the artist
     * @param thumbnailMrl An mrl pointing to the thumbnail
     * @return true in case of success, false otherwise
     *
     * @note The medialibrary does not take ownership of the thumbnail. It is
     * application responsibility to ensure that it will always be available
     * or that a later call will invalidate the thumbnail if it gets (re)moved
     */
    virtual bool setThumbnail( const std::string& thumbnailMrl,
                               ThumbnailSizeType sizeType ) = 0;
    virtual const std::string& musicBrainzId() const = 0;
    /**
     * @brief nbAlbums
     * @return The number of albums *by* this artist. This doesn't include the
     *         albums an artist appears on.
     */
    virtual unsigned int nbAlbums() const = 0;
    virtual unsigned int nbTracks() const = 0;
    virtual unsigned int nbPresentTracks() const = 0;
};

}
