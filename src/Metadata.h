/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "medialibrary/IMetadata.h"
#include "medialibrary/IMedia.h"
#include "Types.h"

namespace medialibrary
{

namespace sqlite
{
class Connection;
}

class Metadata
{
public:
    struct Table
    {
        static const std::string Name;
    };
    Metadata( MediaLibraryPtr ml, IMetadata::EntityType entityType );

    // We have to "lazy init" this object since during containing object creation,
    // we might not know the ID yet (for instance. when instantiating the
    // metadata object during the creation of a new Media)
    void init( int64_t entityId, uint32_t nbMeta );
    bool isReady() const;

    IMetadata& get( uint32_t type ) const;
    bool set( uint32_t type, const std::string& value );
    bool set( uint32_t type, int64_t value );
    bool unset( uint32_t type );

    static void unset( sqlite::Connection* dbConn, IMetadata::EntityType entityType, uint32_t type );

    static void createTable( sqlite::Connection* connection );

private:
    class Record : public IMetadata
    {
    public:
        virtual ~Record() = default;
        Record( uint32_t t, std::string v );
        Record( uint32_t t );
        virtual bool isSet() const override;
        virtual int64_t integer() const override;
        virtual double asDouble() const override;
        virtual const std::string& str() const override;
        void unset();
        void set( const std::string& value );

    private:
        const uint32_t m_type;
        std::string m_value;
        bool m_isSet;

        friend Metadata;
    };

private:
    MediaLibraryPtr m_ml;
    IMetadata::EntityType m_entityType;
    uint32_t m_nbMeta;
    int64_t m_entityId;
    mutable std::vector<Record> m_records;
};

}
