#include "Album.h"
#include "AlbumTrack.h"

#include "SqliteTools.h"

const std::string policy::AlbumTable::Name = "Album";
const std::string policy::AlbumTable::CacheColumn = "id_album";
unsigned int Album::* const policy::AlbumTable::PrimaryKey = &Album::m_id;

Album::Album(DBConnection dbConnection, sqlite3_stmt* stmt)
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_name = Traits<std::string>::Load( stmt, 1 );
    m_releaseDate = sqlite3_column_int( stmt, 2 );
    m_shortSummary = Traits<std::string>::Load( stmt, 3 );
    m_artworkUrl = Traits<std::string>::Load( stmt, 4 );
    m_lastSyncDate = sqlite3_column_int( stmt, 5 );
    m_id3tag = Traits<std::string>::Load( stmt, 6 );
}

Album::Album( const std::string& id3tag )
    : m_id( 0 )
    , m_releaseDate( 0 )
    , m_lastSyncDate( 0 )
    , m_id3tag( id3tag )
{
}

unsigned int Album::id() const
{
    return m_id;
}

const std::string& Album::name() const
{
    return m_name;
}

bool Album::setName( const std::string& name )
{
    static const std::string& req = "UPDATE " + policy::AlbumTable::Name
            + " SET name = ? WHERE id_album = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, name, m_id ) == false )
        return false;
    m_name = name;
    return true;
}

time_t Album::releaseDate() const
{
    return m_releaseDate;
}

bool Album::setReleaseDate( time_t date )
{
    static const std::string& req = "UPDATE " + policy::AlbumTable::Name
            + " SET release_date = ? WHERE id_album = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, date, m_id ) == false )
        return false;
    m_releaseDate = date;
    return true;
}

const std::string& Album::shortSummary() const
{
    return m_shortSummary;
}

bool Album::setShortSummary( const std::string& summary )
{
    static const std::string& req = "UPDATE " + policy::AlbumTable::Name
            + " SET short_summary = ? WHERE id_album = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, summary, m_id ) == false )
        return false;
    m_shortSummary = summary;
    return true;
}

const std::string& Album::artworkUrl() const
{
    return m_artworkUrl;
}

bool Album::setArtworkUrl( const std::string& artworkUrl )
{
    static const std::string& req = "UPDATE " + policy::AlbumTable::Name
            + " SET artwork_url = ? WHERE id_album = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, artworkUrl, m_id ) == false )
        return false;
    m_artworkUrl = artworkUrl;
    return true;
}

time_t Album::lastSyncDate() const
{
    return m_lastSyncDate;
}

bool Album::tracks( std::vector<std::shared_ptr<IAlbumTrack> >& tracks ) const
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTrackTable::Name
            + " WHERE album_id = ?";
    return SqliteTools::fetchAll<AlbumTrack>( m_dbConnection, req, tracks, m_id );
}

AlbumTrackPtr Album::addTrack( const std::string& name, unsigned int trackNb )
{
    return AlbumTrack::create( m_dbConnection, m_id, name, trackNb );
}

bool Album::destroy()
{
    std::vector<AlbumTrackPtr> ts;
    if ( tracks( ts ) == false )
        return false;
    //FIXME: Have a single request to fetch all files at once, instead of having one per track
    for ( auto& t : ts )
    {
        t->destroy();
    }
    return _Cache::destroy( m_dbConnection, this );
}

bool Album::createTable(DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::AlbumTable::Name +
            "("
                "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT,"
                "release_date UNSIGNED INTEGER,"
                "short_summary TEXT,"
                "artwork_url TEXT,"
                "UNSIGNED INTEGER last_sync_date,"
                "id3tag TEXT UNIQUE ON CONFLICT FAIL"
            ")";
   return SqliteTools::executeRequest( dbConnection, req );
}

AlbumPtr Album::create(DBConnection dbConnection, const std::string& id3Tag )
{
    auto album = std::make_shared<Album>( id3Tag );
    static const std::string& req = "INSERT INTO " + policy::AlbumTable::Name +
            "(id_album, id3tag) VALUES(NULL, ?)";
    if ( _Cache::insert( dbConnection, album, req, id3Tag ) == false )
        return nullptr;
    album->m_dbConnection = dbConnection;
    return album;
}
