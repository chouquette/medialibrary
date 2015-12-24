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

#include "database/DatabaseHelpers.h"
#include "factory/IFileSystem.h"

#include <sqlite3.h>

class Folder;
class Device;

namespace fs
{
    class IDirectory;
}

namespace policy
{
struct FolderTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static unsigned int Folder::*const PrimaryKey;
};

}

class Folder : public DatabaseHelpers<Folder, policy::FolderTable>
{
public:
    Folder( DBConnection dbConnection, sqlite::Row& row );
    Folder(const std::string& path, time_t lastModificationDate, unsigned int parent , unsigned int deviceId);

    static bool createTable( DBConnection connection );
    static std::shared_ptr<Folder> create(DBConnection connection, const std::string& path, time_t lastModificationDate, unsigned int parentId, Device& device );
    static bool blacklist( DBConnection connection, const std::string& path );
    ///
    /// \brief setFileSystemFactory Sets a file system factory to be used when building IDevices
    /// This is assumed to be called once, before any discovery/reloading process is launched.
    /// \param fsFactory The factory to be used
    ///
    static void setFileSystemFactory( std::shared_ptr<factory::IFileSystem> fsFactory );

    static std::shared_ptr<Folder> fromPath( DBConnection conn, const std::string& path );

    unsigned int id() const;
    const std::string& path() const;
    std::vector<MediaPtr> files();
    std::vector<std::shared_ptr<Folder>> folders();
    std::shared_ptr<Folder> parent();
    unsigned int lastModificationDate();
    bool setLastModificationDate(unsigned int lastModificationDate);
    unsigned int deviceId() const;
    bool isPresent() const;

private:
    DBConnection m_dbConection;

    unsigned int m_id;
    std::string m_path;
    unsigned int m_parent;
    unsigned int m_lastModificationDate;
    bool m_isBlacklisted;
    unsigned int m_deviceId;
    bool m_isPresent;

    friend struct policy::FolderTable;
};
