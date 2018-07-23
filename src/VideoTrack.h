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
        static std::shared_ptr<VideoTrack> create( MediaLibraryPtr ml, const std::string& codec,
                                    unsigned int width, unsigned int height, uint32_t fpsNum,
                                    uint32_t fpsDen, uint32_t bitrate, uint32_t sarNum,
                                    uint32_t sarDen, int64_t mediaId, const std::string& language,
                                    const std::string& description );

    private:
        int64_t m_id;
        std::string m_codec;
        unsigned int m_width;
        unsigned int m_height;
        uint32_t m_fpsNum;
        uint32_t m_fpsDen;
        uint32_t m_bitrate;
        uint32_t m_sarNum;
        uint32_t m_sarDen;
        int64_t m_mediaId;
        std::string m_language;
        std::string m_description;

    private:
        friend struct VideoTrack::Table;
};

}

#endif // VIDEOTRACK_H
