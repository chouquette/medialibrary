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
#include "Service.h"

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

    enum class Triggers : uint8_t
    {
        PropagateTaskDeletion,
        IncrementCachedSize,
        DecrementCachedSize,
        DecrementCachedSizeOnRemoval,
        /*
         * Increment/decrement unplayed media when inserting/removing from the
         * subscription table
         */
        IncrementUnplayedMedia,
        DecrementUnplayedMedia,
        /*
         * This trigger handles the media removal. We can't implement this as part
         * of the regular DecrementUnplayedMedia trigger since when the media
         * gets removed from the relation table following a delete, we can't know
         * its play count.
         */
        DecrementUnplayedMediaOnDestroy,
        /* Handle play_count changes in the media table */
        UpdateUnplayedMedia,
    };

    enum class Indexes : uint8_t
    {
        ServiceId,
        RelationMediaId,
        RelationSubscriptionId,
    };

public:
    Subscription( MediaLibraryPtr ml, sqlite::Row& row );
    Subscription( MediaLibraryPtr ml, IService::Type service, std::string name,
                  int64_t parentId );

    virtual int64_t id() const override;
    virtual IService::Type service() const override;
    virtual const std::string& name() const override;
    virtual Query<ISubscription> childSubscriptions( const QueryParameters* params ) const override;
    virtual SubscriptionPtr parent() override;
    virtual Query<IMedia> media( const QueryParameters* params ) const override;
    virtual uint64_t cachedSize() const override;
    virtual int32_t maxCachedMedia() const override;
    virtual bool setMaxCachedMedia( int32_t nbCachedMedia ) override;
    virtual int64_t maxCachedSize() const override;
    virtual bool setMaxCachedSize( int64_t maxCachedSize ) override;
    virtual int8_t newMediaNotification() const override;
    virtual bool setNewMediaNotification( int8_t value ) override;
    virtual uint32_t nbUnplayedMedia() const override;
    virtual uint32_t nbMedia() const override;

    bool addMedia( Media& m );
    bool removeMedia( int64_t mediaId );
    bool refresh() override;
    std::shared_ptr<File> file() const;
    /**
     * @brief cachedMedia Returns all the cached media for this collection
     * @param evictableOnly If true, this function will only return media that are
     *                      considered safe to be evited
     * @return A query to access cached media
     *
     * The returned media will already be sorted by descending play_count and
     * ascending release_date, which is the order in which we should evict the
     * media from the cache: when removing something from cache we prioritize
     * media that were played the most and that were released as far as possible
     * in the past.
     * A media is considered safe for eviction if it was cached manually AND has
     * been played, or if it was automatically cached.
     */
    Query<Media> cachedMedia(bool evictableOnly) const;
    /**
     * @brief uncachedMedia Returns the uncached media in this collection
     * @param autoOnly If true, only the media not already handled by automatic
     *                 caching will be returned
     *
     * The returned media will be sorted by descending release date (from the most
     * recent to the oldest one, in term of release date as provided by the
     * subscription manifest)
     */
    std::vector<std::shared_ptr<Media>> uncachedMedia(bool autoOnly) const;
    /**
     * @brief markCacheAsHandled Will mark all the media belonging to a subscription
     * as handed by the automatic cache pass.
     * @return true in case of success, false otherwise
     *
     * Once a media has been marked as handled by the automatic caching, it won't
     * be considered for automatic caching anymore.
     */
    bool markCacheAsHandled();

    std::shared_ptr<Subscription> addChildSubscription( std::string name );

    bool clearContent();

    static void createTable( sqlite::Connection* connection );
    static void createTriggers( sqlite::Connection* connection );
    static void createIndexes( sqlite::Connection* connection );
    static std::string schema( const std::string& name, uint32_t dbModel );
    static std::string trigger( Triggers trigger, uint32_t dbModel );
    static std::string triggerName( Triggers trigger, uint32_t dbModel );
    static std::string index( Indexes index, uint32_t dbModel );
    static std::string indexName( Indexes index, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static bool addMedia( MediaLibraryPtr ml, int64_t collectionId, int64_t mediaId );

    static std::shared_ptr<Subscription> create( MediaLibraryPtr ml, IService::Type service,
                                                 std::string name, int64_t parentId );
    static Query<ISubscription> fromService( MediaLibraryPtr ml, IService::Type service,
                                             const QueryParameters* params );

    static std::shared_ptr<Subscription> fromFile( MediaLibraryPtr ml, int64_t fileId );

private:
    static std::string orderBy( const QueryParameters* params );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    IService::Type m_service;
    std::string m_name;
    int64_t m_parentId;
    uint64_t m_cachedSize;
    int32_t m_maxCachedMedia;
    int64_t m_maxCachedSize;
    int8_t m_newMediaNotification;
    uint32_t m_nbUnplayedMedia;
    uint32_t m_nbMedia;
};

}

