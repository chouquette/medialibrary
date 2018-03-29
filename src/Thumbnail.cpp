/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#include "Thumbnail.h"
#include "utils/Cache.h"

namespace medialibrary
{

namespace policy
{
const std::string ThumbnailTable::Name = "Thumbnail";
const std::string ThumbnailTable::PrimaryKeyColumn = "id_thumbnail";
int64_t Thumbnail::*const ThumbnailTable::PrimaryKey = &Thumbnail::m_id;
}

const std::string Thumbnail::EmptyMrl;

Thumbnail::Thumbnail(MediaLibraryPtr ml, sqlite::Row& row)
{
    row >> m_id
        >> m_mrl
        >> m_origin;
}

Thumbnail::Thumbnail( MediaLibraryPtr ml, std::string mrl, Thumbnail::Origin origin )
    : m_ml( ml )
    , m_id( 0 )
    , m_mrl( std::move( mrl ) )
    , m_origin( origin )
{
}

int64_t Thumbnail::id() const
{
    return m_id;
}

const std::string& Thumbnail::mrl() const
{
    return m_mrl;
}

bool Thumbnail::update( std::string mrl )
{
    if ( m_mrl == mrl )
        return true;
    static const std::string req = "UPDATE " + policy::ThumbnailTable::Name +
            " SET mrl = ? WHERE id_thumbnail = ?";
    if( sqlite::Tools::executeUpdate( m_ml->getConn(), req, mrl, m_id ) == false )
        return false;
    m_mrl = std::move( mrl );
    return true;
}

Thumbnail::Origin Thumbnail::origin() const
{
    return m_origin;
}

bool Thumbnail::setMrlFromPrimaryKey( MediaLibraryPtr ml,
                                      Cache<std::shared_ptr<Thumbnail>>& thumbnail,
                                      int64_t thumbnailId, std::string mrl )
{
    auto lock = thumbnail.lock();
    if ( thumbnail.isCached() == false )
    {
        thumbnail = Thumbnail::fetch( ml, thumbnailId );
        if ( thumbnail.get() == nullptr )
        {
            LOG_WARN( "Failed to fetch thumbnail entity #", thumbnailId );
            return false;
        }
    }
    return thumbnail.get()->update( std::move( mrl ) );
}

void Thumbnail::createTable( sqlite::Connection* dbConnection )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::ThumbnailTable::Name +
            "("
                "id_thumbnail INTEGER PRIMARY KEY AUTOINCREMENT,"
                "mrl TEXT NOT NULL,"
                "origin INTEGER NOT NULL"
            ")";
    sqlite::Tools::executeRequest( dbConnection, req );
}

std::shared_ptr<Thumbnail> Thumbnail::create( MediaLibraryPtr ml, std::string mrl,
                                              Thumbnail::Origin origin )
{
    static const std::string req = "INSERT INTO " + policy::ThumbnailTable::Name +
            "(mrl, origin) VALUES(?,?)";
    auto self = std::make_shared<Thumbnail>( ml, std::move( mrl ), origin );
    if ( insert( ml, self, req, self->mrl(), origin ) == false )
        return nullptr;
    return self;
}


}
