/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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
#include "database/SqliteQuery.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "medialibrary/filesystem/Errors.h"
#include "utils/Filename.h"
#include "utils/Url.h"

namespace medialibrary
{

const std::string Folder::Table::Name = "Folder";
const std::string Folder::Table::PrimaryKeyColumn = "id_folder";
int64_t Folder::* const Folder::Table::PrimaryKey = &Folder::m_id;
const std::string Folder::FtsTable::Name = "FolderFts";
const std::string Folder::ExcludedFolderTable::Name = "ExcludedEntryFolder";

Folder::Folder( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_path( row.extract<decltype(m_path)>() )
    , m_name( row.extract<decltype(m_name)>() )
    , m_parent( row.extract<decltype(m_parent)>() )
    , m_isBanned( row.extract<decltype(m_isBanned)>() )
    , m_deviceId( row.extract<decltype(m_deviceId)>() )
    , m_isRemovable( row.extract<decltype(m_isRemovable)>() )
    , m_nbAudio( row.extract<decltype(m_nbAudio)>() )
    , m_nbVideo( row.extract<decltype(m_nbVideo)>() )
{
    assert( row.hasRemainingColumns() == false );
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
    , m_nbAudio( 0 )
    , m_nbVideo( 0 )
{
}

void Folder::createTable( sqlite::Connection* connection)
{
    const std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
        schema( ExcludedFolderTable::Name, Settings::DbModelVersion ),
        schema( FtsTable::Name, Settings::DbModelVersion ),
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( connection, req );
}

void Folder::createTriggers( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::InsertFts, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::DeleteFts, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::UpdateNbMediaOnIndex,
                                            Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::UpdateNbMediaOnDelete,
                                            Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::UpdateNbMediaOnUpdate,
                                            Settings::DbModelVersion ) );
}

void Folder::createIndexes( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::DeviceId, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::ParentId, Settings::DbModelVersion ) );
}

std::string Folder::schema( const std::string& tableName, uint32_t dbModel )
{
    if ( tableName == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE " + FtsTable::Name +
                " USING FTS3(name)";
    }
    else if ( tableName == ExcludedFolderTable::Name )
    {
        return "CREATE TABLE " + ExcludedFolderTable::Name +
        "("
            "folder_id UNSIGNED INTEGER NOT NULL,"

            "FOREIGN KEY(folder_id) REFERENCES " + Table::Name +
            "(id_folder) ON DELETE CASCADE,"

            "UNIQUE(folder_id) ON CONFLICT FAIL"
        ")";
    }
    assert( tableName == Table::Name );
    return "CREATE TABLE " + Table::Name +
    "("
        "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
        "path TEXT,"
        "name TEXT" + (dbModel >= 15 ? " COLLATE NOCASE" : "") + ","
        "parent_id UNSIGNED INTEGER,"
        "is_banned BOOLEAN NOT NULL DEFAULT 0,"
        "device_id UNSIGNED INTEGER,"
        "is_removable BOOLEAN NOT NULL,"
        "nb_audio UNSIGNED INTEGER NOT NULL DEFAULT 0,"
        "nb_video UNSIGNED INTEGER NOT NULL DEFAULT 0,"

        "FOREIGN KEY(parent_id) REFERENCES " + Table::Name +
        "(id_folder) ON DELETE CASCADE,"

        "FOREIGN KEY(device_id) REFERENCES " + Device::Table::Name +
        "(id_device) ON DELETE CASCADE,"

        "UNIQUE(path,device_id) ON CONFLICT FAIL"
        ")";
}

