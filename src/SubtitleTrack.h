/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs
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

#include "medialibrary/ISubtitleTrack.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class SubtitleTrack : public ISubtitleTrack, public DatabaseHelpers<SubtitleTrack>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t SubtitleTrack::* const PrimaryKey;
    };
    enum class Indexes : uint8_t
    {
        MediaId
    };

    SubtitleTrack( MediaLibraryPtr ml, sqlite::Row& row );
    SubtitleTrack( MediaLibraryPtr ml, std::string codec, std::string language,
                   std::string description, std::string encoding );
    virtual int64_t id() const override;
    virtual const std::string& codec() const override;
    virtual const std::string& language() const override;
    virtual const std::string& description() const override;
    virtual const std::string& encoding() const override;

    static void createTable( sqlite::Connection* dbConnection );
    static void createIndexes( sqlite::Connection* dbConnection );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static std::string index( Indexes index, uint32_t dbModel );
    static std::string indexName( Indexes index, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static std::shared_ptr<SubtitleTrack> create( MediaLibraryPtr ml,
                std::string codec, std::string language, std::string description,
                std::string encoding, int64_t mediaId );
    static bool removeFromMedia( MediaLibraryPtr ml, int64_t mediaId );

private:
    int64_t m_id;
    std::string m_codec;
    std::string m_language;
    std::string m_description;
    std::string m_encoding;
};

}
