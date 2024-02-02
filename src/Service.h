/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#pragma once

#include "medialibrary/IService.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class Service : public IService, public DatabaseHelpers<Service>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Service::*const PrimaryKey;
    };
    enum class Triggers : uint8_t
    {
        IncrementNbSubscriptions,
        DecrementNbSubscriptions,
        UpdateMediaCounters,
        DecrementMediaCountersOnSubRemoval,
    };

    Service( MediaLibraryPtr ml, sqlite::Row& row );
    Service( MediaLibraryPtr ml, Type type );

    virtual Type type() const override;
    virtual bool addSubscription( std::string mrl ) override;
    virtual Query<ISubscription> subscriptions( const QueryParameters* params ) const override;
    virtual Query<ISubscription> searchSubscription( const std::string& pattern,
                                                     const QueryParameters* params ) const override;
    virtual Query<IMedia> media( const QueryParameters* params ) const override;
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       const QueryParameters* params ) const override;
    virtual bool isAutoDownloadEnabled() const override;
    virtual bool setAutoDownloadEnabled(bool enabled) override;
    virtual bool isNewMediaNotificationEnabled() const override;
    virtual bool setNewMediaNotificationEnabled(bool enabled) override;
    virtual int64_t maxCacheSize() const override;
    virtual bool setMaxCacheSize(int64_t maxSize) override;
    virtual uint32_t nbSubscriptions() const override;
    virtual uint32_t nbUnplayedMedia() const override;
    virtual uint32_t nbMedia() const override;
    virtual bool refresh() override;

    static std::string schema( const std::string& name, uint32_t dbModel );
    static std::string trigger( Triggers t, uint32_t dbModel );
    static std::string triggerName( Triggers t, uint32_t dbModel );
    static void createTable(sqlite::Connection* dbConn );
    static void createTriggers( sqlite::Connection* dbConn );
    static bool checkDbModel( MediaLibraryPtr ml );


    static std::shared_ptr<Service> fetch( MediaLibraryPtr ml, Type type );

private:
    static std::shared_ptr<Service> create( MediaLibraryPtr ml, Type type );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    bool m_autoDownload;
    bool m_newMediaNotif;
    int64_t m_maxCacheSize;
    uint32_t m_nbSubscriptions;
    uint32_t m_nbUnplayedMedia;
    uint32_t m_nbMedia;
};

}
