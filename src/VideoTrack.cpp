#include "VideoTrack.h"

const std::string policy::VideoTrackTable::Name = "VideoTrack";
const std::string policy::VideoTrackTable::CacheColumn = "id_track";
unsigned int VideoTrack::* const policy::VideoTrackTable::PrimaryKey = &VideoTrack::m_id;

VideoTrack::VideoTrack(DBConnection dbConnection, sqlite3_stmt* stmt)
    : m_dbConnection( dbConnection )
    , m_id( sqlite::Traits<unsigned int>::Load( stmt, 0 ) )
    , m_codec( sqlite::Traits<std::string>::Load( stmt, 1 ) )
    , m_width( sqlite::Traits<unsigned int>::Load( stmt, 2 ) )
    , m_height( sqlite::Traits<unsigned int>::Load( stmt, 3 ) )
    , m_fps( sqlite::Traits<float>::Load( stmt, 4 ) )
{
}

VideoTrack::VideoTrack( const std::string& codec, unsigned int width, unsigned int height, float fps )
    : m_id( 0 )
    , m_codec( codec )
    , m_width( width )
    , m_height( height )
    , m_fps( fps )
{
}

unsigned int VideoTrack::id() const
{
    return m_id;
}

const std::string& VideoTrack::codec() const
{
    return m_codec;
}

unsigned int VideoTrack::width() const
{
    return m_width;
}

unsigned int VideoTrack::height() const
{
    return m_height;
}

float VideoTrack::fps() const
{
    return m_fps;
}

VideoTrackPtr VideoTrack::create( DBConnection dbConnection, const std::string &codec, unsigned int width, unsigned int height, float fps )
{
    static const std::string req  = "INSERT INTO " + policy::VideoTrackTable::Name
            + "(codec, width, height, fps) VALUES(?, ?, ?, ?)";
    auto track = std::make_shared<VideoTrack>( codec, width, height, fps );
    if ( _Cache::insert( dbConnection, track, req, codec, width, height, fps ) == false )
        return nullptr;
    track->m_dbConnection = dbConnection;
    return track;
}

bool VideoTrack::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::VideoTrackTable::Name
            + "(" +
                policy::VideoTrackTable::CacheColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                "codec TEXT,"
                "width UNSIGNED INTEGER,"
                "height UNSIGNED INTEGER,"
                "fps FLOAT,"
                "UNIQUE ( codec, width, height, fps ) ON CONFLICT FAIL"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

VideoTrackPtr VideoTrack::fetch( DBConnection dbConnection, const std::string& codec, unsigned int width, unsigned int height, float fps )
{
    static const std::string req = "SELECT * FROM " + policy::VideoTrackTable::Name +
            " WHERE codec = ? AND width = ? AND height = ? AND fps = ?";
    return sqlite::Tools::fetchOne<VideoTrack>( dbConnection, req, codec, width, height, fps );
}
