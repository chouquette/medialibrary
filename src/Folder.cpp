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

#include "Folder.h"
#include "Device.h"
#include "Media.h"

#include "database/SqliteTools.h"
#include "filesystem/IDirectory.h"

namespace policy
{
    const std::string FolderTable::Name = "Folder";
    const std::string FolderTable::PrimaryKeyColumn = "id_folder";
    unsigned int Folder::* const FolderTable::PrimaryKey = &Folder::m_id;
}

Folder::Folder(DBConnection dbConnection, sqlite::Row& row )
    : m_dbConection( dbConnection )
{
    row >> m_id
        >> m_path
        >> m_parent
        >> m_lastModificationDate
        >> m_isBlacklisted
        >> m_deviceId
        >> m_isPresent;
}

Folder::Folder( const std::string& path, time_t lastModificationDate, unsigned int parent, unsigned int deviceId )
    : m_id( 0 )
    , m_path( path )
    , m_parent( parent )
    , m_lastModificationDate( lastModificationDate )
    , m_isBlacklisted( false )
    , m_deviceId( deviceId )
    , m_isPresent( true )
{
}

bool Folder::createTable(DBConnection connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FolderTable::Name +
            "("
            "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
            "path TEXT UNIQUE ON CONFLICT FAIL,"
            "id_parent UNSIGNED INTEGER,"
            "last_modification_date UNSIGNED INTEGER,"
            "is_blacklisted INTEGER,"
            "device_id UNSIGNED INTEGER,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "FOREIGN KEY (id_parent) REFERENCES " + policy::FolderTable::Name +
            "(id_folder) ON DELETE CASCADE,"
            "FOREIGN KEY (device_id) REFERENCES " + policy::DeviceTable::Name +
            "(id_device) ON DELETE CASCADE"
            ")";
    std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_device_present AFTER UPDATE OF is_present ON "
            + policy::DeviceTable::Name +
            " BEGIN"
            " UPDATE " + policy::FolderTable::Name + " SET is_present = new.is_present WHERE device_id = new.id_device;"
            " END";
    return sqlite::Tools::executeRequest( connection, req ) &&
            sqlite::Tools::executeRequest( connection, triggerReq );
}

std::shared_ptr<Folder> Folder::create(DBConnection connection, const std::string& path, time_t lastModificationDate, unsigned int parentId, Device& device )
{
    auto self = std::make_shared<Folder>( path, lastModificationDate, parentId, device.id() );
    static const std::string req = "INSERT INTO " + policy::FolderTable::Name +
            "(path, id_parent, last_modification_date, device_id) VALUES(?, ?, ?, ?)";
    if ( insert( connection, self, req, path, sqlite::ForeignKey( parentId ),
                         lastModificationDate, device.id() ) == false )
        return nullptr;
    self->m_dbConection = connection;
    return self;
}

bool Folder::blacklist( DBConnection connection, const std::string& path )
{
    static const std::string req = "INSERT INTO " + policy::FolderTable::Name +
            "(path, id_parent, is_blacklisted) VALUES(?, ?, ?)";
    return sqlite::Tools::insert( connection, req, path, nullptr, true ) != 0;
}

std::shared_ptr<Folder> Folder::fromPath( DBConnection conn, const std::string& path )
{
    const std::string req = "SELECT * FROM " + policy::FolderTable::Name + " WHERE path = ? AND is_blacklisted IS NULL";
    return fetch( conn, req, path );
}

unsigned int Folder::id() const
{
    return m_id;
}

const std::string& Folder::path() const
{
    return m_path;
}

std::vector<MediaPtr> Folder::files()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name +
        " WHERE folder_id = ?";
    return Media::fetchAll<IMedia>( m_dbConection, req, m_id );
}

std::vector<FolderPtr> Folder::folders()
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name +
            " WHERE id_parent = ? AND is_blacklisted IS NULL";
    return fetchAll<IFolder>( m_dbConection, req, m_id );
}

FolderPtr Folder::parent()
{
    return fetch( m_dbConection, m_parent );
}

unsigned int Folder::lastModificationDate()
{
    return m_lastModificationDate;
}

bool Folder::setLastModificationDate( unsigned int lastModificationDate )
{
    static const std::string req = "UPDATE " + policy::FolderTable::Name +
            " SET last_modification_date = ? WHERE id_folder = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConection, req, lastModificationDate, m_id ) == false )
        return false;
    m_lastModificationDate = lastModificationDate;
    return true;
}

unsigned int Folder::deviceId() const
{
    return m_deviceId;
}

bool Folder::isPresent() const
{
    return m_isPresent;
}
