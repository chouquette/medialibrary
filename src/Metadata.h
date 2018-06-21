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

namespace policy
{
struct MediaMetadataTable
{
    static const std::string Name;
};
}

class MediaMetadata
{
public:
    MediaMetadata( MediaLibraryPtr ml );

    // We have to "lazy init" this object since during containing object creation,
    // we might not know the ID yet (for instance. when instantiating the
    // metadata object during the creation of a new Media)
    void init( int64_t entityId, uint32_t nbMeta );
    bool isReady() const;

    IMediaMetadata& get( IMedia::MetadataType type ) const;
    bool set( IMedia::MetadataType type, const std::string& value );
    bool set( IMedia::MetadataType type, int64_t value );

    static void createTable( sqlite::Connection* connection );

private:
    class Record : public IMediaMetadata
    {
    public:
        virtual ~Record() = default;
        Record( IMedia::MetadataType t, std::string v );
        Record( IMedia::MetadataType t );
        virtual bool isSet() const override;
        virtual int64_t integer() const override;
        virtual const std::string& str() const override;
        void set( const std::string& value );

    private:
        IMedia::MetadataType m_type;
        std::string m_value;
        bool m_isSet;

        friend MediaMetadata;
    };

private:
    MediaLibraryPtr m_ml;
    uint32_t m_nbMeta;
    int64_t m_entityId;
    mutable std::vector<Record> m_records;
};

}
