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

#include <string>
#include "medialibrary/ISubscription.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class Subscription : public ISubscription, public DatabaseHelpers<Subscription>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Subscription::*const PrimaryKey;
    };
    struct MediaRelationTable
    {
        static const std::string Name;
    };

    enum class Indexes : uint8_t
    {
        ServiceId,
        RelationMediaId,
        RelationSubscriptionId,
    };

public:
    Subscription( MediaLibraryPtr ml, sqlite::Row& row );
    Subscription( MediaLibraryPtr ml, Service service, std::string name, int64_t parentId );

    virtual int64_t id() const override;
    virtual const std::string& name() const override;
    virtual Query<ISubscription> childSubscriptions(const QueryParameters* params) override;
    virtual SubscriptionPtr parent() override;
    virtual Query<IMedia> media( const QueryParameters* params ) override;
    bool addMedia( Media& m );
    bool removeMedia( int64_t mediaId );
    bool refresh() override;
    std::shared_ptr<File> file() const;

    std::shared_ptr<Subscription> addChildSubscription( std::string name );

    bool clearContent();

    static void createTable( sqlite::Connection* connection );
    static void createIndexes( sqlite::Connection* connection );
    static std::string schema( const std::string& name, uint32_t dbModel );
    static std::string index( Indexes index, uint32_t dbModel );
    static std::string indexName( Indexes index, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static bool addMedia( MediaLibraryPtr ml, int64_t collectionId, int64_t mediaId );

    static std::shared_ptr<Subscription> create(MediaLibraryPtr ml, Service service,
                                               std::string name, int64_t parentId);
    static Query<ISubscription> fromService( MediaLibraryPtr ml, Service service,
                                           const QueryParameters* params );

    static std::shared_ptr<Subscription> fromFile( MediaLibraryPtr ml, int64_t fileId );

private:
    static std::string orderBy( const QueryParameters* params );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    Service m_service;
    std::string m_name;
    int64_t m_parentId;
    std::string m_description;
};

}

