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

namespace medialibrary
{

const std::string Device::Table::Name = "Device";
const std::string Device::Table::PrimaryKeyColumn = "id_device";
int64_t Device::* const Device::Table::PrimaryKey = &Device::m_id;

Device::Device( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_uuid( row.extract<decltype(m_uuid)>() )
    , m_scheme( row.extract<decltype(m_scheme)>() )
    , m_isRemovable( row.extract<decltype(m_isRemovable)>() )
    , m_isPresent( row.extract<decltype(m_isPresent)>() )
    , m_isNetwork( row.extract<decltype(m_isNetwork)>() )
    , m_lastSeen( row.extract<decltype(m_lastSeen)>() )
{
    assert( row.hasRemainingColumns() == false );
}

Device::Device( MediaLibraryPtr ml, const std::string& uuid,
                const std::string& scheme, bool isRemovable, bool isNetwork,
                time_t lastSeen )
    : m_ml( ml )
    , m_id( 0 )
    , m_uuid( uuid )
    , m_scheme( scheme )
    , m_isRemovable( isRemovable )
    // Assume we can't add an absent device
    , m_isPresent( true )
    , m_isNetwork( isNetwork )
    , m_lastSeen( lastSeen )
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

bool Device::forceNonRemovable()
{
    LOG_INFO( "Fixing up device ", m_uuid, " removable state..." );
    auto dbConn = m_ml->getConn();
    auto t = dbConn->newTransaction();
    // The folders were also create based on the removable state, so we need to
    // update their mrl.
    // Files were not impacted by the issue.
    const std::string foldersReq = "SELECT * FROM " + Folder::Table::Name +
            " WHERE device_id = ?";
    auto folders = Folder::fetchAll<Folder>( m_ml, foldersReq, m_id );
    for ( auto& f : folders )
    {
        if ( f->isRemovable() == false )
            continue;
        auto fullMrl = f->mrl();
        if ( f->forceNonRemovable( fullMrl ) == false )
            return false;
    }
    // Update the device after updating the folders, to avoid any potential
    // screwup where the device would be deemed non-removable, and the MRL would
    // be only the relative part.
    const std::string req = "UPDATE " + Table::Name + " SET is_removable = ? "
            " WHERE id_device = ?";
    if ( sqlite::Tools::executeUpdate( dbConn, req, false, m_id ) == false )
        return false;
    m_isRemovable = false;
    t->commit();
    return true;
}

bool Device::isPresent() const
{
    return m_isPresent;
}

void Device::setPresent(bool value)
{
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
    auto self = std::make_shared<Device>( ml, uuid, scheme, isRemovable, isNetwork,
                                          lastSeen );
    if ( insert( ml, self, req, uuid, scheme, isRemovable, self->isPresent(),
                 self->isNetwork(), lastSeen ) == false )
        return nullptr;
    return self;
}

void Device::createTable( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
}

std::string Device::schema( const std::string& tableName, uint32_t dbModel )
{
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

bool Device::markNetworkAsDeviceMissing( MediaLibraryPtr ml )
{
    const std::string req = "UPDATE " + Table::Name + " SET is_present = 0 "
            "WHERE is_present != 0 AND is_network != 0 AND is_removable != 0";
    return sqlite::Tools::executeUpdate( ml->getConn(), req );
}

}
