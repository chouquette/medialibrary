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

#include <vector>
#include <memory>

namespace medialibrary
{

template <typename T>
class IQuery
{
public:
    using Result = std::vector<std::shared_ptr<T>>;

    virtual ~IQuery() = default;
    /**
     * @brief count returns the total number of items that would be returned by all()
     *
     * There is no temporal guarantee, meaning that if an item gets added between
     * a call to count() and all(), the call to all() will return count() + 1 items.
     */
    virtual size_t count() = 0;
    /**
     * @brief items returns a subset of a query result
     * @param nbItems The number of item requested
     * @param offset The number of elements to omit from the begining of the result
     * @return A vector of shared pointer for the requested type.
     *
     * If nbItems & offset are both 0, then this method returns all results.
     * nbItems & offset are directly mapped to the LIMIT OFFSET parameters of
     * the SQL query that will be generated to compute the result.
     */
    virtual Result items( uint32_t nbItems,  uint32_t offset ) = 0;
    virtual Result all() = 0;
};

template <typename T>
using Query = std::unique_ptr<IQuery<T>>;

}
