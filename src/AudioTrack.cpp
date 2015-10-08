#include "AudioTrack.h"

const std::string policy::AudioTrackTable::Name = "AudioTrack";
const std::string policy::AudioTrackTable::CacheColumn  = "id_track";
unsigned int AudioTrack::* const policy::AudioTrackTable::PrimaryKey = &AudioTrack::m_id;

AudioTrack::AudioTrack( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_id( sqlite::Traits<unsigned int>::Load( stmt, 0 ) )
    , m_codec( sqlite::Traits<std::string>::Load( stmt, 1 ) )
    , m_bitrate( sqlite::Traits<unsigned int>::Load( stmt, 2 ) )
    , m_sampleRate( sqlite::Traits<unsigned int>::Load( stmt, 3 ) )
    , m_nbChannels( sqlite::Traits<unsigned int>::Load( stmt, 4 ) )
{
}

AudioTrack::AudioTrack(const std::string& codec, unsigned int bitrate , unsigned int sampleRate, unsigned int nbChannels )
    : m_id( 0 )
    , m_codec( codec )
    , m_bitrate( bitrate )
    , m_sampleRate( sampleRate )
    , m_nbChannels( nbChannels )
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

unsigned int AudioTrack::sampleRate() const
{
    return m_sampleRate;
}

unsigned int AudioTrack::nbChannels() const
{
    return m_nbChannels;
}

bool AudioTrack::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::AudioTrackTable::Name
            + "(" +
                policy::AudioTrackTable::CacheColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                "codec TEXT,"
                "bitrate UNSIGNED INTEGER,"
                "samplerate UNSIGNED INTEGER,"
                "nb_channels UNSIGNED INTEGER,"
                "UNIQUE ( codec, bitrate ) ON CONFLICT FAIL"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

AudioTrackPtr AudioTrack::fetch(DBConnection dbConnection, const std::string& codec,
                unsigned int bitrate , unsigned int sampleRate, unsigned int nbChannels )
{
    static const std::string req = "SELECT * FROM " + policy::AudioTrackTable::Name
            + " WHERE codec = ? AND bitrate = ? AND samplerate = ? AND nb_channels = ?";
    return AudioTrack::fetchOne( dbConnection, req, codec, bitrate, sampleRate, nbChannels );
}

AudioTrackPtr AudioTrack::create( DBConnection dbConnection, const std::string& codec,
                unsigned int bitrate, unsigned int sampleRate, unsigned int nbChannels )
{
    static const std::string req = "INSERT INTO " + policy::AudioTrackTable::Name
            + "(codec, bitrate, samplerate, nb_channels) VALUES(?, ?, ?, ?)";
    auto track = std::make_shared<AudioTrack>( codec, bitrate, sampleRate, nbChannels );
    if ( _Cache::insert( dbConnection, track, req, codec, bitrate, sampleRate, nbChannels ) == false )
        return nullptr;
    track->m_dbConnection = dbConnection;
    return track;
}
