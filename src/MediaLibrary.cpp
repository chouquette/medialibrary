#include <algorithm>
#include <functional>

#include "MediaLibrary.h"
#include "IMetadataService.h"
#include "SqliteTools.h"
#include "File.h"
#include "Label.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Movie.h"
#include "Show.h"
#include "ShowEpisode.h"

MediaLibrary::MediaLibrary()
    : m_dbConnection( nullptr )
{
}

MediaLibrary::~MediaLibrary()
{
    File::clear();
    Label::clear();
    Album::clear();
    AlbumTrack::clear();
    Show::clear();
    ShowEpisode::clear();
    Movie::clear();
}

bool MediaLibrary::initialize(const std::string& dbPath)
{
    int res = sqlite3_open( dbPath.c_str(), &m_dbConnection );
    if ( res != SQLITE_OK )
        return false;
    if ( SqliteTools::executeRequest( m_dbConnection, "PRAGMA foreign_keys = ON" ) == false )
        return false;
    return ( File::createTable( m_dbConnection ) &&
        Label::createTable( m_dbConnection ) &&
        Album::createTable( m_dbConnection ) &&
        AlbumTrack::createTable( m_dbConnection ) &&
        Show::createTable( m_dbConnection ) &&
        ShowEpisode::createTable( m_dbConnection ) &&
        Movie::createTable( m_dbConnection ) );
}


bool MediaLibrary::files( std::vector<FilePtr>& res )
{
    return File::fetchAll( m_dbConnection, res );
}

FilePtr MediaLibrary::file( const std::string& path )
{
    return File::fetch( m_dbConnection, path );
}

FilePtr MediaLibrary::addFile( const std::string& path )
{
    return File::create( m_dbConnection, path );
}

bool MediaLibrary::deleteFile( const std::string& mrl )
{
    return File::destroy( m_dbConnection, mrl );
}

bool MediaLibrary::deleteFile( FilePtr file )
{
    return File::destroy( m_dbConnection, std::static_pointer_cast<File>( file ) );
}

LabelPtr MediaLibrary::createLabel( const std::string& label )
{
    return Label::create( m_dbConnection, label );
}

bool MediaLibrary::deleteLabel( const std::string& text )
{
    return Label::destroy( m_dbConnection, text );
}

bool MediaLibrary::deleteLabel( LabelPtr label )
{
    return Label::destroy( m_dbConnection, std::static_pointer_cast<Label>( label ) );
}

AlbumPtr MediaLibrary::album( const std::string& id3Tag )
{
    // We can't use Cache helper, since albums are cached by primary keys
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
            " WHERE id3tag = ?";
    return SqliteTools::fetchOne<Album>( m_dbConnection, req, id3Tag );
}

AlbumPtr MediaLibrary::createAlbum( const std::string& id3Tag )
{
    return Album::create( m_dbConnection, id3Tag );
}

ShowPtr MediaLibrary::show(const std::string& name)
{
    static const std::string req = "SELECT * FROM " + policy::ShowTable::Name
            + " WHERE name = ?";
    return SqliteTools::fetchOne<Show>( m_dbConnection, req, name );
}

ShowPtr MediaLibrary::createShow(const std::string& name)
{
    return Show::create( m_dbConnection, name );
}

MoviePtr MediaLibrary::movie( const std::string& title )
{
    static const std::string req = "SELECT * FROM " + policy::MovieTable::Name
            + " WHERE title = ?";
    return SqliteTools::fetchOne<Movie>( m_dbConnection, req, title );
}

MoviePtr MediaLibrary::createMovie( const std::string& title )
{
    return Movie::create( m_dbConnection, title );
}

void MediaLibrary::addMetadataService(IMetadataService* service)
{
    typedef std::unique_ptr<IMetadataService> MdsPtr;
    std::function<bool(const MdsPtr&, const MdsPtr&)> comp = []( const MdsPtr& a, const MdsPtr& b )
    {
        // We want higher priority first
        return a->priority() > b->priority();
    };
    m_mdServices.push_back( MdsPtr( service ) );
    std::push_heap( m_mdServices.begin(), m_mdServices.end(), comp );
}