std::string Folder::trigger( Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::InsertFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER INSERT ON " + Table::Name + " "
                   "BEGIN "
                       "INSERT INTO " + FtsTable::Name + "(rowid,name) "
                           "VALUES(new.id_folder,new.name);"
                   "END";
        case Triggers::DeleteFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " BEFORE DELETE ON " + Table::Name + " "
                   "BEGIN "
                       "DELETE FROM " + FtsTable::Name +
                            " WHERE rowid = old.id_folder;"
                   "END";
        case Triggers::UpdateNbMediaOnIndex:
            assert( dbModel >= 14 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER INSERT ON " + Media::Table::Name +
                        " WHEN new.folder_id IS NOT NULL "
                    "BEGIN "
                        "UPDATE " + Table::Name + " SET "
                        "nb_audio = nb_audio + "
                            "(CASE new.type "
                                "WHEN " +
                                    std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                        IMedia::Type::Audio ) ) + " THEN 1 "
                                "ELSE 0 "
                            "END),"
                        "nb_video = nb_video + "
                            "(CASE new.type WHEN " +
                                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                    IMedia::Type::Video ) ) + " THEN 1 "
                                "ELSE 0 "
                            "END) "
                        "WHERE id_folder = new.folder_id;"
                    "END";
        case Triggers::UpdateNbMediaOnUpdate:
            assert( dbModel >= 14 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                       " AFTER UPDATE ON " + Media::Table::Name +
                       " WHEN new.folder_id IS NOT NULL AND old.type != new.type "
                   "BEGIN "
                       "UPDATE " + Table::Name + " SET "
                       "nb_audio = nb_audio + "
                           "(CASE old.type "
                               "WHEN " +
                                   std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                   IMedia::Type::Audio ) ) + " THEN -1 "
                               "ELSE 0 "
                           "END)"
                           "+"
                           "(CASE new.type "
                               "WHEN " +
                                   std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                   IMedia::Type::Audio ) ) + " THEN 1 "
                               "ELSE 0 "
                           "END)"
                       ","
                       "nb_video = nb_video + "
                           "(CASE old.type "
                               "WHEN " +
                                   std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                       IMedia::Type::Video ) ) + " THEN -1 "
                               "ELSE 0 "
                           "END)"
                           "+"
                           "(CASE new.type "
                               "WHEN " +
                                   std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                       IMedia::Type::Video ) ) + " THEN 1 "
                               "ELSE 0 "
                           "END)"
                       "WHERE id_folder = new.folder_id;"
                   "END";
        case Triggers::UpdateNbMediaOnDelete:
            assert( dbModel >= 14 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER DELETE ON " + Media::Table::Name +
                        " WHEN old.folder_id IS NOT NULL "
                    "BEGIN "
                        "UPDATE " + Table::Name + " SET "
                        "nb_audio = nb_audio + "
                            "(CASE old.type "
                                "WHEN " +
                                    std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                        IMedia::Type::Audio ) ) + " THEN -1 "
                                "ELSE 0 "
                            "END),"
                        "nb_video = nb_video + "
                            "(CASE old.type "
                                "WHEN " +
                                    std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                        IMedia::Type::Video ) ) + " THEN -1 "
                                "ELSE 0 "
                            "END) "
                        "WHERE id_folder = old.folder_id;"
                    "END";

        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Folder::triggerName( Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::InsertFts:
            return "insert_folder_fts";
        case Triggers::DeleteFts:
            return "delete_folder_fts";
        case Triggers::UpdateNbMediaOnIndex:
            assert( dbModel >= 14 );
            return "update_folder_nb_media_on_insert";
        case Triggers::UpdateNbMediaOnUpdate:
            assert( dbModel >= 14 );
            return "update_folder_nb_media_on_update";
        case Triggers::UpdateNbMediaOnDelete:
            assert( dbModel >= 14 );
            return "update_folder_nb_media_on_delete";
    default:
        assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Folder::index( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::DeviceId:
            return "CREATE INDEX " + indexName( index, dbModel ) + " ON " +
                        Table::Name + " (device_id)";
        case Indexes::ParentId:
            return "CREATE INDEX " + indexName( index, dbModel ) + " ON " +
                        Table::Name + " (parent_id)";
        default:
            assert( !"Invalid index provided" );
    }
    return "<invalid request>";
}

std::string Folder::indexName( Indexes index, uint32_t )
{
    switch ( index )
    {
        case Indexes::DeviceId:
            return "folder_device_id_idx";
        case Indexes::ParentId:
            return "parent_folder_id_idx";
        default:
            assert( !"Invalid index provided" );
    }
    return "<invalid request>";
}

