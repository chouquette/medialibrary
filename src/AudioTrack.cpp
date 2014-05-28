#include "AudioTrack.h"

const std::string policy::AudioTrackTable::Name = "AudioTrack";
const std::string policy::AudioTrackTable::CacheColumn  = "id_track";
unsigned int AudioTrack::* const policy::AudioTrackTable::PrimaryKey = &AudioTrack::m_id;

AudioTrack::AudioTrack( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_id( Traits<unsigned int>::Load( stmt, 0 ) )
    , m_codec( Traits<std::string>::Load( stmt, 1 ) )
    , m_bitrate( Traits<unsigned int>::Load( stmt, 2 ) )
{
}

AudioTrack::AudioTrack( const std::string& codec, unsigned int bitrate )
    : m_id( 0 )
    , m_codec( codec )
    , m_bitrate( bitrate )
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

bool AudioTrack::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::AudioTrackTable::Name
            + "(" +
                policy::AudioTrackTable::CacheColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                "codec TEXT,"
                "bitrate UNSIGNED INTEGER,"
                "UNIQUE ( codec, bitrate ) ON CONFLICT FAIL"
            ")";
    return SqliteTools::executeRequest( dbConnection, req );
}

AudioTrackPtr AudioTrack::fetch( DBConnection dbConnection, const std::string& codec, unsigned int bitrate )
{
    static const std::string req = "SELECT * FROM " + policy::AudioTrackTable::Name
            + " WHERE codec = ? AND bitrate = ?";
    return SqliteTools::fetchOne<AudioTrack>( dbConnection, req, codec, bitrate );
}

AudioTrackPtr AudioTrack::create( DBConnection dbConnection, const std::string& codec, unsigned int bitrate )
{
    static const std::string req = "INSERT INTO " + policy::AudioTrackTable::Name
            + "(codec, bitrate) VALUES(?, ?)";
    auto track = std::make_shared<AudioTrack>( codec, bitrate );
    if ( _Cache::insert( dbConnection, track, req, codec, bitrate ) == false )
        return nullptr;
    track->m_dbConnection = dbConnection;
    return track;
}
