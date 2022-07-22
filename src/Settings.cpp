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

#include "Settings.h"

#include "database/SqliteTools.h"
#include "MediaLibrary.h"

namespace medialibrary
{

const uint32_t Settings::DbModelVersion = 37u;
const uint32_t Settings::MaxTaskAttempts = 2u;
const uint32_t Settings::MaxLinkTaskAttempts = 6u;
const uint32_t Settings::DefaultNbCachedMediaPerSubscription = 2u;
const uint64_t Settings::DefaultMaxSubscriptionCacheSize = 1024 * 1024 * 1024;
const uint64_t Settings::DefaultCacheSize = DefaultMaxSubscriptionCacheSize * 3;

Settings::Settings( MediaLibrary* ml )
    : m_ml( ml )
    , m_dbModelVersion( 0 )
    , m_nbCachedMediaPerSubscription( 0 )
    , m_maxSubscriptionCacheSize( 0 )
{
}

bool Settings::load()
{
    sqlite::Statement s( "SELECT * FROM Settings" );
    auto row = s.row();
    // First launch: no settings
    if ( row == nullptr )
    {
        if ( sqlite::Tools::executeInsert( m_ml->getConn(),
                "INSERT INTO Settings VALUES(?, ?, ?, ?, ?, ?)",
                DbModelVersion, MaxTaskAttempts, MaxLinkTaskAttempts,
                DefaultNbCachedMediaPerSubscription,
                DefaultMaxSubscriptionCacheSize, DefaultCacheSize ) == false )
        {
            return false;
        }
        m_dbModelVersion = 0;
        m_nbCachedMediaPerSubscription = DefaultNbCachedMediaPerSubscription;
        m_maxSubscriptionCacheSize = DefaultMaxSubscriptionCacheSize;
    }
    else
    {
        row >> m_dbModelVersion;
        if ( m_dbModelVersion >= 37 )
        {
            m_nbCachedMediaPerSubscription = row.load<decltype(m_nbCachedMediaPerSubscription)>( 3 );
            m_maxSubscriptionCacheSize = row.load<decltype(m_maxSubscriptionCacheSize)>( 4 );
        }
        // safety check: there sould only be one row
        assert( s.row() == nullptr );
    }
    return true;
}

uint32_t Settings::dbModelVersion() const
{
    return m_dbModelVersion;
}

bool Settings::setDbModelVersion( uint32_t dbModelVersion )
{
    assert( dbModelVersion != m_dbModelVersion );
    static const std::string req = "UPDATE Settings SET db_model_version = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, dbModelVersion ) == false )
        return false;
    m_dbModelVersion = dbModelVersion;
    return true;
}

uint32_t Settings::nbCachedMediaPerSubscription() const
{
    return m_nbCachedMediaPerSubscription;
}

bool Settings::setNbCachedMediaPerSubscription( uint32_t nbCachedMedia )
{
    if ( m_nbCachedMediaPerSubscription == nbCachedMedia )
        return true;
    const std::string req = "UPDATE Settings SET nb_cached_media_per_subscription = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, nbCachedMedia ) == false )
        return false;
    m_nbCachedMediaPerSubscription = nbCachedMedia;
    return true;
}

uint64_t Settings::maxSubscriptionCacheSize() const
{
    return m_maxSubscriptionCacheSize;
}

bool Settings::setMaxSubscriptionCacheSize( uint64_t maxCacheSize )
{
    if ( m_maxSubscriptionCacheSize == maxCacheSize )
        return true;
    const std::string req = "UPDATE Settings SET max_subscription_cache_size = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, maxCacheSize ) == false )
        return false;
    m_maxSubscriptionCacheSize = maxCacheSize;
    return true;
}

uint64_t Settings::maxCacheSize() const
{
    return m_maxCacheSize;
}

bool Settings::setMaxCacheSize( uint64_t maxCacheSize )
{
    if ( m_maxCacheSize == maxCacheSize )
        return true;
    const std::string req = "UPDATE Settings SET max_cache_size = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, maxCacheSize ) == false )
        return false;
    m_maxCacheSize = maxCacheSize;
    return true;
}

void Settings::createTable( sqlite::Connection* dbConn )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS Settings("
                "db_model_version UNSIGNED INTEGER NOT NULL,"
                "max_task_attempts UNSIGNED INTEGER NOT NULL,"
                "max_link_task_attempts UNSIGNED INTEGER NOT NULL,"
                "nb_cached_media_per_subscription UNSIGNED INTEGER NOT NULL,"
                "max_subscription_cache_size UNSIGNED INTEGER NOT NULL,"
                "max_cache_size UNSIGNED INTEGER NOT NULL"
            ")";
    sqlite::Tools::executeRequest( dbConn, req );
}

}
