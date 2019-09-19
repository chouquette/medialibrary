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
#include "Types.h"
#include "IQuery.h"

namespace medialibrary
{

struct QueryParameters;

class IVideoGroup
{
public:
    virtual ~IVideoGroup() = default;
    /**
     * @brief name returns the name of this group
     */
    virtual const std::string& name() const = 0;
    /**
     * @brief count Returns the number of videos in this group
     */
    virtual size_t count() const = 0;
    /**
     * @brief media Returns a query object to fetch this group media
     */
    virtual Query<IMedia> media( const QueryParameters* params ) const = 0;
    /**
     * @brief media Returns a query object to search this group media
     */
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       const QueryParameters* params ) const = 0;
};

}
