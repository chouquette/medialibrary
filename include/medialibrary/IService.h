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

#include "IQuery.h"
#include <string>

namespace medialibrary
{

class ISubscription;
struct QueryParameters;

/**
 * @brief The IService interface represents a service that can allow the user
 * to import subscriptions
 */
class IService
{
public:
    enum class Type : uint8_t
    {
        /*
         * The type is used as the service's primary key, so any valid value
         * must be != 0
         */
        Podcast = 1,
    };
    virtual ~IService() = default;
    virtual Type type() const = 0;
    virtual bool isAutoDownloadEnabled() const = 0;
    virtual bool setAutoDownloadEnabled( bool enabled ) = 0;
    /**
     * @brief newMediaNotification Returns true if new content for this service
     *                             should issue a notification
     *
     * Each subscription can override this setting, or inherit it.
     */
    virtual bool isNewMediaNotificationEnabled() const = 0;
    virtual bool setNewMediaNotificationEnabled( bool enabled ) = 0;
    /**
     * @brief maxCachedSize Returns the maximum size of the cache for all of this service
     * @return The maximum cache size or -1 if the limit isn't set for this service
     *
     * If the limit isn't set, the global maximum cache size setting will be
     * used instead.
     */
    virtual int64_t maxCachedSize() const = 0;
    /**
     * @brief setMaxCachedSize Sets the maximum cache size for this service
     * @param maxSize A size in bytes, or -1 to disable the setting for the service
     *                and inherit the default one.
     * @return true if the change was successful, false otherwise.
     */
    virtual bool setMaxCachedSize( int64_t maxSize ) = 0;
    virtual bool addSubscription( std::string mrl ) = 0;
    virtual Query<ISubscription> subscriptions( const QueryParameters* params ) = 0;
};
}
