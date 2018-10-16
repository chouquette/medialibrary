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
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "utils/Filename.h"
#include "utils/Url.h"

#include <unordered_map>

namespace medialibrary
{

const std::string Folder::Table::Name = "Folder";
const std::string Folder::Table::PrimaryKeyColumn = "id_folder";
int64_t Folder::* const Folder::Table::PrimaryKey = &Folder::m_id;

Folder::Folder( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.load<decltype(m_id)>( 0 ) )
    , m_path( row.load<decltype(m_path)>( 1 ) )
    , m_name( row.load<decltype(m_name)>( 2 ) )
    , m_parent( row.load<decltype(m_parent)>( 3 ) )
    , m_isBanned( row.load<decltype(m_isBanned)>( 4 ) )
    , m_deviceId( row.load<decltype(m_deviceId)>( 5 ) )
    , m_isRemovable( row.load<decltype(m_isRemovable)>( 6 ) )
    , m_nbMedia( row.load<decltype(m_nbMedia)>( 7 ) )
{
}

Folder::Folder(MediaLibraryPtr ml, const std::string& path,
               int64_t parent, int64_t deviceId, bool isRemovable )
    : m_ml( ml )
    , m_id( 0 )
    , m_path( path )
    , m_name( utils::url::decode( utils::file::directoryName( path ) ) )
    , m_parent( parent )
    , m_isBanned( false )
    , m_deviceId( deviceId )
    , m_isRemovable( isRemovable )
{
}

void Folder::createTable( sqlite::Connection* connection)
{
    const std::string reqs[] = {
        #include "database/tables/Folder_v14.sql"
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( connection, req );
}

void Folder::createTriggers( sqlite::Connection* connection, uint32_t modelVersion )
{
    const std::string reqs[] = {
        #include "database/tables/Folder_triggers_v14.sql"
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( connection, req );
    if ( modelVersion >= 14 )
    {
        const std::string v14Reqs[] = {
            "CREATE TRIGGER IF NOT EXISTS update_folder_nb_media_on_insert "
                "AFTER INSERT ON " + Media::Table::Name + " "
                "WHEN new.folder_id IS NOT NULL "
            "BEGIN "
                "UPDATE " + Folder::Table::Name + " SET nb_media = nb_media + 1 "
                    "WHERE id_folder = new.folder_id;"
            "END",

            "CREATE TRIGGER IF NOT EXISTS update_folder_nb_media_on_delete "
                "AFTER DELETE ON " + Media::Table::Name + " "
                "WHEN old.folder_id IS NOT NULL "
            "BEGIN "
                "UPDATE " + Folder::Table::Name + " SET nb_media = nb_media - 1 "
                    "WHERE id_folder = old.folder_id;"
            "END",
        };
        for ( const auto& req : v14Reqs )
            sqlite::Tools::executeRequest( connection, req );
    }
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
    static const std::string req = "INSERT INTO " + Folder::Table::Name +
            "(path, name, parent_id, device_id, is_removable) VALUES(?, ?, ?, ?, ?)";
    if ( insert( ml, self, req, path, self->m_name, sqlite::ForeignKey( parentId ), device.id(), device.isRemovable() ) == false )
        return nullptr;
    if ( device.isRemovable() == true )
    {
        self->m_deviceMountpoint = deviceFs.mountpoint();
        self->m_fullPath = self->m_deviceMountpoint + path;
    }
    return self;
}

void Folder::excludeEntryFolder( MediaLibraryPtr ml, int64_t folderId )
{
    std::string req = "INSERT INTO ExcludedEntryFolder(folder_id) VALUES(?)";
    sqlite::Tools::executeRequest( ml->getConn(), req, folderId );
}

bool Folder::ban( MediaLibraryPtr ml, const std::string& mrl )
{
    // Ensure we delete the existing folder if any & ban the folder in an "atomic" way
    return sqlite::Tools::withRetries( 3, [ml, &mrl]() {
        auto t = ml->getConn()->newTransaction();

        auto f = fromMrl( ml, mrl, BannedType::Any );
        if ( f != nullptr )
        {
            // No need to ban a folder twice
            if ( f->m_isBanned == true )
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
        static const std::string req = "INSERT INTO " + Folder::Table::Name +
                "(path, parent_id, is_banned, device_id, is_removable) VALUES(?, ?, ?, ?, ?)";
        auto res = sqlite::Tools::executeInsert( ml->getConn(), req, path, nullptr, true, device->id(), deviceFs->isRemovable() ) != 0;
        t->commit();
        return res;
    });
}

std::shared_ptr<Folder> Folder::fromMrl( MediaLibraryPtr ml, const std::string& mrl )
{
    return fromMrl( ml, mrl, BannedType::No );
}

std::shared_ptr<Folder> Folder::bannedFolder( MediaLibraryPtr ml, const std::string& mrl )
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
        std::string req = "SELECT * FROM " + Folder::Table::Name + " WHERE path = ? AND is_removable = 0";
        if ( bannedType == BannedType::Any )
            return fetch( ml, req, folderFs->mrl() );
        req += " AND is_banned = ?";
        return fetch( ml, req, folderFs->mrl(), bannedType == BannedType::Yes ? true : false );
    }

    auto device = Device::fromUuid( ml, deviceFs->uuid() );
    // We are trying to find a folder. If we don't know the device it's on, we don't know the folder.
    if ( device == nullptr )
        return nullptr;
    auto path = utils::file::removePath( folderFs->mrl(), deviceFs->mountpoint() );
    std::string req = "SELECT * FROM " + Folder::Table::Name + " WHERE path = ? AND device_id = ?";
    std::shared_ptr<Folder> folder;
    if ( bannedType == BannedType::Any )
    {
        folder = fetch( ml, req, path, device->id() );
    }
    else
    {
        req += " AND is_banned = ?";
        folder = fetch( ml, req, path, device->id(), bannedType == BannedType::Yes ? true : false );
    }
    if ( folder == nullptr )
        return nullptr;
    folder->m_deviceMountpoint = deviceFs->mountpoint();
    folder->m_fullPath = folder->m_deviceMountpoint + path;
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

    if ( m_fullPath.empty() == false )
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

    auto fsFactory = m_ml->fsFactoryForMrl( m_device->scheme() );
    assert( fsFactory != nullptr );
    auto deviceFs = fsFactory->createDevice( m_device->uuid() );
    // In case the device lister hasn't been updated accordingly, we might think
    // a device still is present while it's not.
    if( deviceFs == nullptr )
    {
        assert( !"File system Device representation couldn't be found" );
        m_fullPath = "";
        return m_fullPath;
    }
    m_deviceMountpoint = deviceFs->mountpoint();
    m_fullPath = m_deviceMountpoint + m_path;
    return m_fullPath;
}

const std::string&Folder::name() const
{
    return m_name;
}

void Folder::setName( std::string name )
{
    assert( m_name.empty() == true );
    static const std::string req = "UPDATE " + Table::Name +
            " SET name = ? WHERE id_folder = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, name, m_id ) == false )
        return;
    m_name = std::move( name );
}

const std::string& Folder::rawMrl() const
{
    return m_path;
}

void Folder::setMrl( std::string mrl )
{
    if ( m_path == mrl )
        return;
    static const std::string req = "UPDATE " + Folder::Table::Name + " SET "
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
    static const std::string req = "SELECT * FROM " + File::Table::Name +
        " WHERE folder_id = ?";
    return File::fetchAll<File>( m_ml, req, m_id );
}

std::vector<std::shared_ptr<Folder>> Folder::folders()
{
    static const std::string req = "SELECT * FROM " + Folder::Table::Name + " f "
            " LEFT JOIN " + Device::Table::Name + " d ON d.id_device = f.device_id"
            " WHERE parent_id = ? AND is_banned = 0 AND d.is_present != 0";
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
    if ( m_device == nullptr )
        m_device = Device::fetch( m_ml, m_deviceId );
    // There must be a device containing the folder, since we never create a folder
    // without a device
    assert( m_device != nullptr );
    // However, handle potential sporadic errors gracefully
    if( m_device == nullptr )
        return false;
    return m_device->isPresent();
}

bool Folder::isBanned() const
{
    return m_isBanned;
}

bool Folder::isRootFolder() const
{
    return m_parent == 0;
}

uint32_t Folder::nbMedia() const
{
    return m_nbMedia;
}

std::vector<std::shared_ptr<Folder>> Folder::fetchRootFolders( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT * FROM " + Folder::Table::Name + " f "
            " LEFT JOIN ExcludedEntryFolder"
            " ON f.id_folder = ExcludedEntryFolder.folder_id"
            " LEFT JOIN " + Device::Table::Name + " d ON d.id_device = f.device_id"
            " WHERE ExcludedEntryFolder.folder_id IS NULL AND"
            " parent_id IS NULL AND is_banned = 0 AND d.is_present != 0";
    return DatabaseHelpers::fetchAll<Folder>( ml, req );
}

}
