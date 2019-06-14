/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs
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

#include "SubtitleTrack.h"

#include "Media.h"

namespace medialibrary
{

const std::string SubtitleTrack::Table::Name = "SubtitleTrack";
const std::string SubtitleTrack::Table::PrimaryKeyColumn = "id_track";
int64_t SubtitleTrack::* const SubtitleTrack::Table::PrimaryKey = &SubtitleTrack::m_id;

SubtitleTrack::SubtitleTrack( MediaLibraryPtr, sqlite::Row& row )
    : m_id( row.extract<decltype(m_id)>() )
    , m_codec( row.extract<decltype(m_codec)>() )
    , m_language( row.extract<decltype(m_language)>() )
    , m_description( row.extract<decltype(m_description)>() )
    , m_encoding( row.extract<decltype(m_encoding)>() )
{
    // Ensure there is a media id to load
    assert( row.extract<int64_t>() );
    assert( row.hasRemainingColumns() == false );
}

SubtitleTrack::SubtitleTrack( MediaLibraryPtr, std::string codec,
                              std::string language, std::string description,
                              std::string encoding )
    : m_id( 0 )
    , m_codec( std::move( codec ) )
    , m_language( std::move( language ) )
    , m_description( std::move( description ) )
    , m_encoding( std::move( encoding ) )
{
}

int64_t SubtitleTrack::id() const
{
    return m_id;
}

const std::string&SubtitleTrack::codec() const
{
    return m_codec;
}

const std::string&SubtitleTrack::language() const
{
    return m_language;
}

const std::string&SubtitleTrack::description() const
{
    return m_description;
}

const std::string&SubtitleTrack::encoding() const
{
    return m_encoding;
}

void SubtitleTrack::createTable( sqlite::Connection* dbConnection )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + Table::Name +
            "(" +
                Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                "codec TEXT,"
                "language TEXT,"
                "description TEXT,"
                "encoding TEXT,"
                "media_id UNSIGNED INT,"
                "FOREIGN KEY( media_id ) REFERENCES " + Media::Table::Name +
                    "(id_media) ON DELETE CASCADE"
            ")";
    const std::string indexReq = "CREATE INDEX IF NOT EXISTS subtitle_track_media_idx "
            " ON " + SubtitleTrack::Table::Name + "(media_id)";
    sqlite::Tools::executeRequest( dbConnection, req );
    sqlite::Tools::executeRequest( dbConnection, indexReq );
}

std::shared_ptr<SubtitleTrack> SubtitleTrack::create( MediaLibraryPtr ml,
            std::string codec, std::string language, std::string description,
            std::string encoding, int64_t mediaId )
{
    const std::string req = "INSERT INTO " + Table::Name + "(codec, language,"
            "description, encoding, media_id) VALUES(?, ?, ?, ?, ?)";
    auto track = std::make_shared<SubtitleTrack>( ml, std::move( codec ),
                    std::move( language ), std::move( description ),
                    std::move( encoding ) );
    if ( insert( ml, track, req, track->codec(), track->language(),
                 track->description(), track->encoding(), mediaId ) == false )
        return nullptr;
    return track;
}

}
