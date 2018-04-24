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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "History.h"
#include "Media.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"

namespace medialibrary
{

namespace policy
{
const std::string HistoryTable::Name = "History";
const std::string HistoryTable::PrimaryKeyColumn = "id_media";
int64_t History::* const HistoryTable::PrimaryKey = &History::m_mediaId;
}

constexpr unsigned int History::MaxEntries;

History::History( MediaLibraryPtr ml, sqlite::Row& row )
    : m_media( Media::load( ml, row ) )
{
    // In case we load the media from cache, we won't advance in columns
    row.advanceToColumn( row.nbColumns() - 1 );
    row >> m_date;
}

void History::createTable( sqlite::Connection* dbConnection )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::HistoryTable::Name +
            "("
                "id_media INTEGER PRIMARY KEY,"
                "insertion_date UNSIGNED INT NOT NULL,"
                "FOREIGN KEY (id_media) REFERENCES " + policy::MediaTable::Name +
                "(id_media) ON DELETE CASCADE"
            ")";
    // Don't index the id_media field, we don't want to select history records using the media_id
    sqlite::Tools::executeRequest( dbConnection, req );
}

void History::createTriggers(sqlite::Connection* dbConnection)
{
    const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS limit_nb_records AFTER INSERT ON "
            + policy::HistoryTable::Name +
            " BEGIN "
            "DELETE FROM " + policy::HistoryTable::Name + " WHERE id_media in "
                "(SELECT id_media FROM " + policy::HistoryTable::Name +
                " ORDER BY insertion_date DESC LIMIT -1 OFFSET " + std::to_string( MaxEntries ) + ");"
            " END";
    sqlite::Tools::executeRequest( dbConnection, triggerReq );
}

bool History::insert( sqlite::Connection* dbConn, int64_t mediaId )
{
    static const std::string req = "INSERT OR REPLACE INTO " + policy::HistoryTable::Name +
            "(id_media, insertion_date) VALUES(?, strftime('%s', 'now'))";
    return sqlite::Tools::executeInsert( dbConn, req, mediaId ) != 0;
}

Query<IHistoryEntry> History::fetch( MediaLibraryPtr ml )
{
    static const std::string req = "FROM " + policy::MediaTable::Name + " f "
            "INNER JOIN " + policy::HistoryTable::Name + " h ON h.id_media = f.id_media "
            "ORDER BY h.insertion_date DESC";
    return make_query<History, IHistoryEntry>( ml, "f.*, h.insertion_date", req );
}

void History::clearStreams( MediaLibraryPtr ml )
{
    static const std::string req = "DELETE FROM " + policy::HistoryTable::Name;
    sqlite::Tools::executeRequest( ml->getConn(), req );
}

MediaPtr History::media() const
{
    return m_media;
}

unsigned int History::insertionDate() const
{
    return m_date;
}

}
