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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "VideoTrack.h"
#include "Media.h"

namespace medialibrary
{

const std::string VideoTrack::Table::Name = "VideoTrack";
const std::string VideoTrack::Table::PrimaryKeyColumn = "id_track";
int64_t VideoTrack::* const VideoTrack::Table::PrimaryKey = &VideoTrack::m_id;

VideoTrack::VideoTrack( MediaLibraryPtr, sqlite::Row& row )
    : m_id( row.extract<decltype(m_id)>() )
    , m_codec( row.extract<decltype(m_codec)>() )
    , m_width( row.extract<decltype(m_width)>() )
    , m_height( row.extract<decltype(m_height)>() )
    , m_fpsNum( row.extract<decltype(m_fpsNum)>() )
    , m_fpsDen( row.extract<decltype(m_fpsDen)>() )
    , m_bitrate( row.extract<decltype(m_bitrate)>() )
    , m_sarNum( row.extract<decltype(m_sarNum)>() )
    , m_sarDen( row.extract<decltype(m_sarDen)>() )
    , m_mediaId( row.extract<decltype(m_mediaId)>() )
    , m_language( row.extract<decltype(m_language)>() )
    , m_description( row.extract<decltype(m_description)>() )
{
    assert( row.hasRemainingColumns() == false );
}

VideoTrack::VideoTrack( MediaLibraryPtr, const std::string& codec, unsigned int width,
                        unsigned int height, uint32_t fpsNum, uint32_t fpsDen,
                        uint32_t bitrate, uint32_t sarNum, uint32_t sarDen, int64_t mediaId,
                        const std::string& language, const std::string& description )
    : m_id( 0 )
    , m_codec( codec )
    , m_width( width )
    , m_height( height )
    , m_fpsNum( fpsNum )
    , m_fpsDen( fpsDen )
    , m_bitrate( bitrate )
    , m_sarNum( sarNum )
    , m_sarDen( sarDen )
    , m_mediaId( mediaId )
    , m_language( language )
    , m_description( description )
{
}

int64_t VideoTrack::id() const
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
    return static_cast<float>( m_fpsNum ) / static_cast<float>( m_fpsDen );
}

uint32_t VideoTrack::fpsNum() const
{
    return m_fpsNum;
}

uint32_t VideoTrack::fpsDen() const
{
    return m_fpsDen;
}

uint32_t VideoTrack::bitrate() const
{
    return m_bitrate;
}

uint32_t VideoTrack::sarNum() const
{
    return m_sarNum;
}

uint32_t VideoTrack::sarDen() const
{
    return m_sarDen;
}

const std::string& VideoTrack::language() const
{
    return m_language;
}

const std::string& VideoTrack::description() const
{
    return m_description;
}

std::shared_ptr<VideoTrack> VideoTrack::create( MediaLibraryPtr ml, const std::string& codec, unsigned int width,
                                                unsigned int height, uint32_t fpsNum, uint32_t fpsDen,
                                                uint32_t bitrate, uint32_t sarNum, uint32_t sarDen,
                                                int64_t mediaId, const std::string& language,
                                                const std::string& description )
{
    static const std::string req  = "INSERT INTO " + VideoTrack::Table::Name
            + "(codec, width, height, fps_num, fps_den, bitrate, sar_num, sar_den,"
               "media_id, language, description) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    auto track = std::make_shared<VideoTrack>( ml, codec, width, height, fpsNum,
                                               fpsDen, bitrate, sarNum, sarDen,
                                               mediaId, language, description );
    if ( insert( ml, track, req, codec, width, height, fpsNum, fpsDen, bitrate,
                 sarNum, sarDen, mediaId, language, description ) == false )
        return nullptr;
    return track;
}

bool VideoTrack::removeFromMedia( MediaLibraryPtr ml, int64_t mediaId )
{
    static const std::string req = "DELETE FROM " + Table::Name + " "
            "WHERE media_id = ?";
    return sqlite::Tools::executeDelete( ml->getConn(), req, mediaId );
}

void VideoTrack::createTable( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
}

void VideoTrack::createIndexes( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::MediaId, Settings::DbModelVersion ) );
}

std::string VideoTrack::schema( const std::string& tableName, uint32_t )
{
    assert( tableName == Table::Name );
    return "CREATE TABLE " + Table::Name +
    "(" +
        Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
        "codec TEXT,"
        "width UNSIGNED INTEGER,"
        "height UNSIGNED INTEGER,"
        "fps_num UNSIGNED INTEGER,"
        "fps_den UNSIGNED INTEGER,"
        "bitrate UNSIGNED INTEGER,"
        "sar_num UNSIGNED INTEGER,"
        "sar_den UNSIGNED INTEGER,"
        "media_id UNSIGNED INT,"
        "language TEXT,"
        "description TEXT,"
        "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name +
            "(id_media) ON DELETE CASCADE"
    ")";
}

std::string VideoTrack::index( Indexes index, uint32_t dbModel )
{
    assert( index == Indexes::MediaId );
    return "CREATE INDEX " + indexName( index, dbModel ) +
           " ON " + Table::Name + "(media_id)";
}

std::string VideoTrack::indexName( Indexes index, uint32_t )
{
    assert( index == Indexes::MediaId );
    return "video_track_media_idx";
}

bool VideoTrack::checkDbModel( MediaLibraryPtr ml )
{
    return sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name );
}

}
