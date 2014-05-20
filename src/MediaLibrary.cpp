#include "MediaLibrary.h"
#include "SqliteTools.h"
#include "File.h"
#include "Label.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Show.h"
#include "ShowEpisode.h"

MediaLibrary::MediaLibrary()
    : m_dbConnection( NULL )
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
        ShowEpisode::createTable( m_dbConnection ) );
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
