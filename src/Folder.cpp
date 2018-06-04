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

#include "File.h"
#include "Folder.h"
#include "Device.h"
#include "Media.h"

#include "database/SqliteTools.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IDevice.h"
#include "utils/Filename.h"

#include <unordered_map>

namespace medialibrary
{

namespace policy
{
    const std::string FolderTable::Name = "Folder";
    const std::string FolderTable::PrimaryKeyColumn = "id_folder";
    int64_t Folder::* const FolderTable::PrimaryKey = &Folder::m_id;
}

Folder::Folder( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    bool dummy;
    row >> m_id
        >> m_path
        >> m_parent
        >> m_isBlacklisted
        >> m_deviceId
        >> dummy
        >> m_isRemovable;
}

Folder::Folder(MediaLibraryPtr ml, const std::string& path, int64_t parent, int64_t deviceId, bool isRemovable )
    : m_ml( ml )
    , m_id( 0 )
    , m_path( path )
    , m_parent( parent )
    , m_isBlacklisted( false )
    , m_deviceId( deviceId )
    , m_isRemovable( isRemovable )
{
}

void Folder::createTable( sqlite::Connection* connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FolderTable::Name +
            "("
            "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
            "path TEXT,"
            "parent_id UNSIGNED INTEGER,"
            "is_blacklisted BOOLEAN NOT NULL DEFAULT 0,"
            "device_id UNSIGNED INTEGER,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "is_removable BOOLEAN NOT NULL,"
            "FOREIGN KEY (parent_id) REFERENCES " + policy::FolderTable::Name +
            "(id_folder) ON DELETE CASCADE,"
            "FOREIGN KEY (device_id) REFERENCES " + policy::DeviceTable::Name +
            "(id_device) ON DELETE CASCADE,"
            "UNIQUE(path, device_id) ON CONFLICT FAIL"
            ")";
    std::string exclEntryReq = "CREATE TABLE IF NOT EXISTS ExcludedEntryFolder("
                               "folder_id UNSIGNED INTEGER NOT NULL,"
                               "FOREIGN KEY (folder_id) REFERENCES " + policy::FolderTable::Name +
                               "(id_folder) ON DELETE CASCADE,"
                               "UNIQUE(folder_id) ON CONFLICT FAIL"
                               ")";

    sqlite::Tools::executeRequest( connection, req );
    sqlite::Tools::executeRequest( connection, exclEntryReq );
}

void Folder::createTriggers( sqlite::Connection* connection )
{
    std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_device_present AFTER UPDATE OF is_present ON "
            + policy::DeviceTable::Name +
            " WHEN old.is_present != new.is_present"
            " BEGIN"
            " UPDATE " + policy::FolderTable::Name + " SET is_present = new.is_present WHERE device_id = new.id_device;"
            " END";
    std::string deviceIndexReq = "CREATE INDEX IF NOT EXISTS folder_device_id_idx ON " +
            policy::FolderTable::Name + " (device_id)";
    std::string parentFolderIndexReq = "CREATE INDEX IF NOT EXISTS parent_folder_id_idx ON " +
            policy::FolderTable::Name + " (parent_id)";

    sqlite::Tools::executeRequest( connection, triggerReq );
    sqlite::Tools::executeRequest( connection, deviceIndexReq );
    sqlite::Tools::executeRequest( connection, parentFolderIndexReq );
}

std::shared_ptr<Folder> Folder::create( MediaLibraryPtr ml, const std::string& mrl,
                                        int64_t parentId, Device& device, fs::IDevice& deviceFs )
{
    std::string path;
    if ( device.isRemovable() == true )
        path = utils::file::removePath( mrl, deviceFs.mountpoint() );
    else
        path = mrl;
    auto self = std::make_shared<Folder>( ml, path, parentId, device.id(), device.isRemovable() );
    static const std::string req = "INSERT INTO " + policy::FolderTable::Name +
            "(path, parent_id, device_id, is_removable) VALUES(?, ?, ?, ?)";
    if ( insert( ml, self, req, path, sqlite::ForeignKey( parentId ), device.id(), device.isRemovable() ) == false )
        return nullptr;
    if ( device.isRemovable() == true )
    {
        self->m_deviceMountpoint = deviceFs.mountpoint();
        self->m_fullPath = self->m_deviceMountpoint.get() + path;
    }
    return self;
}

void Folder::excludeEntryFolder( MediaLibraryPtr ml, int64_t folderId )
{
    std::string req = "INSERT INTO ExcludedEntryFolder(folder_id) VALUES(?)";
    sqlite::Tools::executeRequest( ml->getConn(), req, folderId );
}

bool Folder::blacklist( MediaLibraryPtr ml, const std::string& mrl )
{
    // Ensure we delete the existing folder if any & blacklist the folder in an "atomic" way
    return sqlite::Tools::withRetries( 3, [ml, &mrl]() {
        auto t = ml->getConn()->newTransaction();

        auto f = fromMrl( ml, mrl, BannedType::Any );
        if ( f != nullptr )
        {
            // No need to blacklist a folder twice
            if ( f->m_isBlacklisted == true )
                return true;
            // Let the foreign key destroy everything beneath this folder
            destroy( ml, f->id() );
        }
        auto fsFactory = ml->fsFactoryForMrl( mrl );
        if ( fsFactory == nullptr )
            return false;
        std::shared_ptr<fs::IDirectory> folderFs;
        try
        {
            folderFs = fsFactory->createDirectory( mrl );
        }
        catch ( std::system_error& ex )
        {
            LOG_ERROR( "Failed to instantiate a directory to ban folder: ", ex.what() );
            return false;
        }
        auto deviceFs = folderFs->device();
        if ( deviceFs == nullptr )
        {
            LOG_ERROR( "Can't find device associated with mrl ", mrl );
            return false;
        }
        auto device = Device::fromUuid( ml, deviceFs->uuid() );
        if ( device == nullptr )
            device = Device::create( ml, deviceFs->uuid(), utils::file::scheme( mrl ), deviceFs->isRemovable() );
        std::string path;
        if ( deviceFs->isRemovable() == true )
            path = utils::file::removePath( mrl, deviceFs->mountpoint() );
        else
            path = mrl;
        static const std::string req = "INSERT INTO " + policy::FolderTable::Name +
                "(path, parent_id, is_blacklisted, device_id, is_removable) VALUES(?, ?, ?, ?, ?)";
        auto res = sqlite::Tools::executeInsert( ml->getConn(), req, path, nullptr, true, device->id(), deviceFs->isRemovable() ) != 0;
        t->commit();
        return res;
    });
}

std::shared_ptr<Folder> Folder::fromMrl( MediaLibraryPtr ml, const std::string& mrl )
{
    return fromMrl( ml, mrl, BannedType::No );
}

std::shared_ptr<Folder> Folder::blacklistedFolder( MediaLibraryPtr ml, const std::string& mrl )
{
    return fromMrl( ml, mrl, BannedType::Yes );
}

std::shared_ptr<Folder> Folder::fromMrl( MediaLibraryPtr ml, const std::string& mrl, BannedType bannedType )
{
    if ( mrl.empty() == true )
        return nullptr;
    auto fsFactory = ml->fsFactoryForMrl( mrl );
    if ( fsFactory == nullptr )
        return nullptr;
    std::shared_ptr<fs::IDirectory> folderFs;
    try
    {
        folderFs = fsFactory->createDirectory( mrl );
    }
    catch ( const std::system_error& ex )
    {
        LOG_ERROR( "Failed to instanciate a folder for mrl: ", mrl, ": ", ex.what() );
        return nullptr;
    }

    auto deviceFs = folderFs->device();
    if ( deviceFs == nullptr )
    {
        LOG_ERROR( "Failed to get device containing an existing folder: ", folderFs->mrl() );
        return nullptr;
    }
    if ( deviceFs->isRemovable() == false )
    {
        std::string req = "SELECT * FROM " + policy::FolderTable::Name + " WHERE path = ? AND is_removable = 0";
        if ( bannedType == BannedType::Any )
            return fetch( ml, req, folderFs->mrl() );
        req += " AND is_blacklisted = ?";
        return fetch( ml, req, folderFs->mrl(), bannedType == BannedType::Yes ? true : false );
    }

    auto device = Device::fromUuid( ml, deviceFs->uuid() );
    // We are trying to find a folder. If we don't know the device it's on, we don't know the folder.
    if ( device == nullptr )
        return nullptr;
    auto path = utils::file::removePath( folderFs->mrl(), deviceFs->mountpoint() );
    std::string req = "SELECT * FROM " + policy::FolderTable::Name + " WHERE path = ? AND device_id = ?";
    std::shared_ptr<Folder> folder;
    if ( bannedType == BannedType::Any )
    {
        folder = fetch( ml, req, path, device->id() );
    }
    else
    {
        req += " AND is_blacklisted = ?";
        folder = fetch( ml, req, path, device->id(), bannedType == BannedType::Yes ? true : false );
    }
    if ( folder == nullptr )
        return nullptr;
    folder->m_deviceMountpoint = deviceFs->mountpoint();
    folder->m_fullPath = folder->m_deviceMountpoint.get() + path;
    return folder;
}

int64_t Folder::id() const
{
    return m_id;
}

const std::string& Folder::mrl() const
{
    if ( m_isRemovable == false )
        return m_path;

    auto lock = m_deviceMountpoint.lock();
    if ( m_deviceMountpoint.isCached() == true )
        return m_fullPath;

    // We can't compute the full path of a folder if it's removable and the device isn't present.
    // When there's no device, we don't know the mountpoint, therefor we don't know the full path
    // Calling isPresent will ensure we have the device representation cached locally
    if ( isPresent() == false )
    {
        assert( !"Device isn't present" );
        m_fullPath = "";
        return m_fullPath;
    }

    auto fsFactory = m_ml->fsFactoryForMrl( m_device.get()->scheme() );
    assert( fsFactory != nullptr );
    auto deviceFs = fsFactory->createDevice( m_device.get()->uuid() );
    // In case the device lister hasn't been updated accordingly, we might think
    // a device still is present while it's not.
    if( deviceFs == nullptr )
    {
        assert( !"File system Device representation couldn't be found" );
        m_fullPath = "";
        return m_fullPath;
    }
    m_deviceMountpoint = deviceFs->mountpoint();
    m_fullPath = m_deviceMountpoint.get() + m_path;
    return m_fullPath;
}

const std::string& Folder::rawMrl() const
{
    return m_path;
}

void Folder::setMrl( std::string mrl )
{
    if ( m_path == mrl )
        return;
    static const std::string req = "UPDATE " + policy::FolderTable::Name + " SET "
            "path = ? WHERE id_folder = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, mrl, m_id ) == false )
        return;
    // We shouldn't use this if any full path/mrl has been cached.
    // This is meant for migration only, so there is no need to have cached this
    // information so far.
    assert( m_isRemovable == false || m_fullPath.empty() == true );
    m_path = std::move( mrl );
}

std::vector<std::shared_ptr<File>> Folder::files()
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name +
        " WHERE folder_id = ?";
    return File::fetchAll<File>( m_ml, req, m_id );
}

