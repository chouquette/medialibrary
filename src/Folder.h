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

#include "medialibrary/IFolder.h"
#include "database/DatabaseHelpers.h"
#include "factory/IFileSystem.h"
#include "utils/Cache.h"

#include <sqlite3.h>

namespace medialibrary
{

class File;
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
    static int64_t Folder::*const PrimaryKey;
};
}

// This doesn't publicly expose the DatabaseHelper inheritance in order to force
// the user to go through Folder's overloads, as they take care of the device mountpoint
// fetching & path composition
class Folder : public IFolder, public DatabaseHelpers<Folder, policy::FolderTable>
{
public:
    Folder( MediaLibraryPtr ml, sqlite::Row& row );
    Folder(MediaLibraryPtr ml, const std::string& path, int64_t parent , int64_t deviceId , bool isRemovable );

    static bool createTable( DBConnection connection );
    static std::shared_ptr<Folder> create( MediaLibraryPtr ml, const std::string& path, int64_t parentId, Device& device, fs::IDevice& deviceFs );
    static bool blacklist( MediaLibraryPtr ml, const std::string& fullPath );
    static std::vector<std::shared_ptr<Folder>> fetchRootFolders( MediaLibraryPtr ml );

    static std::shared_ptr<Folder> fromPath(MediaLibraryPtr ml, const std::string& fullPath );
    static std::shared_ptr<Folder> blacklistedFolder(MediaLibraryPtr ml, const std::string& fullPath );

    virtual int64_t id() const override;
    virtual const std::string& path() const override;
    std::vector<std::shared_ptr<File>> files();
    std::vector<std::shared_ptr<Folder>> folders();
    std::shared_ptr<Folder> parent();
    int64_t deviceId() const;
    virtual bool isPresent() const override;

private:
    enum class BannedType
    {
        Yes,    //< Only select banned folders
        No,     //< Only select unbanned folders
        Any,    //< Well... any of the above.
    };

    static std::shared_ptr<Folder> fromPath( MediaLibraryPtr ml, const std::string& fullPath, BannedType bannedType );

private:
    MediaLibraryPtr m_ml;

    int64_t m_id;
    // This contains the path relative to the device mountpoint (ie. excluding it)
    std::string m_path;
    int64_t m_parent;
    bool m_isBlacklisted;
    int64_t m_deviceId;
    bool m_isPresent;
    bool m_isRemovable;

    mutable Cache<std::string> m_deviceMountpoint;
    // This contains the full path, including device mountpoint.
    mutable std::string m_fullPath;

    friend struct policy::FolderTable;
};

}
