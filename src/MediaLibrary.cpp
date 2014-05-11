#include "MediaLibrary.h"
#include "SqliteTools.h"
#include "File.h"
#include "Label.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Show.h"
#include "ShowEpisode.h"

MediaLibrary::MediaLibrary()
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


const std::vector<IFile*>&MediaLibrary::files()
{
    if ( m_files == NULL )
    {
        const char* req = "SELECT * FROM Files";
        SqliteTools::fetchAll<File>( m_dbConnection, req, 0, m_files );
    }
    return *m_files;
}
