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

#include "medialibrary/IVideoGroup.h"
#include "Types.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

struct QueryParameters;

class VideoGroup : public IVideoGroup, public DatabaseHelpers<VideoGroup>
{
public:
    struct Table
    {
        static const std::string Name;
    };

    virtual ~VideoGroup() = default;
    VideoGroup( MediaLibraryPtr ml, sqlite::Row& row );

    virtual const std::string& name() const override;
    virtual size_t count() const override;
    virtual Query<IMedia> media( const QueryParameters* params ) const override;
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       const QueryParameters* params ) const override;

    static Query<IVideoGroup> listAll( MediaLibraryPtr ml,
                                       const QueryParameters* params );
    static VideoGroupPtr fromName( MediaLibraryPtr ml, const std::string& name );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static void createView( sqlite::Connection* dbConn );

private:
    MediaLibraryPtr m_ml;
    std::string m_name;
    size_t m_count;
};

}
