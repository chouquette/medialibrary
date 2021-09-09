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

#include "Device.h"
#include "Folder.h"
#include "utils/Filename.h"
#include "utils/Url.h"

#include <strings.h>

namespace medialibrary
{

const std::string Device::Table::Name = "Device";
const std::string Device::Table::PrimaryKeyColumn = "id_device";
int64_t Device::* const Device::Table::PrimaryKey = &Device::m_id;
const std::string Device::MountpointTable::Name = "DeviceMountpoint";

Device::Device( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_uuid( row.extract<decltype(m_uuid)>() )
    , m_scheme( row.extract<decltype(m_scheme)>() )
    , m_isRemovable( row.extract<decltype(m_isRemovable)>() )
    , m_isPresent( row.extract<decltype(m_isPresent)>() )
    , m_isNetwork( row.extract<decltype(m_isNetwork)>() )
{
#ifndef NDEBUG
    auto lastSeen = row.extract<int64_t>();
    assert( row.hasRemainingColumns() == false );
    assert( lastSeen != 0 || m_isRemovable == false );
#endif
}

Device::Device( MediaLibraryPtr ml, const std::string& uuid,
                const std::string& scheme, bool isRemovable, bool isNetwork )
    : m_ml( ml )
    , m_id( 0 )
    , m_uuid( uuid )
    , m_scheme( scheme )
    , m_isRemovable( isRemovable )
    // Assume we can't add an absent device
    , m_isPresent( true )
    , m_isNetwork( isNetwork )
{
}

int64_t Device::id() const
{
    return m_id;
}

const std::string&Device::uuid() const
{
    return m_uuid;
}

bool Device::isRemovable() const
{
    return m_isRemovable;
}

bool Device::isPresent() const
{
    return m_isPresent;
}

void Device::setPresent(bool value)
{
    if ( m_isRemovable == false )
    {
        assert( !"Can't change the presence of a non-removable device" );
        return;
    }
    assert( m_isPresent != value );
    static const std::string req = "UPDATE " + Device::Table::Name +
            " SET is_present = ? WHERE id_device = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, value, m_id ) == false )
        return;
    m_isPresent = value;
}

bool Device::isNetwork() const
{
    return m_isNetwork;
}

const std::string& Device::scheme() const
{
    return m_scheme;
}

void Device::updateLastSeen()
{
    assert( m_isRemovable == true );
    const std::string req = "UPDATE " + Device::Table::Name + " SET "
            "last_seen = ? WHERE id_device = ?";
    auto lastSeen = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, lastSeen, m_id ) == false )
        LOG_WARN( "Failed to update last seen date for device ", m_uuid );
}

bool Device::addMountpoint( const std::string& mrl, int64_t seenDate )
{
    /*
     * We don't need to bother handling mountpoints in database for non
     * removable, non network devices.
     * Local devices can be quickly refreshed and can avoid using a potentially
     * stalled cache from database
     */
    assert( m_isRemovable == true );
    assert( m_isNetwork == true );
    assert( utils::url::schemeIs( m_scheme, mrl ) == true );
    static const std::string req = "INSERT INTO " + MountpointTable::Name +
            " VALUES(?, ?, ?)";
    return sqlite::Tools::executeInsert( m_ml->getConn(), req, m_id,
                                         utils::file::toFolderPath( mrl ),
                                         seenDate ) != 0;
}

std::shared_ptr<Device> Device::create( MediaLibraryPtr ml, const std::string& uuid,
                                        const std::string& scheme, bool isRemovable,
                                        bool isNetwork )
{
    static const std::string req = "INSERT INTO " + Device::Table::Name
            + "(uuid, scheme, is_removable, is_present, is_network, last_seen) "
            "VALUES(?, ?, ?, ?, ?, ?)";
    auto lastSeen = isRemovable ? std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count() : 0;
    auto self = std::make_shared<Device>( ml, uuid, scheme, isRemovable, isNetwork );
    if ( insert( ml, self, req, uuid, scheme, isRemovable, self->isPresent(),
                 self->isNetwork(), lastSeen ) == false )
        return nullptr;
    return self;
}

void Device::createTable( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   schema( MountpointTable::Name, Settings::DbModelVersion ) );
}