bool Folder::checkDbModel( MediaLibraryPtr ml )
{
    if ( sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) == false ||
        sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( FtsTable::Name, Settings::DbModelVersion ),
                                       FtsTable::Name ) == false ||
        sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( ExcludedFolderTable::Name, Settings::DbModelVersion ),
                                       ExcludedFolderTable::Name ) == false )
        return false;

    auto check = []( sqlite::Connection* dbConn, Triggers t ) {
        return sqlite::Tools::checkTriggerStatement( dbConn,
                                    trigger( t, Settings::DbModelVersion ),
                                    triggerName( t, Settings::DbModelVersion ) );
    };
    if ( check( ml->getConn(), Triggers::InsertFts ) == false ||
         check( ml->getConn(), Triggers::DeleteFts ) == false ||
         check( ml->getConn(), Triggers::UpdateNbMediaOnIndex ) == false ||
         check( ml->getConn(), Triggers::UpdateNbMediaOnUpdate ) == false ||
         check( ml->getConn(), Triggers::UpdateNbMediaOnDelete )  == false )
        return false;

    return sqlite::Tools::checkIndexStatement( ml->getConn(),
                index( Indexes::DeviceId, Settings::DbModelVersion ),
                indexName( Indexes::DeviceId, Settings::DbModelVersion ) ) &&
           sqlite::Tools::checkIndexStatement( ml->getConn(),
                index( Indexes::ParentId, Settings::DbModelVersion ),
                indexName( Indexes::ParentId, Settings::DbModelVersion ) );
}

std::shared_ptr<Folder> Folder::create( MediaLibraryPtr ml, const std::string& mrl,
                                        int64_t parentId, const Device& device,
                                        fs::IDevice& deviceFs )
{
    std::string path;
    if ( device.isRemovable() == true )
        path = deviceFs.relativeMrl( mrl );
    else
        path = mrl;
    auto self = std::make_shared<Folder>( ml, path, parentId, device.id(),
                                          deviceFs.isRemovable() );
    static const std::string req = "INSERT INTO " + Folder::Table::Name +
            "(path, name, parent_id, device_id, is_removable) VALUES(?, ?, ?, ?, ?)";
    if ( insert( ml, self, req, path, self->m_name, sqlite::ForeignKey( parentId ),
                 device.id(), deviceFs.isRemovable() ) == false )
        return nullptr;
    if ( device.isRemovable() == true )
        self->m_fullPath = deviceFs.absoluteMrl( path );
    return self;
}

bool Folder::excludeEntryFolder( MediaLibraryPtr ml, int64_t folderId )
{
    std::string req = "INSERT INTO " + ExcludedFolderTable::Name +
            "(folder_id) VALUES(?)";
    return sqlite::Tools::executeInsert( ml->getConn(), req, folderId ) != 0;
}

bool Folder::ban( MediaLibraryPtr ml, const std::string& mrl )
{
    // Ensure we delete the existing folder if any & ban the folder in an "atomic" way
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
    catch ( const fs::errors::System& ex )
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
    auto device = Device::fromUuid( ml, deviceFs->uuid(), fsFactory->scheme() );
    if ( device == nullptr )
        device = Device::create( ml, deviceFs->uuid(),
                                 utils::url::scheme( mrl ),
                                 deviceFs->isRemovable(),
                                 deviceFs->isNetwork() );
    std::string path;
    if ( deviceFs->isRemovable() == true )
        path = deviceFs->relativeMrl( mrl );
    else
        path = mrl;
    static const std::string req = "INSERT INTO " + Folder::Table::Name +
            "(path, parent_id, is_banned, device_id, is_removable) "
            "VALUES(?, ?, ?, ?, ?)";
    auto res = sqlite::Tools::executeInsert( ml->getConn(), req, path,
                                             nullptr, true, device->id(),
                                             deviceFs->isRemovable() ) != 0;
    t->commit();
    return res;
}

std::shared_ptr<Folder> Folder::fromMrl( MediaLibraryPtr ml, const std::string& mrl )
{
    return fromMrl( ml, mrl, BannedType::No );
}

