/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include <cstdint>
#include <string>

namespace medialibrary
{

class IBookmark
{
public:
    virtual ~IBookmark() = default;
    /**
     * @brief time Returns the time of this bookmark, as it was
     * provided to \ref{IMedia::addBookmark}
     */
    virtual int64_t time() const = 0;
    /**
     * @brief name Returns the bookmark name
     */
    virtual const std::string& name() const = 0;
    /**
     * @brief setName Updates the bookmark name
     */
    virtual bool setName( std::string name ) = 0;
    /**
     * @brief name Returns the bookmark description
     */
    virtual const std::string& description() const = 0;
    /**
     * @brief setDescription Updates the bookmark description
     */
    virtual bool setDescription( std::string description ) = 0;
    /**
     * @brief setNameAndDescription Convenience helper to update the name and
     * description in a single operation
     */
    virtual bool setNameAndDescription( std::string name, std::string desc ) = 0;
    /**
     * @brief move Move a bookmark to a new time in the media
     * @param newTime The new time for this bookmark
     * @return true if successful, false otherwise.
     *
     * @note If a bookmark is already present at the given time, the move will fail
     */
    virtual bool move( int64_t newTime ) = 0;
};

}
