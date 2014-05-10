#include "File.h"
#include "SqliteTools.h"

File::File( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_type = (Type)sqlite3_column_int( stmt, 1 );
    m_duration = sqlite3_column_int( stmt, 2 );
}

File::File(sqlite3* dbConnection)
    : m_dbConnection( dbConnection )
    , m_id( 0 )
{
}

bool File::insert()
{
    sqlite3_stmt* stmt;
    std::string req = "INSERT INTO File VALUES(NULL, ?, ?)";
    if ( sqlite3_prepare_v2( m_dbConnection, req.c_str(), -1, &stmt, NULL ) != SQLITE_OK )
        return false;
    sqlite3_bind_int( stmt, 1, m_type );
    sqlite3_bind_int( stmt, 2, m_duration );
    if ( sqlite3_step( stmt ) != SQLITE_DONE )
        return false;
    m_id = sqlite3_last_insert_rowid( m_dbConnection );
    return true;
}

IAlbumTrack*File::albumTrack()
{
}

const std::string&File::artworkUrl()
{
}

unsigned int File::duration()
{
    return m_duration;
}

IShowEpisode*File::showEpisode()
{
}

std::vector<ILabel*> File::labels()
{
}

bool File::CreateTable(sqlite3* connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS File("
            "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
            "type INTEGER, duration UNSIGNED INTEGER)";
    return SqliteTools::CreateTable( connection, req );
}