std::string Device::schema( const std::string& tableName, uint32_t dbModel )
{
    if ( tableName == MountpointTable::Name )
    {
        assert( dbModel >= 27 );
        return "CREATE TABLE " + MountpointTable::Name +
               "("
                   "device_id INTEGER,"
                   "mrl TEXT COLLATE NOCASE,"
                   "last_seen INTEGER,"
                   "PRIMARY KEY(device_id, mrl) ON CONFLICT REPLACE,"
                   "FOREIGN KEY(device_id) REFERENCES " + Table::Name +
                       "(id_device) ON DELETE CASCADE"
               ")";
    }
    assert( tableName == Table::Name );
    if ( dbModel <= 13 )
    {
        return "CREATE TABLE " + Device::Table::Name +
        "("
            "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
            "uuid TEXT UNIQUE ON CONFLICT FAIL,"
            "scheme TEXT,"
            "is_removable BOOLEAN,"
            "is_present BOOLEAN"
        ")";
    }
    if ( dbModel < 24 )
    {
        return "CREATE TABLE " + Device::Table::Name +
        "("
            "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
            "uuid TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
            "scheme TEXT,"
            "is_removable BOOLEAN,"
            "is_present BOOLEAN,"
            "last_seen UNSIGNED INTEGER"
        ")";
    }
    else if ( dbModel == 24 )
    {
        return "CREATE TABLE " + Device::Table::Name +
        "("
            "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
            "uuid TEXT COLLATE NOCASE,"
            "scheme TEXT,"
            "is_removable BOOLEAN,"
            "is_present BOOLEAN,"
            "last_seen UNSIGNED INTEGER,"
            "UNIQUE(uuid,scheme) ON CONFLICT FAIL"
        ")";
    }
    return "CREATE TABLE " + Device::Table::Name +
    "("
        "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
        "uuid TEXT COLLATE NOCASE,"
        "scheme TEXT,"
        "is_removable BOOLEAN,"
        "is_present BOOLEAN,"
        "is_network BOOLEAN,"
        "last_seen UNSIGNED INTEGER,"
        "UNIQUE(uuid,scheme) ON CONFLICT FAIL"
    ")";
}

bool Device::checkDbModel( MediaLibraryPtr ml )
{
    return sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name );
}

std::shared_ptr<Device> Device::fromUuid( MediaLibraryPtr ml, const std::string& uuid,
                                          const std::string& scheme )
{
    static const std::string req = "SELECT * FROM " + Device::Table::Name +
            " WHERE uuid = ? AND scheme = ?";
    return fetch( ml, req, uuid, scheme );
}

void Device::removeOldDevices( MediaLibraryPtr ml, std::chrono::seconds maxLifeTime )
{
    static const std::string req = "DELETE FROM " + Device::Table::Name + " "
            "WHERE last_seen < ? AND is_removable != 0";
    auto deadline = std::chrono::duration_cast<std::chrono::seconds>(
                (std::chrono::system_clock::now() - maxLifeTime).time_since_epoch() );
    if ( sqlite::Tools::executeDelete( ml->getConn(), req,
                                       deadline.count() ) == false )
        LOG_WARN( "Failed to remove old devices" );
}

std::vector<std::shared_ptr<Device>> Device::fetchByScheme( MediaLibraryPtr ml,
                                                            const std::string& scheme )
{
    static const std::string req = "SELECT * FROM " + Table::Name + " WHERE scheme = ?";
    return fetchAll<Device>( ml, req, scheme );
}

bool Device::deleteRemovable( MediaLibraryPtr ml )
{
    const std::string req = "DELETE FROM " + Device::Table::Name +
                " WHERE is_removable = 1";
    return sqlite::Tools::executeDelete( ml->getConn(), req );
}

bool Device::markNetworkAsDeviceMissing( MediaLibraryPtr ml )
{
    const std::string req = "UPDATE " + Table::Name + " SET is_present = 0 "
            "WHERE is_present != 0 AND is_network != 0 AND is_removable != 0";
    return sqlite::Tools::executeUpdate( ml->getConn(), req );
}

std::tuple<int64_t, std::string>
Device::fromMountpoint( MediaLibraryPtr ml, const std::string& mrl )
{
    LOG_DEBUG( "Trying to match ", mrl, " with a cached mountpoint" );
    static const std::string req =
    "SELECT device_id, mrl FROM " + MountpointTable::Name + " mt "
        "INNER JOIN " + Device::Table::Name + " d ON mt.device_id = d.id_device "
        "WHERE d.scheme = ? ORDER BY mt.last_seen DESC";
    auto dbConn = ml->getConn();
    sqlite::Connection::ReadContext ctx;
    if ( sqlite::Transaction::isInProgress() == false )
        ctx = dbConn->acquireReadContext();
    sqlite::Statement stmt{ dbConn->handle(), req };
    stmt.execute( utils::url::scheme( mrl ) );
    for ( auto row = stmt.row(); row != nullptr; row = stmt.row() )
    {
        assert( row.nbColumns() == 2 );
        int64_t deviceId;
        std::string mountpoint;
        row >> deviceId >> mountpoint;
        if ( strncasecmp( mrl.c_str(), mountpoint.c_str(), mountpoint.length() ) == 0 )
        {
            LOG_DEBUG( "Matched device #", deviceId );
            /*
             * Since we match the stored mountpoint in a case insensitive way,
             * be sure to return the match as it was provided, not stored, otherwise
             * the caller may not manage to remove the path since it won't be
             * found in the processed mrl
             */
            return std::make_tuple( deviceId, mrl.substr( 0, mountpoint.length() ) );
        }
    }
    LOG_DEBUG( "No match" );
    return std::make_tuple( 0, "" );
}

std::string Device::cachedMountpoint() const
{
    static const std::string req = "SELECT mrl FROM " +
        MountpointTable::Name + " WHERE device_id = ? ORDER BY last_seen DESC";
    auto dbConn = m_ml->getConn();
    auto ctx = dbConn->acquireReadContext();
    sqlite::Statement stmt{ dbConn->handle(), req };
    stmt.execute( m_id );
    auto row = stmt.row();
    if ( row == nullptr )
        return {};
    return row.extract<std::string>();
}

}
