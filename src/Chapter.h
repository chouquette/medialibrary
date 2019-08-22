/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2018-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "medialibrary/IChapter.h"
#include "database/DatabaseHelpers.h"
#include "Types.h"

namespace medialibrary
{

class Chapter : public IChapter, public DatabaseHelpers<Chapter>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Chapter::*const PrimaryKey;
    };

    Chapter( MediaLibraryPtr ml, sqlite::Row& row );
    Chapter( MediaLibraryPtr ml, int64_t offset, int64_t duration,
             std::string name );


    virtual const std::string& name() const override;
    virtual int64_t offset() const override;
    virtual int64_t duration() const override;

    static void createTable( sqlite::Connection* dbConn );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static std::shared_ptr<Chapter> create( MediaLibraryPtr ml, int64_t offset,
                                            int64_t duration, std::string name,
                                            int64_t mediaId );
    static Query<IChapter> fromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                      const QueryParameters* params );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    int64_t m_offset;
    int64_t m_duration;
    std::string m_name;

    friend Chapter::Table;
};

}
