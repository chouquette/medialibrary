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

#include "AudioTrack.h"

#include "Media.h"

const std::string policy::AudioTrackTable::Name = "AudioTrack";
const std::string policy::AudioTrackTable::PrimaryKeyColumn  = "id_track";
unsigned int AudioTrack::* const policy::AudioTrackTable::PrimaryKey = &AudioTrack::m_id;

AudioTrack::AudioTrack( DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_codec
        >> m_bitrate
        >> m_sampleRate
        >> m_nbChannels
        >> m_language
        >> m_description
        >> m_mediaId;
}

AudioTrack::AudioTrack( const std::string& codec, unsigned int bitrate , unsigned int sampleRate,
                        unsigned int nbChannels, const std::string& language, const std::string& desc,
                        unsigned int mediaId )
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

unsigned int AudioTrack::id() const
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

bool AudioTrack::createTable( DBConnection dbConnection )
{
    //FIXME: Index on media_id ? Unless it's already implied by the foreign key
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::AudioTrackTable::Name
            + "(" +
                policy::AudioTrackTable::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                "codec TEXT,"
                "bitrate UNSIGNED INTEGER,"
                "samplerate UNSIGNED INTEGER,"
                "nb_channels UNSIGNED INTEGER,"
                "language TEXT,"
                "description TEXT,"
                "media_id UNSIGNED INT,"
                "FOREIGN KEY ( media_id ) REFERENCES " + policy::MediaTable::Name
                    + "( id_media ) ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

std::shared_ptr<AudioTrack> AudioTrack::create( DBConnection dbConnection, const std::string& codec,
                unsigned int bitrate, unsigned int sampleRate, unsigned int nbChannels,
                                                const std::string& language, const std::string& desc, unsigned int mediaId )
{
    static const std::string req = "INSERT INTO " + policy::AudioTrackTable::Name
            + "(codec, bitrate, samplerate, nb_channels, language, description, media_id) VALUES(?, ?, ?, ?, ?, ?, ?)";
    auto track = std::make_shared<AudioTrack>( codec, bitrate, sampleRate, nbChannels, language, desc, mediaId );
    if ( insert( dbConnection, track, req, codec, bitrate, sampleRate, nbChannels, language, desc, mediaId ) == false )
        return nullptr;
    track->m_dbConnection = dbConnection;
    return track;
}
