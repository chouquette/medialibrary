/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
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

#include "Types.h"
#include "database/DatabaseHelpers.h"
#include "medialibrary/IHistoryEntry.h"

#include <vector>
#include <string>

namespace medialibrary
{

class History;
class Media;

namespace policy
{
struct HistoryTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t History::* const PrimaryKey;
};
}

class History : public IHistoryEntry, public DatabaseHelpers<History, policy::HistoryTable, cachepolicy::Uncached<History>>
{
public:
    History( MediaLibraryPtr ml, sqlite::Row& row );
    static void createTable( sqlite::Connection* dbConnection );
    static bool insert( sqlite::Connection* dbConn, int64_t mediaId );
    static std::vector<HistoryPtr> fetch( MediaLibraryPtr ml );
    static void clearStreams( MediaLibraryPtr ml );

    virtual MediaPtr media() const override;
    virtual unsigned int insertionDate() const override;

    static constexpr unsigned int MaxEntries = 20u;

private:
    MediaPtr m_media;
    int64_t m_mediaId;
    unsigned int m_date;

    friend policy::HistoryTable;
};

}
