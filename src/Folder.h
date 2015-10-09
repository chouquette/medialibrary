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

#include "database/Cache.h"
#include "IFolder.h"

#include <sqlite3.h>

class Folder;

namespace fs
{
    class IDirectory;
}

namespace policy
{
struct FolderTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Folder::*const PrimaryKey;
};

struct FolderCache
{
    using KeyType = std::string;
    static const KeyType& key( const std::shared_ptr<Folder>& self );
    static KeyType key( sqlite3_stmt* stmt );
};

}

class Folder : public IFolder, public Cache<Folder, IFolder, policy::FolderTable, policy::FolderCache>
{
    using _Cache = Cache<Folder, IFolder, policy::FolderTable, policy::FolderCache>;

public:
    Folder( DBConnection dbConnection, sqlite3_stmt* stmt );
    Folder( const std::string& path, time_t lastModificationDate, bool isRemovable, unsigned int parent );

    static bool createTable( DBConnection connection );
    static FolderPtr create( DBConnection connection, const std::string& path, time_t lastModificationDate, bool isRemovable, unsigned int parentId );

    virtual unsigned int id() const override;
    virtual const std::string& path() override;
    virtual std::vector<FilePtr> files() override;
    virtual std::vector<FolderPtr> folders() override;
    virtual FolderPtr parent() override;
    virtual unsigned int lastModificationDate() override;
    virtual bool setLastModificationDate(unsigned int lastModificationDate) override;
    virtual bool isRemovable() override;

private:
    DBConnection m_dbConection;

    unsigned int m_id;
    std::string m_path;
    unsigned int m_parent;
    unsigned int m_lastModificationDate;
    bool m_isRemovable;

    friend _Cache;
    friend struct policy::FolderTable;
};
