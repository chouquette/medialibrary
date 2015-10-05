#include "Artist.h"
#include "Album.h"

#include "database/SqliteTools.h"

const std::string policy::ArtistTable::Name = "artist";
const std::string policy::ArtistTable::CacheColumn = "id_artist";
unsigned int Artist::*const policy::ArtistTable::PrimaryKey = &Artist::m_id;


Artist::Artist( DBConnection dbConnection, sqlite3_stmt *stmt )
    : m_dbConnection( dbConnection )
{
    m_id = sqlite::Traits<unsigned int>::Load( stmt, 0 );
    m_name = sqlite::Traits<std::string>::Load( stmt, 1 );
    m_shortBio = sqlite::Traits<std::string>::Load( stmt, 2 );
}

Artist::Artist( const std::string& name )
    : m_id( 0 )
    , m_name( name )
{
}

unsigned int Artist::id() const
{
    return m_id;
}

const std::string &Artist::name() const
{
    return m_name;
}

const std::string &Artist::shortBio() const
{
    return m_shortBio;
}

bool Artist::setShortBio(const std::string &shortBio)
{
    static const std::string req = "UPDATE " + policy::ArtistTable::Name
            + " SET shortbio = ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, shortBio, m_id ) == false )
        return false;
    m_shortBio = shortBio;
    return true;
}

std::vector<AlbumPtr> Artist::albums() const
{
    static const std::string req = "SELECT alb.* FROM " + policy::AlbumTable::Name + " alb "
            "LEFT JOIN AlbumArtistRelation aar ON aar.id_album = alb.id_album "
            "WHERE aar.id_artist = ?";
    return sqlite::Tools::fetchAll<Album, IAlbum>( m_dbConnection, req, m_id );
}

bool Artist::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::ArtistTable::Name +
            "("
                "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT UNIQUE ON CONFLICT FAIL,"
                "shortbio TEXT"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

ArtistPtr Artist::create( DBConnection dbConnection, const std::string &name )
{
    auto artist = std::make_shared<Artist>( name );
    static const std::string req = "INSERT INTO " + policy::ArtistTable::Name +
            "(id_artist, name) VALUES(NULL, ?)";
    if ( _Cache::insert( dbConnection, artist, req, name ) == false )
        return nullptr;
    artist->m_dbConnection = dbConnection;
    return artist;
}

