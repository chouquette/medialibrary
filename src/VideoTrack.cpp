/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
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

#include "VideoTrack.h"
#include "Media.h"

const std::string policy::VideoTrackTable::Name = "VideoTrack";
const std::string policy::VideoTrackTable::PrimaryKeyColumn = "id_track";
unsigned int VideoTrack::* const policy::VideoTrackTable::PrimaryKey = &VideoTrack::m_id;

VideoTrack::VideoTrack( DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_codec
        >> m_width
        >> m_height
        >> m_fps
        >> m_mediaId;
}

VideoTrack::VideoTrack( const std::string& codec, unsigned int width, unsigned int height, float fps, unsigned int mediaId )
    : m_id( 0 )
    , m_codec( codec )
    , m_width( width )
    , m_height( height )
    , m_fps( fps )
    , m_mediaId( mediaId )
{
}

unsigned int VideoTrack::id() const
{
    return m_id;
}

const std::string& VideoTrack::codec() const
{
    return m_codec;
}

unsigned int VideoTrack::width() const
{
    return m_width;
}

unsigned int VideoTrack::height() const
{
    return m_height;
}

float VideoTrack::fps() const
{
    return m_fps;
}

std::shared_ptr<VideoTrack> VideoTrack::create( DBConnection dbConnection, const std::string &codec, unsigned int width,
                                                unsigned int height, float fps, unsigned int mediaId )
{
    static const std::string req  = "INSERT INTO " + policy::VideoTrackTable::Name
            + "(codec, width, height, fps, media_id) VALUES(?, ?, ?, ?, ?)";
    auto track = std::make_shared<VideoTrack>( codec, width, height, fps, mediaId );
    if ( insert( dbConnection, track, req, codec, width, height, fps, mediaId ) == false )
        return nullptr;
    track->m_dbConnection = dbConnection;
    return track;
}

bool VideoTrack::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::VideoTrackTable::Name
            + "(" +
                policy::VideoTrackTable::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                "codec TEXT,"
                "width UNSIGNED INTEGER,"
                "height UNSIGNED INTEGER,"
                "fps FLOAT,"
                "media_id UNSIGNED INT,"
                "FOREIGN KEY ( media_id ) REFERENCES " + policy::MediaTable::Name +
                    "(id_media) ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}
