#ifndef VIDEOTRACK_H
#define VIDEOTRACK_H

#include "Cache.h"
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

        virtual unsigned int id() const;
        virtual const std::string& codec() const;
        virtual unsigned int width() const;
        virtual unsigned int height() const;
        virtual float fps() const;

        static bool createTable( DBConnection dbConnection );
        static VideoTrackPtr fetch( DBConnection dbConnection, const std::string& codec,
                                    unsigned int width, unsigned int height, float fps );
        static VideoTrackPtr create( DBConnection dbConnection, const std::string& codec,
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
