#include "AlbumTrack.h"
#include "Album.h"
#include "File.h"
#include "SqliteTools.h"

const std::string policy::AlbumTrackTable::Name = "AlbumTrack";
const std::string policy::AlbumTrackTable::CacheColumn = "id_track";
unsigned int AlbumTrack::* const policy::AlbumTrackTable::PrimaryKey = &AlbumTrack::m_id;

AlbumTrack::AlbumTrack( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_album( nullptr )
{
    m_id = Traits<unsigned int>::Load( stmt, 0 );
    m_title = Traits<std::string>::Load( stmt, 1 );
    m_genre = Traits<std::string>::Load( stmt, 2 );
    m_trackNumber = Traits<unsigned int>::Load( stmt, 3 );
    m_albumId = Traits<unsigned int>::Load( stmt, 4 );
}

AlbumTrack::AlbumTrack( const std::string& title, unsigned int trackNumber, unsigned int albumId )
    : m_dbConnection( nullptr )
    , m_id( 0 )
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

bool AlbumTrack::createTable(sqlite3* dbConnection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS AlbumTrack ("
                "id_track INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT,"
                "genre TEXT,"
                "track_number UNSIGNED INTEGER,"
                "album_id UNSIGNED INTEGER NOT NULL,"
                "FOREIGN KEY (album_id) REFERENCES Album(id_album) ON DELETE CASCADE"
            ")";
    return SqliteTools::executeRequest( dbConnection, req );
}

AlbumTrackPtr AlbumTrack::create(sqlite3* dbConnection, unsigned int albumId, const std::string& name, unsigned int trackNb)
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
        if ( File::discard( std::static_pointer_cast<File>( f ) ) == false )
        {
            std::cerr << "Failed to discard a file from cache";
            return false;
        }
    }
    return _Cache::destroy( m_dbConnection, this );
}

bool AlbumTrack::files(std::vector<FilePtr>& files)
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE album_track_id = ? ";
    return SqliteTools::fetchAll<File>( m_dbConnection, req, files, m_id );
}
