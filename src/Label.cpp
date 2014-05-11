#include "Label.h"
#include "File.h"
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
        const char* req = "SELECT * FROM Files f"
                "LEFT JOIN LabelFileRelation lfr ON lfr.id_file = f.id_file "
                "WHERE lfr.id_label = ?";
        SqliteTools::fetchAll<File>( m_dbConnection, req, m_id, m_files );
    }
    return *m_files;
}

bool Label::createTable(sqlite3* dbConnection)
{
    const char* req = "CREATE TABLE IF NOT EXISTS Label("
                "id_label INTEGER PRIMARY KEY AUTO INCREMENT, "
                "name TEXT"
            ")";
    if ( SqliteTools::createTable( dbConnection, req ) )
        return false;
    req = "CREATE TABLE IF NOT EXISTS LabelFileRelation("
                "id_label INTEGER,"
                "id_file INTEGER,"
            "PRIMARY KEY (id_label, id_file)"
            "FOREIGN KEY(id_label) REFERENCES Label(id_label) ON DELETE CASCADE,"
            "FOREIGN KEY(id_file) REFERENCES File(id_file) ON DELETE CASCADE);";
    return SqliteTools::createTable( dbConnection, req );
}