std::shared_ptr<Folder> Folder::bannedFolder( MediaLibraryPtr ml, const std::string& mrl )
{
    return fromMrl( ml, mrl, BannedType::Yes );
}

std::shared_ptr<Folder> Folder::fromMrl( MediaLibraryPtr ml, const std::string& mrl,
                                         BannedType bannedType )
{
    if ( mrl.empty() == true )
        return nullptr;
    auto fsFactory = ml->fsFactoryForMrl( mrl );
    if ( fsFactory == nullptr )
        return nullptr;
    std::shared_ptr<fs::IDevice> deviceFs;
    std::shared_ptr<fs::IDirectory> folderFs;
    try
    {
        /*
         * It's ok to instanciate a fs::IFolder even though the fs factories are
         * not started, since no actual FS interraction will happen by doing so.
         * This allows us to use the sanitized mrl that's returned
         * by fs::IFolder::mrl() (decoded & reencoded to ensure it matches our
         * encoding scheme)
         */
        folderFs = fsFactory->createDirectory( mrl );
    }
    catch ( const fs::errors::System& ex )
    {
        LOG_ERROR( "Failed to instanciate a folder for mrl: ", mrl, ": ",
                   ex.what() );
        return nullptr;
    }
    if ( fsFactory->isStarted() == true )
    {
        /* If the fs factory is started, we can probe the devices it knows */
        deviceFs = folderFs->device();
        if ( deviceFs == nullptr )
        {
            LOG_WARN( "Failed to get device containing an existing folder: ",
                      folderFs->mrl() );
            return nullptr;
        }
    }

    int64_t deviceId;
    std::string path;

    if ( deviceFs != nullptr )
    {
        if ( deviceFs->isRemovable() == false )
        {
            std::string req = "SELECT * FROM " + Folder::Table::Name +
                              " WHERE path = ? AND is_removable = 0";
            if ( bannedType == BannedType::Any )
                return fetch( ml, req, folderFs->mrl() );
            req += " AND is_banned = ?";
            return fetch( ml, req, folderFs->mrl(),
                          bannedType == BannedType::Yes ? true : false );
        }
        auto device = Device::fromUuid( ml, deviceFs->uuid(), fsFactory->scheme() );
        // We are trying to find a folder. If we don't know the device it's on, we don't know the folder.
        if ( device == nullptr )
            return nullptr;

        path = deviceFs->relativeMrl( folderFs->mrl() );
        deviceId = device->id();
    }
    else
    {
        /*
         * If it's not started, or if the device was unknown, we can try to
         * probe the previously known mountpoints that were stored in database
         */
        auto deviceTuple = Device::fromMountpoint( ml, mrl );
        deviceId = std::get<0>( deviceTuple );
        if ( deviceId == 0 )
            return nullptr;
        path = utils::file::removePath( mrl, std::get<1>( deviceTuple ) );
    }

    std::string req = "SELECT * FROM " + Folder::Table::Name +
                      " WHERE path = ? AND device_id = ?";
    std::shared_ptr<Folder> folder;
    if ( bannedType == BannedType::Any )
    {
        folder = fetch( ml, req, path, deviceId );
    }
    else
    {
        req += " AND is_banned = ?";
        folder = fetch( ml, req, path, deviceId,
                        bannedType == BannedType::Yes ? true : false );
    }
    if ( folder == nullptr )
        return nullptr;
    folder->m_fullPath = deviceFs != nullptr ? deviceFs->absoluteMrl( path ) : mrl;
    return folder;
}

std::string Folder::sortRequest( const QueryParameters* params )
{
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    std::string req = "ORDER BY ";
    switch ( sort )
    {
        case SortingCriteria::NbVideo:
            req += "nb_video";
            desc = !desc;
            break;
        case SortingCriteria::NbAudio:
            req += "nb_audio";
            desc = !desc;
            break;
        case SortingCriteria::NbMedia:
            req += "(nb_audio + nb_video)";
            desc = !desc;
            break;
        default:
            LOG_WARN( "Unsupported sorting criteria, falling back to Default (alpha)" );
            /* fall-through */
        case SortingCriteria::Default:
        case SortingCriteria::Alpha:
            req += "name";
    }
    if ( desc == true )
        req += " DESC";
    return req;
}

