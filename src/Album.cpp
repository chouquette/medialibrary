#include "Album.h"
#include "AlbumTrack.h"

#include "SqliteTools.h"

const std::string policy::AlbumTable::Name = "Album";
const std::string policy::AlbumTable::CacheColumn = "id_album";
unsigned int Album::* const policy::AlbumTable::PrimaryKey = &Album::m_id;

Album::Album(sqlite3* dbConnection, sqlite3_stmt* stmt)
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_name = Traits<std::string>::Load( stmt, 1 );
    m_releaseYear = sqlite3_column_int( stmt, 2 );
    m_shortSummary = Traits<std::string>::Load( stmt, 3 );
    m_artworkUrl = Traits<std::string>::Load( stmt, 4 );
    m_lastSyncDate = sqlite3_column_int( stmt, 5 );
    m_id3tag = Traits<std::string>::Load( stmt, 6 );
}

Album::Album( const std::string& id3tag )
    : m_dbConnection( nullptr )
    , m_id( 0 )
    , m_releaseYear( 0 )
    , m_lastSyncDate( 0 )
    , m_id3tag( id3tag )
{
}

unsigned int Album::id() const
{
    return m_id;
}

const std::string& Album::name()
{
    return m_name;
}

unsigned int Album::releaseYear()
{
    return m_releaseYear;
}

const std::string& Album::shortSummary()
{
    return m_shortSummary;
}

const std::string& Album::artworkUrl()
{
    return m_artworkUrl;
}

time_t Album::lastSyncDate()
{
    return m_lastSyncDate;
}

bool Album::tracks( std::vector<std::shared_ptr<IAlbumTrack> >& tracks )
{
    const char* req = "SELECT * FROM AlbumTrack WHERE album_id = ?";
    return SqliteTools::fetchAll<AlbumTrack>( m_dbConnection, req, tracks, m_id );
}

AlbumTrackPtr Album::addTrack( const std::string& name, unsigned int trackNb )
{
    return AlbumTrack::create( m_dbConnection, m_id, name, trackNb );
}

bool Album::createTable( sqlite3* dbConnection )
{
    const char* req = "CREATE TABLE IF NOT EXISTS Album("
                "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT,"
                "release_year UNSIGNED INTEGER,"
                "short_summary TEXT,"
                "artwork_url TEXT,"
                "UNSIGNED INTEGER last_sync_date,"
                "id3tag TEXT UNIQUE ON CONFLICT FAIL"
            ")";
   return SqliteTools::executeRequest( dbConnection, req );
}

AlbumPtr Album::create( sqlite3* dbConnection, const std::string& id3Tag )
{
    auto album = std::make_shared<Album>( id3Tag );
    static const std::string& req = "INSERT INTO " + policy::AlbumTable::Name +
            "(id_album, id3tag) VALUES(NULL, ?)";
    if ( _Cache::insert( dbConnection, album, req.c_str(), id3Tag ) == false )
        return nullptr;
    album->m_dbConnection = dbConnection;
    return album;
}
