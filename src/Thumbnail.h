/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class Thumbnail;

template <typename T>
class Cache;

namespace policy
{
struct ThumbnailTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t Thumbnail::*const PrimaryKey;
};
}

class Thumbnail : public DatabaseHelpers<Thumbnail, policy::ThumbnailTable>
{
public:
    enum class Origin : uint8_t
    {
        Artist,
        AlbumArtist,
        Album,
        Media,
        UserProvided,
    };

    Thumbnail( MediaLibraryPtr ml, sqlite::Row& row );
    Thumbnail( MediaLibraryPtr ml, std::string mrl, Origin origin );

    int64_t id() const;
    const std::string& mrl() const;
    bool update( std::string mrl );
    Origin origin() const;

    /**
     * @brief setMrlFromPrimaryKey Helper to set the thumbnail mrl based on a
     * thumbnail ID.
     * @param thumbnail The cached thumbnail entity. The value can be uncached
     * in which case it will be fetched and cached.
     * @param thumbnailId The thumbnail primary key
     */
    static bool setMrlFromPrimaryKey( MediaLibraryPtr ml,
                                      Cache<std::shared_ptr<Thumbnail>>& thumbnail,
                                      int64_t thumbnailId, std::string mrl );

    static void createTable( sqlite::Connection* dbConnection );
    static std::shared_ptr<Thumbnail> create( MediaLibraryPtr ml, std::string mrl,
                                              Origin origin );

    static const std::string EmptyMrl;

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    std::string m_mrl;
    Origin m_origin;

    friend policy::ThumbnailTable;
};

}
