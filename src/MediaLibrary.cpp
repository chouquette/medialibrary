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

bool MediaLibrary::initialize(const std::string& dbPath)
{
    int res = sqlite3_open( dbPath.c_str(), &m_dbConnection );
    // PRAGMA foreign_keys = ON;
    if ( res != SQLITE_OK )
        return false;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2( m_dbConnection, "PRAGMA foreign_keys = ON", -1, &stmt, NULL );
    if ( sqlite3_step( stmt ) != SQLITE_DONE )
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
    auto f = File::create( path );
    if ( f->insert( m_dbConnection ) == false )
    {
        return nullptr;
    }
    return f;
}

LabelPtr MediaLibrary::createLabel( const std::string& label )
{
    auto l = Label::create( label );
    if ( l->insert( m_dbConnection ) == false )
    {
        return nullptr;
    }
    return l;
}
