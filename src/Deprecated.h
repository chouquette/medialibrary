/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2021 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

/* This file is meant to contain old tables or similar references to deprecated
 * code. It shouldn't be referenced outside of old schema/trigger/index functions
 * or old migration code
 */

#include <string>

namespace medialibrary
{
class AlbumTrack
{
public:
    struct Table
    {
        static const std::string Name;
    };
    enum class Indexes : uint8_t
    {
        MediaArtistGenreAlbum,
        AlbumGenreArtist,
    };
    static std::string schema( const std::string& tableName, uint32_t );
    static std::string index( AlbumTrack::Indexes index, uint32_t dbModel );
    static std::string indexName( AlbumTrack::Indexes index, uint32_t );
};
}
