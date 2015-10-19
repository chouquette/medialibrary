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

#include "database/Cache.h"
#include "IVideoTrack.h"

#include <sqlite3.h>

class VideoTrack;

namespace policy
{
struct VideoTrackTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int VideoTrack::* const PrimaryKey;
};
}

class VideoTrack : public IVideoTrack, public Cache<VideoTrack, IVideoTrack, policy::VideoTrackTable>
{
    public:
        VideoTrack( DBConnection dbConnection, sqlite3_stmt* stmt );
        VideoTrack( const std::string& codec, unsigned int width, unsigned int height, float fps );

        virtual unsigned int id() const override;
        virtual const std::string& codec() const override;
        virtual unsigned int width() const override;
        virtual unsigned int height() const override;
        virtual float fps() const override;

        static bool createTable( DBConnection dbConnection );
        static std::shared_ptr<VideoTrack> fetch( DBConnection dbConnection, const std::string& codec,
                                    unsigned int width, unsigned int height, float fps );
        static std::shared_ptr<VideoTrack> create( DBConnection dbConnection, const std::string& codec,
                                    unsigned int width, unsigned int height, float fps );

    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        const std::string m_codec;
        const unsigned int m_width;
        const unsigned int m_height;
        const float m_fps;

    private:
        typedef Cache<VideoTrack, IVideoTrack, policy::VideoTrackTable> _Cache;
        friend struct policy::VideoTrackTable;
};

#endif // VIDEOTRACK_H
