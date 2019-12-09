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
        DeleteFts
    };

    MediaGroup( MediaLibraryPtr ml, sqlite::Row& row );
    MediaGroup( MediaLibraryPtr ml, int64_t parentId, std::string name );
    virtual int64_t id() const override;
    virtual const std::string& name() const override;
    virtual uint32_t nbMedia() const override;
    virtual uint32_t nbVideo() const override;
    virtual uint32_t nbAudio() const override;
    virtual uint32_t nbUnknown() const override;
    virtual bool add( IMedia& media ) override;
    virtual bool add(int64_t mediaId ) override;
    virtual bool remove( IMedia& media ) override;
    virtual bool remove( int64_t mediaId ) override;
    virtual std::shared_ptr<IMediaGroup> createSubgroup( const std::string& name ) override;
    virtual Query<IMediaGroup> subgroups( const QueryParameters* params ) const override;
    virtual bool isSubgroup() const override;
    virtual std::shared_ptr<IMediaGroup> parent() const override;
    virtual Query<IMedia> media( IMedia::Type mediaType,
                                 const QueryParameters* params = nullptr ) override;
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       IMedia::Type mediaType,
                                       const QueryParameters* params = nullptr ) override;

    static std::shared_ptr<MediaGroup> create(MediaLibraryPtr ml,
                                               int64_t parentId, std::string name );
    static std::shared_ptr<MediaGroup> fetchByName( MediaLibraryPtr ml,
                                            const std::string& name );
    static Query<IMediaGroup> listAll( MediaLibraryPtr ml, const QueryParameters* params );
    static Query<IMediaGroup> search( MediaLibraryPtr ml, const std::string& pattern,
                                      const QueryParameters* params );
    static void createTable( sqlite::Connection* connection );
    static void createTriggers( sqlite::Connection* connection );
    static std::string schema( const std::string& name, uint32_t dbModel );
    static std::string trigger( Triggers t, uint32_t dbModel );

private:
    static std::string orderBy( const QueryParameters* params );
    /**
     * @brief exists Checks for the existance of a root group with the given name
     */
    static bool exists( MediaLibraryPtr ml, const std::string& name );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    int64_t m_parentId;
    std::string m_name;
    uint32_t m_nbVideo;
    uint32_t m_nbAudio;
    uint32_t m_nbUnknown;
};

}
