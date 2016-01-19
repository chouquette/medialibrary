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

#include "History.h"

#include "Media.h"
#include "database/SqliteTools.h"

namespace policy
{
const std::string HistoryTable::Name = "History";
const std::string HistoryTable::PrimaryKeyColumn = "id_record";
unsigned int History::* const HistoryTable::PrimaryKey = &History::m_id;
}

constexpr unsigned int History::MaxEntries;

History::History( DBConnection dbConn, sqlite::Row& row )
{
    row >> m_id
        >> m_mrl
        >> m_mediaId
        >> m_date;
    if ( m_mediaId != 0 )
    {
        m_media = Media::load( dbConn, row );
    }
}

bool History::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::HistoryTable::Name +
            "("
                "id_record INTEGER PRIMARY KEY AUTOINCREMENT,"
                "mrl TEXT UNIQUE ON CONFLICT REPLACE,"
                "media_id INTEGER UNIQUE ON CONFLICT REPLACE,"
                "insertion_date UNSIGNED INT DEFAULT (strftime('%s', 'now')),"
                "FOREIGN KEY (media_id) REFERENCES " + policy::MediaTable::Name
                + "(id_media) ON DELETE CASCADE"
            ")";
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS limit_nb_records AFTER INSERT ON "
            + policy::HistoryTable::Name +
            " BEGIN "
            "DELETE FROM " + policy::HistoryTable::Name + " WHERE id_record in "
                "(SELECT id_record FROM " + policy::HistoryTable::Name +
                " ORDER BY insertion_date DESC LIMIT -1 OFFSET " + std::to_string( MaxEntries ) + ");"
            " END";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, triggerReq );
}

bool History::insert(DBConnection dbConn, const IMedia& media )
{
    static const std::string req = "INSERT INTO " + policy::HistoryTable::Name + "(media_id)"
            "VALUES(?)";
    return sqlite::Tools::insert( dbConn, req, media.id() ) != 0;
}

bool History::insert( DBConnection dbConn, const std::string& mrl )
{
    static const std::string req = "INSERT INTO " + policy::HistoryTable::Name + "(mrl)"
            "VALUES(?)";
    return sqlite::Tools::insert( dbConn, req, mrl ) != 0;
}

std::vector<std::shared_ptr<IHistoryEntry> > History::fetch( DBConnection dbConn )
{
    static const std::string req = "SELECT * FROM " + policy::HistoryTable::Name + " h "
            " LEFT OUTER JOIN " + policy::MediaTable::Name + " m ON m.id_media = h.media_id"
            " ORDER BY insertion_date DESC";
    return sqlite::Tools::fetchAll<History, IHistoryEntry>( dbConn, req );
}

MediaPtr History::media() const
{
    return m_media;
}

const std::string& History::mrl() const
{
    return m_mrl;
}

unsigned int History::insertionDate() const
{
    return m_date;
}
