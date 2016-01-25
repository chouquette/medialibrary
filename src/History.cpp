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

#include "database/SqliteTools.h"

namespace policy
{
const std::string HistoryTable::Name = "History";
const std::string HistoryTable::PrimaryKeyColumn = "id_record";
unsigned int History::* const HistoryTable::PrimaryKey = &History::m_id;
}

constexpr unsigned int History::MaxEntries;

History::History( DBConnection dbConn, sqlite::Row& row )
    : m_dbConn( dbConn )
{
    row >> m_id
        >> m_mrl
        >> m_date
        >> m_favorite;
}

bool History::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::HistoryTable::Name +
            "("
                "id_record INTEGER PRIMARY KEY AUTOINCREMENT,"
                "mrl TEXT UNIQUE ON CONFLICT FAIL,"
                "insertion_date UNSIGNED INT NOT NULL DEFAULT (strftime('%s', 'now')),"
                "favorite BOOLEAN NOT NULL DEFAULT 0"
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

bool History::insert( DBConnection dbConn, const std::string& mrl )
{
    History::clear();
    static const std::string req = "INSERT OR REPLACE INTO " + policy::HistoryTable::Name +
            "(id_record, mrl, insertion_date, favorite)"
            " SELECT id_record, mrl, strftime('%s', 'now'), favorite FROM " +
            policy::HistoryTable::Name + " WHERE mrl = ?"
            " UNION SELECT NULL, ?, NULL, NULL"
            " ORDER BY id_record DESC"
            " LIMIT 1";
    return sqlite::Tools::insert( dbConn, req, mrl, mrl ) != 0;
}

std::vector<std::shared_ptr<IHistoryEntry> > History::fetch( DBConnection dbConn )
{
    static const std::string req = "SELECT * FROM " + policy::HistoryTable::Name + " ORDER BY insertion_date DESC";
    return fetchAll<IHistoryEntry>( dbConn, req );
}

const std::string& History::mrl() const
{
    return m_mrl;
}

unsigned int History::insertionDate() const
{
    return m_date;
}

bool History::isFavorite() const
{
    return m_favorite;
}

bool History::setFavorite( bool isFavorite )
{
    if ( isFavorite == m_favorite )
        return true;

    static const std::string req = "UPDATE " + policy::HistoryTable::Name + " SET favorite = ? WHERE id_record = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConn, req, isFavorite, m_id ) == false )
        return false;
    m_favorite = isFavorite;
    return true;
}
