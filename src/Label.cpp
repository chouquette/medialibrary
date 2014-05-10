#include "Label.h"

#include "SqliteTools.h"

Label::Label( sqlite3* dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_files( NULL )
{
    m_id = sqlite3_column_int( stmt, 1 );
    m_name = (const char*)sqlite3_column_text( stmt, 2 );
}


const std::string& Label::name()
{
    return m_name;
}

std::vector<IFile*> Label::files()
{
    if ( m_files == NULL )
    {
        m_files = new std::vector<IFile*>;
        const char* req = "SELECT * FROM Files f"
                "LEFT JOIN LabelFileRelation lfr ON lfr.id_file = f.id_file "
                "WHERE lfr.id_label = ?";
        sqlite3_stmt* stmt;
        int res = sqlite3_prepare_v2( m_dbConnection, req, -1, &stmt, NULL );
        if ( res != SQLITE_OK )
            return ;
        sqlite3_bind_int( stmt, 1, m_id );
        res = sqlite3_step( stmt );
        while ( res == SQLITE_ROW )
        {
            IFile* f = new File( m_dbConnection, stmt );
            m_files->push_back( f );
            res = sqlite3_step( stmt );
        }
        sqlite3_finalize( stmt );
    }
    return *m_files;
}

bool Label::createTable(sqlite3* dbConnection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS Label("
                "id_label INTEGER PRIMARY KEY AUTO INCREMENT, "
                "name TEXT"
            ")";
    if ( SqliteTools::CreateTable( dbConnection, req ) )
        return false;
    req = "CREATE TABLE IF NOT EXISTS LabelFileRelation("
                "id_label INTEGER,"
                "id_file INTEGER,"
            "PRIMARY KEY (id_label, id_file)"
            "FOREIGN KEY(id_label) REFERENCES Label(id_label) ON DELETE CASCADE,"
            "FOREIGN KEY(id_file) REFERENCES File(id_file) ON DELETE CASCADE);";
    return SqliteTools::CreateTable( dbConnection, req );
}
