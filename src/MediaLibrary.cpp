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
    , m_files( NULL )
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


const std::vector<IFile*>& MediaLibrary::files()
{
    if ( m_files == NULL )
    {
        const char* req = "SELECT * FROM File";
        SqliteTools::fetchAll<File>( m_dbConnection, req, 0, m_files );
    }
    return *m_files;
}

IFile*MediaLibrary::file( const std::string& path )
{
    if ( m_files == NULL )
    {
        // FIXME: This is probably ineficient.
        // Consider loading the file itself from the DB & eventually store it in a tmp
        // vector? Or implement caching globally for each class
        files();
    }

    std::vector<IFile*>::iterator it = m_files->begin();
    std::vector<IFile*>::iterator ite = m_files->end();
    while ( it != ite )
    {
        if ( (*it)->mrl() == path )
            return *it;
        ++it;
    }
    return NULL;
}

IFile* MediaLibrary::addFile( const std::string& path )
{
    File* f = new File( path );
    if ( f->insert( m_dbConnection ) == false )
    {
        delete f;
        return NULL;
    }
    return f;
}
