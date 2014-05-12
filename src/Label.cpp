#include <cassert>
#include <cstdlib>
#include <cstring>

#include "Label.h"
#include "File.h"
#include "SqliteTools.h"

Label::Label( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_files( NULL )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_name = (const char*)sqlite3_column_text( stmt, 1 );
}

Label::Label( const std::string& name )
    : m_dbConnection( NULL )
    , m_id( 0 )
    , m_name( name )
    , m_files( NULL )
{
}


const std::string& Label::name()
{
    return m_name;
}

std::vector<IFile*> Label::files()
{
    if ( m_files == NULL )
    {
        const char* req = "SELECT f.* FROM Files f "
                "LEFT JOIN LabelFileRelation lfr ON lfr.id_file = f.id_file "
                "WHERE lfr.id_label = ?";
        SqliteTools::fetchAll<File>( m_dbConnection, req, m_id, m_files );
    }
    return *m_files;
}

bool Label::insert( sqlite3* dbConnection )
{
    assert( m_dbConnection == NULL );
    assert( m_id == 0 );
    sqlite3_stmt* stmt;
    const char* req = "INSERT INTO Label VALUES(NULL, ?)";
    if ( sqlite3_prepare_v2( dbConnection, req, -1, &stmt, NULL ) != SQLITE_OK )
    {
        std::cerr << "Failed to insert record: " << sqlite3_errmsg( dbConnection ) << std::endl;
        return false;
    }
    char* tmpName = strdup( m_name.c_str() );
    sqlite3_bind_text( stmt, 1, tmpName, -1, &free );
    if ( sqlite3_step( stmt ) != SQLITE_DONE )
        return false;
    m_dbConnection = dbConnection;
    m_id = sqlite3_last_insert_rowid( dbConnection );
    return true;
}

bool Label::createTable(sqlite3* dbConnection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS Label("
                "id_label INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT"
            ")";
    if ( SqliteTools::createTable( dbConnection, req ) == false )
        return false;
    req = "CREATE TABLE IF NOT EXISTS LabelFileRelation("
                "id_label INTEGER,"
                "id_file INTEGER,"
            "PRIMARY KEY (id_label, id_file)"
            "FOREIGN KEY(id_label) REFERENCES Label(id_label) ON DELETE CASCADE,"
            "FOREIGN KEY(id_file) REFERENCES File(id_file) ON DELETE CASCADE);";
    return SqliteTools::createTable( dbConnection, req );
}

bool Label::link(File* file)
{
    if ( m_dbConnection == NULL || m_id == 0 )
    {
        std::cerr << "A label needs to be inserted in database before being linked to a file" << std::endl;
        return false;
    }
    const char* req = "INSERT INTO LabelFileRelation VALUES(?, ?)";
    sqlite3_stmt* stmt;
    if ( sqlite3_prepare_v2( m_dbConnection, req, -1, &stmt, NULL ) != SQLITE_OK )
    {
        std::cerr << "Failed to insert record: " << sqlite3_errmsg( m_dbConnection ) << std::endl;
        return false;
    }
    sqlite3_bind_int( stmt, 1, m_id );
    sqlite3_bind_int( stmt, 2, file->id() );
    bool res = sqlite3_step( stmt ) == SQLITE_DONE;
    sqlite3_finalize( stmt );
    return res;
}
