/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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
#include "medialibrary/Types.h"
#include "medialibrary/IQuery.h"
#include "medialibrary/IService.h"

namespace medialibrary
{

struct QueryParameters;

class ISubscription
{
public:
    virtual ~ISubscription() = default;
    virtual int64_t id() const = 0;
    virtual IService::Type service() const = 0;
    virtual const std::string& name() const = 0;
    virtual Query<ISubscription> childSubscriptions( const QueryParameters* params ) const = 0;
    virtual SubscriptionPtr parent() = 0;
    virtual Query<IMedia> media( const QueryParameters* params ) const = 0;
    virtual bool refresh() = 0;
    /**
     * @brief cachedSize Returns the sum of all cached files for this collection
     */
    virtual uint64_t cachedSize() const = 0;
    /**
     * @brief maxCachedMedia Returns the maximum number of cached media for this
     *                       collection, or -1 is unset
     */
    virtual int32_t maxCachedMedia() const = 0;
    /**
     * @brief setMaxCachedMedia Sets the maximum number of automatically cached media
     * @param nbCachedMedia The number of cached media, or a negative value to unset
     * @return true if the setting was changed, false otherwise
     *
     * If this isn't set for the collection, the global setting is used instead.
     * If this is set to a negative value, this collection's setting will be
     * erased and the parent setting will be used again when caching, however
     * the maxCachedMedia will return -1 to denote that this collection inherits
     * its setting from its parent.
     */
    virtual bool setMaxCachedMedia( int32_t nbCachedMedia ) = 0;
    /**
     * @brief maxCachedSize Returns the maximum size in bytes for this collection
     *
     * This will return the collection specific setting, regardless of the global
     * setting value.
     * In case the value is unset, -1 will be returned. In this case, the parent
     * setting will be used when performing caching.
     */
    virtual int64_t maxCachedSize() const = 0;
    /**
     * @brief setMaxCachedSize Sets the maximum cached size for this collection
     * @param maxCachedSize The size in bytes for this collection cache, or a
     *                      negative value to use the parent setting.
     * @return true if the change was successful, false otherwise.
     *
     * This will not perform any checks against the global setting, but if this
     * is greater than the global maximum cache size, the global setting will
     * prevail when caching.
     * If passing the current value, this will return true.
     */
    virtual bool setMaxCachedSize( int64_t maxCachedSize ) = 0;
    /**
     * @brief newMediaNotification Returns the new media notification setting
     * @return A positive value if explicitly enabled, 0 if explicitly disabled, -1 is unset
     *
     * If this value is unset, the parent service setting will be used.
     */
    virtual int8_t newMediaNotification() const = 0;
    /**
     * @brief setNewMediaNotification Sets the new media notification setting
     * @param value A negative value to default to the parent service setting, 0
     *              to disable, and a positive value to enable.
     * @return true if the change was successful, false otherwise.
     */
    virtual bool setNewMediaNotification( int8_t value ) = 0;
    /**
     * @brief nbUnplayedMedia Returns the number of media that belong to this
     *                        subscription and haven't been played
     */
    virtual uint32_t nbUnplayedMedia() const = 0;
};

}
