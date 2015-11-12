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

#ifndef AUDIOTRACK_H
#define AUDIOTRACK_H

#include "IAudioTrack.h"
#include "IMediaLibrary.h"
#include "database/Cache.h"

class AudioTrack;

namespace policy
{
struct AudioTrackTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int AudioTrack::* const PrimaryKey;
};
}

class AudioTrack : public IAudioTrack, public Table<AudioTrack, policy::AudioTrackTable>
{
    using _Cache = Table<AudioTrack, policy::AudioTrackTable>;

    public:
        AudioTrack( DBConnection dbConnection, sqlite::Row& row );
        AudioTrack(const std::string& codec, unsigned int bitrate, unsigned int sampleRate, unsigned int nbChannels, const std::string& language, const std::string& desc , unsigned int mediaId);

        virtual unsigned int id() const override;
        virtual const std::string&codec() const override;
        virtual unsigned int bitrate() const override;
        virtual unsigned int sampleRate() const override;
        virtual unsigned int nbChannels() const override;
        virtual const std::string& language() const override;
        virtual const std::string& description() const override;

        static bool createTable( DBConnection dbConnection );
        static std::shared_ptr<AudioTrack> create( DBConnection dbConnection, const std::string& codec,
                                                   unsigned int bitrate , unsigned int sampleRate, unsigned int nbChannels,
                                                   const std::string& language, const std::string& desc, unsigned int mediaId );

    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_codec;
        unsigned int m_bitrate;
        unsigned int m_sampleRate;
        unsigned int m_nbChannels;
        std::string m_language;
        std::string m_description;
        unsigned int m_mediaId;


    private:
        friend struct policy::AudioTrackTable;
};

#endif // AUDIOTRACK_H
