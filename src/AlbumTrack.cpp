#include "AlbumTrack.h"
#include "Album.h"
#include "File.h"
#include "SqliteTools.h"

const std::string policy::AlbumTrackTable::Name = "AlbumTrack";
const std::string policy::AlbumTrackTable::CacheColumn = "id_track";
unsigned int AlbumTrack::* const policy::AlbumTrackTable::PrimaryKey = &AlbumTrack::m_id;

AlbumTrack::AlbumTrack( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_album( nullptr )
{
    m_id = Traits<unsigned int>::Load( stmt, 0 );
    m_title = Traits<std::string>::Load( stmt, 1 );
    m_genre = Traits<std::string>::Load( stmt, 2 );
    m_trackNumber = Traits<unsigned int>::Load( stmt, 3 );
    m_artist = Traits<std::string>::Load( stmt, 4 );
    m_albumId = Traits<unsigned int>::Load( stmt, 5 );
}

AlbumTrack::AlbumTrack( const std::string& title, unsigned int trackNumber, unsigned int albumId )
    : m_id( 0 )
    , m_title( title )
    , m_trackNumber( trackNumber )
    , m_albumId( albumId )
    , m_album( nullptr )
{
}

unsigned int AlbumTrack::id() const
{
    return m_id;
}

bool AlbumTrack::createTable( DBConnection dbConnection )
{
    const char* req = "CREATE TABLE IF NOT EXISTS AlbumTrack ("
                "id_track INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT,"
                "genre TEXT,"
                "track_number UNSIGNED INTEGER,"
                "artist TEXT,"
                "album_id UNSIGNED INTEGER NOT NULL,"
                "FOREIGN KEY (album_id) REFERENCES Album(id_album) ON DELETE CASCADE"
            ")";
    return SqliteTools::executeRequest( dbConnection, req );
}

AlbumTrackPtr AlbumTrack::create(DBConnection dbConnection, unsigned int albumId, const std::string& name, unsigned int trackNb)
{
    auto self = std::make_shared<AlbumTrack>( name, trackNb, albumId );
    static const std::string req = "INSERT INTO " + policy::AlbumTrackTable::Name
            + "(title, track_number, album_id) VALUES(?, ?, ?)";
    if ( _Cache::insert( dbConnection, self, req, name, trackNb, albumId ) == false )
        return nullptr;
    self->m_dbConnection = dbConnection;
    return self;
}

const std::string& AlbumTrack::genre()
{
    return m_genre;
}

bool AlbumTrack::setGenre(const std::string& genre)
{
    static const std::string req = "UPDATE " + policy::AlbumTrackTable::Name
            + " SET genre = ? WHERE id_track = ? ";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, genre, m_id ) == false )
        return false;
    m_genre = genre;
    return true;
}

const std::string& AlbumTrack::title()
{
    return m_title;
}

unsigned int AlbumTrack::trackNumber()
{
    return m_trackNumber;
}

std::shared_ptr<IAlbum> AlbumTrack::album()
{
    if ( m_album == nullptr && m_albumId != 0 )
    {
        m_album = Album::fetch( m_dbConnection, m_albumId );
    }
    return m_album;
}

bool AlbumTrack::destroy()
{
    // Manually remove Files from cache, and let foreign key handling delete them from the DB
    std::vector<FilePtr> fs;
    if ( files( fs ) == false )
        return false;
    if ( fs.size() == 0 )
        std::cerr << "No files found for AlbumTrack " << m_id << std::endl;
    for ( auto& f : fs )
    {
        // Ignore failures to discard from cache, we might want to discard records from
        // cache in a near future to avoid running out of memory on mobile devices
        File::discard( std::static_pointer_cast<File>( f ) );
    }
    return _Cache::destroy( m_dbConnection, this );
}

const std::string&AlbumTrack::artist() const
{
    return m_artist;
}

bool AlbumTrack::setArtist(const std::string& artist)
{
    static const std::string req = "UPDATE " + policy::AlbumTrackTable::Name +
            " SET artist = ? WHERE id_track = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, artist, m_id ) == false )
        return false;
    m_artist = artist;
    return true;
}

bool AlbumTrack::files(std::vector<FilePtr>& files)
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE album_track_id = ? ";
    return SqliteTools::fetchAll<File>( m_dbConnection, req, files, m_id );
}
