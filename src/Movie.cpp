#include "Movie.h"
#include "File.h"
#include "database/SqliteTools.h"

const std::string policy::MovieTable::Name = "Movie";
const std::string policy::MovieTable::CacheColumn = "id_movie";
unsigned int Movie::* const policy::MovieTable::PrimaryKey = &Movie::m_id;

Movie::Movie( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
{
    m_id = sqlite::Traits<unsigned int>::Load( stmt, 0 );
    m_title = sqlite::Traits<std::string>::Load( stmt, 1 );
    m_releaseDate = sqlite::Traits<time_t>::Load( stmt, 2 );
    m_summary = sqlite::Traits<std::string>::Load( stmt, 3 );
    m_artworkUrl = sqlite::Traits<std::string>::Load( stmt, 4 );
    m_imdbId = sqlite::Traits<std::string>::Load( stmt, 5 );
}

Movie::Movie( const std::string& title )
    : m_id( 0 )
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
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, date, m_id ) == false )
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
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, summary, m_id ) == false )
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
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artworkUrl, m_id ) == false )
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
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, imdbId, m_id ) == false )
        return false;
    m_imdbId = imdbId;
    return true;
}

bool Movie::destroy()
{
    auto fs = files();
    for ( auto& f : fs )
    {
        File::discard( std::static_pointer_cast<File>( f ) );
    }
    return _Cache::destroy( m_dbConnection, this );
}

std::vector<FilePtr> Movie::files()
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE movie_id = ?";
    return File::fetchAll( m_dbConnection, req, m_id );
}

bool Movie::createTable( DBConnection dbConnection )
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
    return sqlite::Tools::executeRequest( dbConnection, req );
}

MoviePtr Movie::create(DBConnection dbConnection, const std::string& title )
{
    auto movie = std::make_shared<Movie>( title );
    static const std::string req = "INSERT INTO " + policy::MovieTable::Name
            + "(title) VALUES(?)";
    if ( _Cache::insert( dbConnection, movie, req, title ) == false )
        return nullptr;
    movie->m_dbConnection = dbConnection;
    return movie;
}
