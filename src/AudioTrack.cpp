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
#include "database/SqliteQuery.h"
#include "File.h"

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
    , m_attachedFileId( row.extract<decltype(m_attachedFileId)>() )
{
    assert( row.hasRemainingColumns() == false );
}

AudioTrack::AudioTrack( MediaLibraryPtr, std::string codec, unsigned int bitrate, unsigned int sampleRate,
                        unsigned int nbChannels, std::string language, std::string desc,
                        int64_t mediaId, int64_t attachedFileId )
    : m_id( 0 )
    , m_codec( std::move( codec ) )
    , m_bitrate( bitrate )
    , m_sampleRate( sampleRate )
    , m_nbChannels( nbChannels )
    , m_language( std::move( language ) )
    , m_description( std::move( desc ) )
    , m_mediaId( mediaId )
    , m_attachedFileId( attachedFileId )
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

bool AudioTrack::isInAttachedFile() const
{
    return m_attachedFileId != 0;
}

void AudioTrack::createTable( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
}

void AudioTrack::createIndexes(sqlite::Connection* dbConnection)
{
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::MediaId, Settings::DbModelVersion ) );
}

std::string AudioTrack::schema( const std::string& tableName, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( tableName );

    assert( tableName == Table::Name );

    if ( dbModel < 27 )
    {
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
        "attached_file_id UNSIGNED INT,"
        "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name
            + "(id_media) ON DELETE CASCADE,"
        "FOREIGN KEY(attached_file_id) REFERENCES " + File::Table::Name +
            "(id_file) ON DELETE CASCADE,"
        "UNIQUE(media_id, attached_file_id) ON CONFLICT FAIL"
    ")";
}

std::string AudioTrack::index( AudioTrack::Indexes index, uint32_t dbModel )
{
    assert( index == Indexes::MediaId );
    return "CREATE INDEX " + indexName( index, dbModel ) + " ON "
            + Table::Name + "(media_id)";
}

std::string AudioTrack::indexName( Indexes index, uint32_t )
{
    UNUSED_IN_RELEASE( index );

    assert( index == Indexes::MediaId );
    return "audio_track_media_idx";
}

bool AudioTrack::checkDbModel( MediaLibraryPtr ml )
{
    return sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) &&
           sqlite::Tools::checkIndexStatement( ml->getConn(),
                index( Indexes::MediaId, Settings::DbModelVersion ),
                indexName( Indexes::MediaId, Settings::DbModelVersion ) );
}

std::shared_ptr<AudioTrack> AudioTrack::create( MediaLibraryPtr ml, std::string codec,
                                                unsigned int bitrate, unsigned int sampleRate, unsigned int nbChannels,
                                                std::string language, std::string desc,
                                                int64_t mediaId,
                                                int64_t attachedFileId )
{
    static const std::string req = "INSERT INTO " + AudioTrack::Table::Name
            + "(codec, bitrate, samplerate, nb_channels, language, description, "
            "media_id, attached_file_id) VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
    auto track = std::make_shared<AudioTrack>( ml, std::move( codec ), bitrate, sampleRate,
                                               nbChannels, std::move( language ),
                                               std::move( desc ), mediaId,
                                               attachedFileId );
    if ( insert( ml, track, req, track->m_codec, bitrate, sampleRate, nbChannels,
                 track->m_language, track->m_description, mediaId,
                 sqlite::ForeignKey{ attachedFileId } ) == false )
        return nullptr;
    return track;
}

bool AudioTrack::removeFromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                  bool internalTracksOnly )
{
    std::string req = "DELETE FROM " + Table::Name + " WHERE media_id = ?";
    if ( internalTracksOnly == true )
        req += " AND attached_file_id IS NULL";
    return sqlite::Tools::executeDelete( ml->getConn(), req, mediaId );
}

Query<IAudioTrack> AudioTrack::fromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                          bool internalTracksOnly )
{
    std::string req = "FROM " + AudioTrack::Table::Name + " WHERE media_id = ?";
    if ( internalTracksOnly == true )
        req += " AND attached_file_id IS NULL";
    return make_query<AudioTrack, IAudioTrack>( ml, "*", req, "", mediaId );
}

}
