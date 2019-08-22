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

#ifndef AUDIOTRACK_H
#define AUDIOTRACK_H

#include "medialibrary/IAudioTrack.h"
#include "medialibrary/IMediaLibrary.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class AudioTrack;

class AudioTrack : public IAudioTrack, public DatabaseHelpers<AudioTrack>
{
    public:
        struct Table
        {
            static const std::string Name;
            static const std::string PrimaryKeyColumn;
            static int64_t AudioTrack::* const PrimaryKey;
        };
        AudioTrack(MediaLibraryPtr ml, sqlite::Row& row );
        AudioTrack( MediaLibraryPtr ml, const std::string& codec, unsigned int bitrate,
                    unsigned int sampleRate, unsigned int nbChannels, const std::string& language,
                    const std::string& desc, int64_t mediaId );

        virtual int64_t id() const override;
        virtual const std::string&codec() const override;
        virtual unsigned int bitrate() const override;
        virtual unsigned int sampleRate() const override;
        virtual unsigned int nbChannels() const override;
        virtual const std::string& language() const override;
        virtual const std::string& description() const override;

        static void createTable( sqlite::Connection* dbConnection );
        static void createIndexes( sqlite::Connection* dbConnection );
        static std::string schema( const std::string& tableName, uint32_t dbModel );
        static bool checkDbModel( MediaLibraryPtr ml );
        static std::shared_ptr<AudioTrack> create( MediaLibraryPtr ml, const std::string& codec,
                                                   unsigned int bitrate, unsigned int sampleRate, unsigned int nbChannels,
                                                   const std::string& language, const std::string& desc, int64_t mediaId );
        static void removeFromMedia( MediaLibraryPtr ml, int64_t mediaId );

    private:
        int64_t m_id;
        const std::string m_codec;
        const unsigned int m_bitrate;
        const unsigned int m_sampleRate;
        const unsigned int m_nbChannels;
        const std::string m_language;
        const std::string m_description;
        const int64_t m_mediaId;

    private:
        friend struct AudioTrack::Table;
};

}

#endif // AUDIOTRACK_H
