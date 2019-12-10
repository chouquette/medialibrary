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

#include "medialibrary/IQuery.h"
#include "medialibrary/IMedia.h"

namespace medialibrary
{

struct QueryParameters;

class IMediaGroup
{
public:
    virtual ~IMediaGroup() = default;

    /**
     * @brief id Returns this group id
     */
    virtual int64_t id() const = 0;
    /**
     * @brief name Returns this group name
     */
    virtual const std::string& name() const = 0;
    /**
     * @brief nbMedia Returns the number of media in this group
     */
    virtual uint32_t nbMedia() const = 0;
    /**
     * @brief nbVideo returns the number of video media in this group
     */
    virtual uint32_t nbVideo() const = 0;
    /**
     * @brief nbAudio Returns the number of audio media in this group
     */
    virtual uint32_t nbAudio() const = 0;
    /**
     * @brief nbUnknown Returns the number of media of unknown type in this group
     */
    virtual uint32_t nbUnknown() const = 0;
    /**
     * @brief add Adds a media to this group.
     * @param media A reference to the media to add
     * @return true if the media was successfully added to the group, false otherwise
     *
     * The media will be automatically removed its previous group if it belonged
     * to one
     */
    virtual bool add( IMedia& media ) = 0;
    /**
     * @brief add Adds a media to this group
     * @param mediaId The media to add's ID
     * @return true if the media was successfully added to the group, false otherwise
     *
     * The media will be automatically removed its previous group if it belonged
     * to one
     */
    virtual bool add( int64_t mediaId ) = 0;
    /**
     * @brief add Removes a media from this group.
     * @param media A reference to the media to remove
     * @return true if the media was successfully removed from the group, false otherwise
     */
    virtual bool remove( IMedia& media ) = 0;
    /**
     * @brief add Removes a media from this group
     * @param mediaId The media to remove's ID
     * @return true if the media was successfully removed from the group, false otherwise
     */
    virtual bool remove( int64_t mediaId ) = 0;

    /**
     * @brief media List the media that belong to this group
     * @param mediaType The type of media to return, or Unknown to return them all
     * @param params Some query parameters
     * @return A query object representing the results
     *
     * \see{IMediaLibrary::audioFile} for the supported sorting criteria
     */
    virtual Query<IMedia> media( IMedia::Type mediaType,
                                 const QueryParameters* params = nullptr ) = 0;
    /**
     * @brief searchMedia Search amongst the media belonging to this group
     * @brief pattern The search pattern (3 characters minimum)
     * @param mediaType The type of media to return, or Unknown to return them all
     * @param params Some query parameters
     * @return A query object representing the results
     *
     * \see{IMediaLibrary::audioFile} for the supported sorting criteria
     */
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       IMedia::Type mediaType,
                                       const QueryParameters* params = nullptr ) = 0;

    /**
     * @brief createSubgroup Create a subgroup
     * @param name This subgroup name
     * @return The newly created group, or nullptr in case of error
     *
     * Names are required to be unique for a given parent group.
     */
    virtual std::shared_ptr<IMediaGroup> createSubgroup( const std::string& name ) = 0;
    /**
     * @brief subgroups Returns the subgroups for this group
     *
     * Supported sorting criteria are:
     * - Alpha (default)
     * - NbVideo
     * - NbAudio
     * - NbMedia
     */
    virtual Query<IMediaGroup> subgroups( const QueryParameters* params = nullptr ) const = 0;
    virtual bool isSubgroup() const = 0;
    /**
     * @brief parent Returns this group's parent group, or nullptr if there are none
     */
    virtual std::shared_ptr<IMediaGroup> parent() const = 0;
    /**
     * @brief path Returns a virtual path to this group
     *
     * This consists of <parent group 1>/<parent group 2>/.../<group>
     */
    virtual std::string path() const = 0;
};

}