std::vector<std::shared_ptr<Folder>> Folder::folders()
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE parent_id = ? AND is_blacklisted = 0 AND is_present != 0";
    return DatabaseHelpers::fetchAll<Folder>( m_ml, req, m_id );
}

std::shared_ptr<Folder> Folder::parent()
{
    return fetch( m_ml, m_parent );
}

int64_t Folder::deviceId() const
{
    return m_deviceId;
}

bool Folder::isPresent() const
{
    auto deviceLock = m_device.lock();
    if ( m_device.isCached() == false )
        m_device = Device::fetch( m_ml, m_deviceId );
    // There must be a device containing the folder, since we never create a folder
    // without a device
    assert( m_device.get() != nullptr );
    // However, handle potential sporadic errors gracefully
    if( m_device.get() == nullptr )
        return false;
    return m_device.get()->isPresent();
}

bool Folder::isBanned() const
{
    return m_isBlacklisted;
}

bool Folder::isRootFolder() const
{
    return m_parent == 0;
}

std::vector<std::shared_ptr<Folder>> Folder::fetchRootFolders( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name +
            " LEFT JOIN ExcludedEntryFolder"
            " ON " + policy::FolderTable::Name + ".id_folder = ExcludedEntryFolder.folder_id"
            " WHERE ExcludedEntryFolder.folder_id IS NULL AND"
            " parent_id IS NULL AND is_blacklisted = 0 AND is_present != 0";
    return DatabaseHelpers::fetchAll<Folder>( ml, req );
}

}