std::string Folder::filterByMediaType( IMedia::Type type )
{
    switch ( type )
    {
        case IMedia::Type::Audio:
            return " f.nb_audio > 0";
        case IMedia::Type::Video:
            return " f.nb_video > 0";
        default:
            assert( !"Only Audio/Video/Unknown types are supported when listing folders" );
            /* Fall-through */
        case IMedia::Type::Unknown:
            return " (f.nb_audio > 0 OR f.nb_video > 0)";
    }
}

std::shared_ptr<Device> Folder::device() const
{
    if ( m_device == nullptr )
    {
        m_device = Device::fetch( m_ml, m_deviceId );
        // There must be a device containing the folder, since we never create a folder
        // without a device
        assert( m_device != nullptr );
    }
    return m_device;
}

Query<IFolder> Folder::withMedia( MediaLibraryPtr ml, IMedia::Type type,
                                  const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " f "
                      " LEFT JOIN " + Device::Table::Name +
                            " d ON d.id_device = f.device_id "
                      " WHERE " + filterByMediaType( type ) +
                            " AND d.is_present != 0";
    return make_query<Folder, IFolder>( ml, "f.*", req, sortRequest( params ) );
}

Query<IFolder> Folder::searchWithMedia( MediaLibraryPtr ml,
                                        const std::string& pattern,
                                        IMedia::Type type,
                                        const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " f "
            " LEFT JOIN " + Device::Table::Name +
                " d ON d.id_device = f.device_id "
            "WHERE f.id_folder IN (SELECT rowid FROM " + FtsTable::Name + " WHERE " +
                FtsTable::Name + " MATCH ?) "
            "AND d.is_present != 0 AND " + filterByMediaType( type );
    return make_query<Folder, IFolder>( ml, "f.*", req, sortRequest( params ),
                                        sqlite::Tools::sanitizePattern( pattern ) );
}

Query<IFolder> Folder::entryPoints( MediaLibraryPtr ml, bool banned, int64_t deviceId )
{
    std::string req = "FROM " + Folder::Table::Name + " WHERE parent_id IS NULL"
            " AND is_banned = ?";
    if ( deviceId == 0 )
        return make_query<Folder, IFolder>( ml, "*", req, "", banned );
    req += " AND device_id = ?";
    return make_query<Folder, IFolder>( ml, "*", req, "", banned, deviceId );
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

    /*
     * We need the device entity to know its scheme, in order to fetch the
     * fs factory associated with it
     */
    auto d = device();
    if ( d == nullptr )
        throw fs::errors::DeviceRemoved{};

    /* Fetch and start the fs factory if required */
    auto fsFactory = m_ml->fsFactoryForMrl( m_device->scheme() );
    if ( fsFactory == nullptr )
        throw fs::errors::UnknownScheme{ m_device->scheme() };
    if ( fsFactory->isStarted() == false )
    {
        /*
         * Starting the factory will refresh its device list. This is synchronous
         * for local devices, and asynchronous for network devices.
         * For network devices, we'll try to rely on a previously seen mountpoint
         * in the likely case that the factory doesn't refresh the devices we're
         * about to probe
         */
        m_ml->startFsFactory( *fsFactory );
    }

    // We can't compute the full path of a folder if it's removable and the
    // device isn't present. When there's no device, we don't know the
    // mountpoint, therefor we don't know the full path. Calling isPresent will
    // ensure we have the device representation cached locally
    if ( isPresent() == false )
    {
        if ( d->isNetwork() == true )
        {
            auto mountpoint = d->cachedMountpoint();
            if ( mountpoint.empty() == false )
            {
                m_fullPath = mountpoint + m_path;
                return m_fullPath;
            }
        }
        throw fs::errors::DeviceRemoved{};
    }


    auto deviceFs = fsFactory->createDevice( m_device->uuid() );
    // In case the device lister hasn't been updated accordingly, we might think
    // a device still is present while it's not.
    if( deviceFs == nullptr )
    {
        // We only checked for the database representation so far. If the device
        // representation in DB was not updated but we can't find the device, we
        // should still assume that the device was removed
        throw fs::errors::DeviceRemoved{};
    }
    m_fullPath = deviceFs->absoluteMrl( m_path );
    return m_fullPath;
}

