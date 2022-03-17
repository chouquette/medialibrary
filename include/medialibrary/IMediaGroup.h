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
     * @brief nbTotalMedia Returns the number of media in this group, not accounting
     *                     for their presence.
     *
     * Even if all this group's media are missing, this will still return a non
     * 0 count.
     */
    virtual uint32_t nbTotalMedia() const = 0;
    /**
     * @brief nbMedia Returns the number of present media in this group
     */
    virtual uint32_t nbPresentMedia() const = 0;
    /**
     * @brief nbVideo returns the number of present video media in this group
     */
    virtual uint32_t nbPresentVideo() const = 0;
    /**
     * @brief nbAudio Returns the number of present audio media in this group
     */
    virtual uint32_t nbPresentAudio() const = 0;
    /**
     * @brief nbUnknown Returns the number of present media of unknown type in this group
     */
    virtual uint32_t nbPresentUnknown() const = 0;
    /**
     * @brief nbPresentSeen Returns the number of present seen media in this group
     */
    virtual uint32_t nbPresentSeen() const = 0;
    /**
     * @brief nbVideo Returns the number of video (present or not ) media in this group
     */
    virtual uint32_t nbVideo() const = 0;
    /**
     * @brief nbAudio Returns the number of audio (present or not ) media in this group
     */
    virtual uint32_t nbAudio() const = 0;
    /**
     * @brief nbUnknown Returns the number of media of unknown type (present or not)
     *                  in this group
     */
    virtual uint32_t nbUnknown() const = 0;
    /**
     * @brief nbSeen Returns the number of seen media (present or not) in this group
     */
    virtual uint32_t nbSeen() const = 0;
    /**
     * @brief duration Returns this group duration
     *
     * Which is equal to the sum of all its member's durations
     */
    virtual int64_t duration() const = 0;
    /**
     * @brief creationDate Returns the group creation date
     *
     * The date is expressed as per time(2), ie. a number of seconds since
     * Epoch (UTC)
     */
    virtual time_t creationDate() const = 0;
    /**
     * @brief lastModificationDate Returns the group last modification date
     *
     * Modification date include last media addition/removal, and renaming
     * The date is expressed as per time(2), ie. a number of seconds since
     * Epoch (UTC)
     */
    virtual time_t lastModificationDate() const = 0;
    /**
     * @brief userInteracted Returns true if the group has had user interactions
     *
     * This includes being renamed, or being explicitely created with some specific
     * media or an explicit title.
     * It doesn't include groups that were automatically created by the media library
     * Removing a media from an automatically created group won't be interpreted
     * as a user interaction.
     */
    virtual bool userInteracted() const = 0;
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
     * @param pattern The search pattern (3 characters minimum)
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
     * @brief rename Rename a group
     * @param name The new name
     * @return true if the rename was successfull, false otherwise
     *
     * This will not change the group content, however, it will prevent further
     * media that matched the previous name to be automatically added to this
     * group when they are added to the media library.
     */
    virtual bool rename( std::string name ) = 0;
    /**
     * @brief destroy Destroys a media group.
     * @return true in case of success, false otherwise
     *
     * This will ungroup all media that are part of this group.
     */
    virtual bool destroy() = 0;
};

}
