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

#include "Settings.h"

#include "database/SqliteTools.h"
#include "MediaLibrary.h"

namespace medialibrary
{

const uint32_t Settings::DbModelVersion = 2u;

Settings::Settings()
    : m_dbConn( nullptr )
    , m_dbModelVersion( 0 )
    , m_changed( false )
{
}

bool Settings::load( DBConnection dbConn )
{
    m_dbConn = dbConn;
    sqlite::Statement s( m_dbConn->getConn(), "SELECT * FROM Settings" );
    auto row = s.row();
    // First launch: no settings
    if ( row == nullptr )
    {
        if ( sqlite::Tools::executeInsert( m_dbConn, "INSERT INTO Settings VALUES(?)", DbModelVersion ) == false )
            return false;
        m_dbModelVersion = DbModelVersion;
    }
    else
    {
        row >> m_dbModelVersion;
        // safety check: there sould only be one row
        assert( s.row() == nullptr );
    }
    return true;
}

uint32_t Settings::dbModelVersion() const
{
    return m_dbModelVersion;
}

bool Settings::save()
{
    static const std::string req = "UPDATE Settings SET db_model_version = ?";
    if ( m_changed == false )
        return true;
    if ( sqlite::Tools::executeUpdate( m_dbConn, req, m_dbModelVersion ) == true )
    {
        m_changed = false;
        return true;
    }
    return false;
}

void Settings::setDbModelVersion(uint32_t dbModelVersion)
{
    m_dbModelVersion = dbModelVersion;
    m_changed = true;
}

bool Settings::createTable( DBConnection dbConn )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS Settings("
                "db_model_version UNSIGNED INTEGER NOT NULL DEFAULT 1"
            ")";
    return sqlite::Tools::executeRequest( dbConn, req );
}

}
