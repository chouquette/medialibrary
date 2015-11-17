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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "Types.h"

class Settings
{
public:
    Settings();
    bool load(DBConnection dbConn);
    uint32_t dbModelVersion() const;
    bool save();
    void setDbModelVersion( uint32_t dbModelVersion );

    static bool createTable(DBConnection dbConn);

private:
    DBConnection m_dbConn;

    uint32_t m_dbModelVersion;

    bool m_changed;
};

#endif // SETTINGS_H
