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

#include "AudioTrack.h"

#include "Media.h"


namespace medialibrary
{

const std::string AudioTrack::Table::Name = "AudioTrack";
const std::string AudioTrack::Table::PrimaryKeyColumn  = "id_track";
int64_t AudioTrack::* const AudioTrack::Table::PrimaryKey = &AudioTrack::m_id;

AudioTrack::AudioTrack( MediaLibraryPtr, sqlite::Row& row )
    : m_id( row.extract<decltype(m_id)>() )
    , m_codec( row.extract<decltype(m_codec)>() )
    , m_bitrate( row.extract<decltype(m_bitrate)>() )
    , m_sampleRate( row.extract<decltype(m_sampleRate)>() )
    , m_nbChannels( row.extract<decltype(m_nbChannels)>() )
    , m_language( row.extract<decltype(m_language)>() )
    , m_description( row.extract<decltype(m_description)>() )
    , m_mediaId( row.extract<decltype(m_mediaId)>() )
{
    assert( row.hasRemainingColumns() == false );
}

AudioTrack::AudioTrack( MediaLibraryPtr, const std::string& codec, unsigned int bitrate, unsigned int sampleRate,
                        unsigned int nbChannels, const std::string& language, const std::string& desc,
                        int64_t mediaId )
    : m_id( 0 )
    , m_codec( codec )
    , m_bitrate( bitrate )
    , m_sampleRate( sampleRate )
    , m_nbChannels( nbChannels )
    , m_language( language )
    , m_description( desc )
    , m_mediaId( mediaId )
{
}

int64_t AudioTrack::id() const
{
    return m_id;
}

const std::string&AudioTrack::codec() const
{
    return m_codec;
}

unsigned int AudioTrack::bitrate() const
{
    return m_bitrate;
}

unsigned int AudioTrack::sampleRate() const
{
    return m_sampleRate;
}

unsigned int AudioTrack::nbChannels() const
{
    return m_nbChannels;
}

const std::string& AudioTrack::language() const
{
    return m_language;
}

const std::string& AudioTrack::description() const
{
    return m_description;
}

void AudioTrack::createTable( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
}

void AudioTrack::createIndexes(sqlite::Connection* dbConnection)
{
    const std::string indexReq = "CREATE INDEX IF NOT EXISTS "
            "audio_track_media_idx ON " + Table::Name + "(media_id)";
    sqlite::Tools::executeRequest( dbConnection, indexReq );
}

std::string AudioTrack::schema( const std::string& tableName, uint32_t )
{
    assert( tableName == Table::Name );
    return "CREATE TABLE " + Table::Name +
    "(" +
        AudioTrack::Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
        "codec TEXT,"
        "bitrate UNSIGNED INTEGER,"
        "samplerate UNSIGNED INTEGER,"
        "nb_channels UNSIGNED INTEGER,"
        "language TEXT,"
        "description TEXT,"
        "media_id UNSIGNED INT,"
        "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name
            + "(id_media) ON DELETE CASCADE"
    ")";
}

bool AudioTrack::checkDbModel( MediaLibraryPtr ml )
{
    return sqlite::Tools::checkSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name );
}

std::shared_ptr<AudioTrack> AudioTrack::create( MediaLibraryPtr ml, const std::string& codec,
                                                unsigned int bitrate, unsigned int sampleRate, unsigned int nbChannels,
                                                const std::string& language, const std::string& desc, int64_t mediaId )
{
    static const std::string req = "INSERT INTO " + AudioTrack::Table::Name
            + "(codec, bitrate, samplerate, nb_channels, language, description, media_id) VALUES(?, ?, ?, ?, ?, ?, ?)";
    auto track = std::make_shared<AudioTrack>( ml, codec, bitrate, sampleRate, nbChannels, language, desc, mediaId );
    if ( insert( ml, track, req, codec, bitrate, sampleRate, nbChannels, language, desc, mediaId ) == false )
        return nullptr;
    return track;
}

bool AudioTrack::removeFromMedia(MediaLibraryPtr ml, int64_t mediaId)
{
    static const std::string req = "DELETE FROM " + Table::Name + " "
            "WHERE media_id = ?";
    return sqlite::Tools::executeDelete( ml->getConn(), req, mediaId );
}

}
