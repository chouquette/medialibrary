#include "Movie.h"
#include "File.h"
#include "SqliteTools.h"

const std::string policy::MovieTable::Name = "Movie";
const std::string policy::MovieTable::CacheColumn = "id_movie";
unsigned int Movie::* const policy::MovieTable::PrimaryKey = &Movie::m_id;

Movie::Movie( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
{
    m_id = Traits<unsigned int>::Load( stmt, 0 );
    m_title = Traits<std::string>::Load( stmt, 1 );
    m_releaseDate = Traits<time_t>::Load( stmt, 2 );
    m_summary = Traits<std::string>::Load( stmt, 3 );
    m_artworkUrl = Traits<std::string>::Load( stmt, 4 );
    m_imdbId = Traits<std::string>::Load( stmt, 5 );
}

Movie::Movie( const std::string& title )
    : m_dbConnection( nullptr )
    , m_id( 0 )
    , m_title( title )
    , m_releaseDate( 0 )
{
}


unsigned int Movie::id() const
{
    return m_id;
}

const std::string&Movie::title() const
{
    return m_title;
}

time_t Movie::releaseDate() const
{
    return m_releaseDate;
}

bool Movie::setReleaseDate( time_t date )
{
    static const std::string req = "UPDATE " + policy::MovieTable::Name
            + " SET release_date = ? WHERE id_movie = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, date, m_id ) == false )
        return false;
    m_releaseDate = date;
    return true;
}

const std::string&Movie::shortSummary() const
{
    return m_summary;
}

bool Movie::setShortSummary( const std::string& summary )
{
    static const std::string req = "UPDATE " + policy::MovieTable::Name
            + " SET summary = ? WHERE id_movie = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, summary, m_id ) == false )
        return false;
    m_summary = summary;
    return true;
}

const std::string&Movie::artworkUrl() const
{
    return m_artworkUrl;
}

bool Movie::setArtworkUrl( const std::string& artworkUrl )
{
    static const std::string req = "UPDATE " + policy::MovieTable::Name
            + " SET artwork_url = ? WHERE id_movie = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, artworkUrl, m_id ) == false )
        return false;
    m_artworkUrl = artworkUrl;
    return true;
}

const std::string& Movie::imdbId() const
{
    return m_imdbId;
}

bool Movie::setImdbId( const std::string& imdbId )
{
    static const std::string req = "UPDATE " + policy::MovieTable::Name
            + " SET imdb_id = ? WHERE id_movie = ?";
    if ( SqliteTools::executeUpdate( m_dbConnection, req, imdbId, m_id ) == false )
        return false;
    m_imdbId = imdbId;
    return true;
}

bool Movie::destroy()
{
    std::vector<FilePtr> fs;
    if ( files( fs ) == false )
        return false;
    for ( auto& f : fs )
    {
        File::discard( std::static_pointer_cast<File>( f ) );
    }
    return _Cache::destroy( m_dbConnection, this );
}

bool Movie::files( std::vector<FilePtr>& files )
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE movie_id = ?";
    return SqliteTools::fetchAll<File>( m_dbConnection, req, files, m_id );
}

bool Movie::createTable( sqlite3* dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::MovieTable::Name
            + "("
                "id_movie INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT UNIQUE ON CONFLICT FAIL,"
                "release_date UNSIGNED INTEGER,"
                "summary TEXT,"
                "artwork_url TEXT,"
                "imdb_id TEXT"
            ")";
    return SqliteTools::executeRequest( dbConnection, req );
}

MoviePtr Movie::create( sqlite3* dbConnection, const std::string& title )
{
    auto movie = std::make_shared<Movie>( title );
    static const std::string req = "INSERT INTO " + policy::MovieTable::Name
            + "(title) VALUES(?)";
    if ( _Cache::insert( dbConnection, movie, req, title ) == false )
        return nullptr;
    movie->m_dbConnection = dbConnection;
    return movie;
}
