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

class VideoTrack;

namespace policy
{
struct VideoTrackTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t VideoTrack::* const PrimaryKey;
};
}

class VideoTrack : public medialibrary::IVideoTrack, public DatabaseHelpers<VideoTrack, policy::VideoTrackTable>
{
    public:
        VideoTrack( MediaLibraryPtr, sqlite::Row& row );
        VideoTrack( MediaLibraryPtr, const std::string& codec,
                    unsigned int width, unsigned int height, float fps, int64_t mediaId );

        virtual int64_t id() const override;
        virtual const std::string& codec() const override;
        virtual unsigned int width() const override;
        virtual unsigned int height() const override;
        virtual float fps() const override;

        static bool createTable( DBConnection dbConnection );
        static std::shared_ptr<VideoTrack> create(MediaLibraryPtr ml, const std::string& codec,
                                    unsigned int width, unsigned int height, float fps, int64_t mediaId );

    private:
        int64_t m_id;
        std::string m_codec;
        unsigned int m_width;
        unsigned int m_height;
        float m_fps;
        int64_t m_mediaId;

    private:
        friend struct policy::VideoTrackTable;
};

#endif // VIDEOTRACK_H
