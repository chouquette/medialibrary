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

#pragma once

#include "medialibrary/IMediaGroup.h"
#include "Types.h"
#include "database/SqliteTools.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class MediaGroup : public IMediaGroup, public DatabaseHelpers<MediaGroup>
{
public:
    static constexpr auto AutomaticGroupPrefixSize = 6u;
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t MediaGroup::*const PrimaryKey;
    };
    struct FtsTable
    {
        static const std::string Name;
    };
    enum class Triggers : uint8_t
    {
        InsertFts,
        DeleteFts,
        IncrementNbMediaOnGroupChange, // Deprecated in model 26
        DecrementNbMediaOnGroupChange, // Deprecated in model 26
        DecrementNbMediaOnDeletion,
        DeleteEmptyGroups,
        RenameForcedSingleton,
        UpdateDurationOnMediaChange,
        UpdateDurationOnMediaDeletion,
        UpdateNbMediaPerType,
        UpdateTotalNbMedia,
        UpdateMediaCountOnPresenceChange,
    };
    enum class Indexes : uint8_t
    {
        ParentId, // Deprecated in model 25
        ForcedSingleton,
        Duration,
        CreationDate,
        LastModificationDate,
    };

    MediaGroup( MediaLibraryPtr ml, sqlite::Row& row );
    MediaGroup( MediaLibraryPtr ml, std::string name, bool userInitiated,
                bool forcedSingleton );
    MediaGroup( MediaLibraryPtr ml, std::string name );
    virtual int64_t id() const override;
    virtual const std::string& name() const override;
    virtual uint32_t nbMedia() const override;
    virtual uint32_t nbTotalMedia() const override;
    virtual uint32_t nbVideo() const override;
    virtual uint32_t nbAudio() const override;
    virtual uint32_t nbUnknown() const override;
    virtual int64_t duration() const override;
    virtual time_t creationDate() const override;
    virtual time_t lastModificationDate() const override;
    virtual bool userInteracted() const override;
    virtual bool add( IMedia& media ) override;
    virtual bool add(int64_t mediaId ) override;
    virtual bool remove( IMedia& media ) override;
    virtual bool remove( int64_t mediaId ) override;
    virtual Query<IMedia> media( IMedia::Type mediaType,
                                 const QueryParameters* params = nullptr ) override;
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       IMedia::Type mediaType,
                                       const QueryParameters* params = nullptr ) override;
    virtual bool rename( std::string name ) override;
    bool rename( std::string name, bool userInitiated );

    bool isForcedSingleton() const;

    /**
     * @brief rename Rename a group
     * @param name The new name
     * @param userInitiated true if this comes from the public API
     *
     * If userInitiated is true, the has_been_renamed column will be set to true
     * Otherwise, this is assumed to be an internal change and the user will not
     * be notified
     */
    virtual bool destroy() override;

    static std::shared_ptr<MediaGroup> create( MediaLibraryPtr ml, std::string name,
                                               bool usedInitiated, bool isForcedSingleton );
    static std::shared_ptr<MediaGroup> create( MediaLibraryPtr ml,
                                               const std::vector<int64_t>& mediaIds );
    static Query<IMediaGroup> listAll( MediaLibraryPtr ml, IMedia::Type mediaType,
                                       const QueryParameters* params );
    static Query<IMediaGroup> search( MediaLibraryPtr ml, const std::string& pattern,
                                      const QueryParameters* params );
    static void createTable( sqlite::Connection* connection );
    static void createTriggers( sqlite::Connection* connection );
    static void createIndexes( sqlite::Connection* connection );
    static std::string schema( const std::string& name, uint32_t dbModel );
    static std::string trigger( Triggers t, uint32_t dbModel );
    static std::string triggerName( Triggers t, uint32_t dbModel );
    static std::string index( Indexes i, uint32_t dbModel );
    static std::string indexName( Indexes i, uint32_t dbModel );

    static bool checkDbModel( MediaLibraryPtr ml );

    static bool assignToGroup( MediaLibraryPtr ml, Media& m );
    static std::string commonPattern( const std::string& groupName,
                                      const std::string& newTitle );
    static std::string prefix( const std::string& title );

private:
    static std::string orderBy( const QueryParameters* params );
    static std::vector<std::shared_ptr<MediaGroup> > fetchMatching( MediaLibraryPtr ml,
                                                  const std::string& prefix );

    bool add( IMedia& media, bool force );
    bool add( int64_t mediaId, bool force );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    std::string m_name;
    uint32_t m_nbVideo;
    uint32_t m_nbAudio;
    uint32_t m_nbUnknown;
    uint32_t m_nbMedia;
    int64_t m_duration;
    time_t m_creationDate;
    time_t m_lastModificationDate;
    /* Has the group been interacted with by the user */
    bool m_userInteracted;
    /*
     * Should this group be considered when automatically grouping.
     * This is true for groups we create to contain a single "ungrouped" media
     */
    bool m_forcedSingleton;
};

}