const std::string&Folder::name() const
{
    if ( m_isRemovable == true && m_name.empty() == true )
    {
        // In case this is the root folder of an external device, we don't have
        // any information before knowing the actual mountpoint, so we have to do
        // this at runtime
        auto fullPath = mrl();
        m_name = utils::url::decode( utils::file::directoryName( fullPath ) );
    }
    return m_name;
}

void Folder::setName( std::string name )
{
    assert( m_name.empty() == true );
    static const std::string req = "UPDATE " + Table::Name +
            " SET name = ? WHERE id_folder = ?";
    auto dbConn = m_ml->getConn();
    if ( sqlite::Tools::executeUpdate( dbConn, req, name, m_id ) == false )
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
    static const std::string req = "SELECT f.* FROM " + Folder::Table::Name + " f "
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

bool Folder::isRemovable() const
{
    return m_isRemovable;
}

bool Folder::forceNonRemovable( const std::string& fullMrl )
{
    assert( sqlite::Transaction::transactionInProgress() == true );
    LOG_INFO( "Fixin up: mrl:", m_path, " -> ", fullMrl );

    const std::string req = "UPDATE " + Table::Name +
            " SET path = ?, is_removable = ? WHERE id_folder = ?";
    auto removeFolder = false;
    try
    {
        // Don't check the return value as some updates might fail, because we
        // removed the parents folder before firing the update itself
        if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, fullMrl, false,
                                           m_id ) == false )
            return false;
    }
    catch ( sqlite::errors::ConstraintViolation& )
    {
        // Assume this was a banned folder which we erroneously added.
        removeFolder = true;
    }
    // Don't run the deletion from an exception handler
    if ( removeFolder == true )
    {
        destroy( m_ml, m_id );
        return true;
    }
    m_fullPath = fullMrl;
    m_path = fullMrl;
    m_isRemovable = false;
    return true;
}

bool Folder::isPresent() const
{
    auto d = device();
    if( d == nullptr )
        return false;
    return d->isPresent();
}

bool Folder::isBanned() const
{
    return m_isBanned;
}

bool Folder::isRootFolder() const
{
    return m_parent == 0;
}

Query<IMedia> Folder::media( IMedia::Type type, const QueryParameters* params ) const
{
    return Media::fromFolderId( m_ml, type, m_id, params );
}

Query<IMedia> Folder::searchMedia( const std::string& pattern, IMedia::Type type,
                                   const QueryParameters* params ) const
{
    if ( pattern.size() < 3 )
        return {};
    return Media::searchFromFolderId( m_ml, pattern, type, m_id, params );
}

Query<IFolder> Folder::subfolders( const QueryParameters* params ) const
{
    static const std::string req = "FROM " + Table::Name + " WHERE parent_id = ?";
    return make_query<Folder, IFolder>( m_ml, "*", req, sortRequest( params ), m_id );
}

uint32_t Folder::nbVideo() const
{
    return m_nbVideo;
}

uint32_t Folder::nbAudio() const
{
    return m_nbAudio;
}

uint32_t Folder::nbMedia() const
{
    return m_nbAudio + m_nbVideo;
}

std::vector<std::shared_ptr<Folder>> Folder::fetchRootFolders( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT f.* FROM " + Folder::Table::Name + " f "
            " LEFT JOIN " + ExcludedFolderTable::Name +
            " ON f.id_folder = " + ExcludedFolderTable::Name + ".folder_id"
            " LEFT JOIN " + Device::Table::Name + " d ON d.id_device = f.device_id"
            " WHERE " + ExcludedFolderTable::Name + ".folder_id IS NULL AND"
            " parent_id IS NULL AND is_banned = 0 AND d.is_present != 0";
    return DatabaseHelpers::fetchAll<Folder>( ml, req );
}

}
