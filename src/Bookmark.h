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

#include "medialibrary/IBookmark.h"
#include "database/DatabaseHelpers.h"
#include "medialibrary/IQuery.h"

namespace medialibrary
{

class Bookmark : public IBookmark, public DatabaseHelpers<Bookmark>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Bookmark::*const PrimaryKey;
    };

    Bookmark(MediaLibraryPtr ml, int64_t time);
    Bookmark( MediaLibraryPtr ml, sqlite::Row& row );
    virtual int64_t id() const override;
    virtual int64_t time() const override;
    virtual const std::string& name() const override;
    virtual bool setName( std::string name ) override;
    virtual const std::string& description() const override;
    virtual bool setDescription( std::string description ) override;
    virtual bool setNameAndDescription( std::string name, std::string desc ) override;
    virtual bool move( int64_t newTime ) override;

    static void createTable( sqlite::Connection* dbConn );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static std::shared_ptr<Bookmark> create(MediaLibraryPtr ml, int64_t time, int64_t mediaId );
    static bool remove( MediaLibraryPtr ml, int64_t time, int64_t mediaId );
    static Query<IBookmark> fromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                       const QueryParameters* params );
    static std::shared_ptr<Bookmark> fromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                                int64_t time );
    static bool removeAll( MediaLibraryPtr ml, int64_t mediaId );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    int64_t m_time;
    std::string m_name;
    std::string m_description;
};

}
