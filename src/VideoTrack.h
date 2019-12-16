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

#ifndef VIDEOTRACK_H
#define VIDEOTRACK_H

#include "database/DatabaseHelpers.h"
#include "medialibrary/IVideoTrack.h"

#include <sqlite3.h>

namespace medialibrary
{

class VideoTrack;

class VideoTrack : public IVideoTrack, public DatabaseHelpers<VideoTrack>
{
    public:
        struct Table
        {
            static const std::string Name;
            static const std::string PrimaryKeyColumn;
            static int64_t VideoTrack::* const PrimaryKey;
        };
        enum class Indexes : uint8_t
        {
            MediaId,
        };

        VideoTrack( MediaLibraryPtr, sqlite::Row& row );
        VideoTrack( MediaLibraryPtr, const std::string& codec,
                    unsigned int width, unsigned int height, uint32_t fpsNum,
                    uint32_t fpsDen, uint32_t bitrate, uint32_t sarNum, uint32_t sarDen,
                    int64_t mediaId, const std::string& language, const std::string& description );

        virtual int64_t id() const override;
        virtual const std::string& codec() const override;
        virtual unsigned int width() const override;
        virtual unsigned int height() const override;
        virtual float fps() const override;
        virtual uint32_t fpsNum() const override;
        virtual uint32_t fpsDen() const override;
        virtual uint32_t bitrate() const override;
        virtual uint32_t sarNum() const override;
        virtual uint32_t sarDen() const override;
        virtual const std::string& language() const override;
        virtual const std::string& description() const override;

        static void createTable( sqlite::Connection* dbConnection );
        static void createIndexes( sqlite::Connection* dbConnection );
        static std::string schema( const std::string& tableName, uint32_t dbModel );
        static std::string index( Indexes index, uint32_t dbModel );
        static bool checkDbModel( MediaLibraryPtr ml );
        static std::shared_ptr<VideoTrack> create( MediaLibraryPtr ml, const std::string& codec,
                                    unsigned int width, unsigned int height, uint32_t fpsNum,
                                    uint32_t fpsDen, uint32_t bitrate, uint32_t sarNum,
                                    uint32_t sarDen, int64_t mediaId, const std::string& language,
                                    const std::string& description );
        /**
         * @brief removeFromMedia Remove all video tracks from a media
         */
        static bool removeFromMedia( MediaLibraryPtr ml, int64_t mediaId );

    private:
        int64_t m_id;
        const std::string m_codec;
        const unsigned int m_width;
        const unsigned int m_height;
        const uint32_t m_fpsNum;
        const uint32_t m_fpsDen;
        const uint32_t m_bitrate;
        const uint32_t m_sarNum;
        const uint32_t m_sarDen;
        const int64_t m_mediaId;
        const std::string m_language;
        const std::string m_description;

    private:
        friend struct VideoTrack::Table;
};

}

#endif // VIDEOTRACK_H
